#include "sensor_manager.h"
#include "HX711.h"
#include "config.h"
#include <Arduino.h>
#include <driver/gpio.h>
#include "storage/nvs_storage.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

static HX711 scale1;
static HX711 scale2;

// Serializes HX711 access between the Core-1 display read (updateSensor) and the
// Core-0 record read (readLogWeight*). Without it, simultaneous bit-banged reads
// from both cores corrupt the value. Created in initSensor().
static SemaphoreHandle_t hx711Mutex = nullptr;

volatile float cached_weight1 = 0.0;
volatile float cached_weight2 = 0.0;

static unsigned long last_read_time = 0;

// ── Display smoothing: per-sensor moving-median ring buffer ──────────────
static float medWin1[DISPLAY_MEDIAN_WINDOW];
static float medWin2[DISPLAY_MEDIAN_WINDOW];
static int   medCount1 = 0, medHead1 = 0;
static int   medCount2 = 0, medHead2 = 0;

// Ascending insertion sort for small float arrays (median / trimmed mean).
static void sortFloats(float *a, int n)
{
    for (int i = 1; i < n; i++) {
        float key = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > key) { a[j + 1] = a[j]; j--; }
        a[j + 1] = key;
    }
}

static void pushMedian1(float v)
{
    medWin1[medHead1] = v;
    medHead1 = (medHead1 + 1) % DISPLAY_MEDIAN_WINDOW;
    if (medCount1 < DISPLAY_MEDIAN_WINDOW) medCount1++;
}

static void pushMedian2(float v)
{
    medWin2[medHead2] = v;
    medHead2 = (medHead2 + 1) % DISPLAY_MEDIAN_WINDOW;
    if (medCount2 < DISPLAY_MEDIAN_WINDOW) medCount2++;
}

// Median of the samples collected so far (1..DISPLAY_MEDIAN_WINDOW).
static float median1()
{
    if (medCount1 == 0) return 0.0f;
    float tmp[DISPLAY_MEDIAN_WINDOW];
    for (int i = 0; i < medCount1; i++) tmp[i] = medWin1[i];
    sortFloats(tmp, medCount1);
    return tmp[(medCount1 - 1) / 2];
}

static float median2()
{
    if (medCount2 == 0) return 0.0f;
    float tmp[DISPLAY_MEDIAN_WINDOW];
    for (int i = 0; i < medCount2; i++) tmp[i] = medWin2[i];
    sortFloats(tmp, medCount2);
    return tmp[(medCount2 - 1) / 2];
}

#if SIMULATE_SENSOR
static float sim_weight1 = 10.0;
static float sim_weight2 = 15.0;
#endif

/* ── Internal readers ─────────────────────────────────────────────────── */

static float _doRead1()
{
#if SIMULATE_SENSOR
    float noise = (random(-500, 501)) / 1000.0f;   // -0.500 … +0.500 kg
    sim_weight1 = 25.0f + noise;
    return sim_weight1;
#else
    if (!scale1.is_ready()) return -1.0f;
    float raw = scale1.get_units(1); // single read — no busy-wait; smoothing done by median buffer
    if (fabsf(raw) < 0.01f) raw = 0.0f;
    return raw;
#endif
}

static float _doRead2()
{
#if SIMULATE_SENSOR
    float noise = (random(-500, 501)) / 1000.0f;
    sim_weight2 = 22.0f + noise;
    return sim_weight2;
#else
    if (!scale2.is_ready()) return -1.0f;
    float raw = scale2.get_units(1); // single read — no busy-wait; smoothing done by median buffer
    if (fabsf(raw) < 0.01f) raw = 0.0f;
    return raw;
#endif
}

/* ── Public API ───────────────────────────────────────────────────────── */

void initSensor(long savedOffset1, long savedOffset2)
{
    if (hx711Mutex == nullptr) hx711Mutex = xSemaphoreCreateMutex();
    Serial.printf("[Sensor] Offsets: sensor1=%ld, sensor2=%ld\n", savedOffset1, savedOffset2);

#if !SIMULATE_SENSOR
    // Release GPIO hold from previous deep-sleep power-down
    gpio_hold_dis((gpio_num_t)LOADCELL1_SCK_PIN);
    gpio_hold_dis((gpio_num_t)LOADCELL2_SCK_PIN);

    // ── Sensor 1 ──────────────────────────────────────────────────────
    scale1.begin(LOADCELL1_DOUT_PIN, LOADCELL1_SCK_PIN);
    Serial.println("[Sensor] Initializing sensor 1...");
    if (scale1.wait_ready_timeout(2000))
    {
        float factor1 = hasScaleFactor1() ? getScaleFactor1() : LOADCELL1_SCALE_FACTOR;
        scale1.set_scale(factor1);
        Serial.printf("[Sensor] Sensor 1 scale factor: %.2f%s\n",
                      factor1, hasScaleFactor1() ? " (NVS)" : " (default)");
        Serial.println("[Sensor] Sensor 1 ready");
    }
    else
    {
        Serial.println("[Sensor] Error: Sensor 1 not responding — retrying");
        delay(1000);
        initSensor(savedOffset1, savedOffset2);
        return;
    }

    if (savedOffset1 != 0)
    {
        scale1.set_offset(savedOffset1);
        Serial.printf("[Sensor] Sensor 1 calibration applied: %ld\n", savedOffset1);
    }
    else
    {
        scale1.tare();
        Serial.println("[Sensor] Sensor 1 auto-tare");
    }

    // ── Sensor 2 ──────────────────────────────────────────────────────
    scale2.begin(LOADCELL2_DOUT_PIN, LOADCELL2_SCK_PIN);
    Serial.println("[Sensor] Initializing sensor 2...");
    if (scale2.wait_ready_timeout(2000))
    {
        float factor2 = hasScaleFactor2() ? getScaleFactor2() : LOADCELL2_SCALE_FACTOR;
        scale2.set_scale(factor2);
        Serial.printf("[Sensor] Sensor 2 scale factor: %.2f%s\n",
                      factor2, hasScaleFactor2() ? " (NVS)" : " (default)");
        Serial.println("[Sensor] Sensor 2 ready");

        if (savedOffset2 != 0)
        {
            scale2.set_offset(savedOffset2);
            Serial.printf("[Sensor] Sensor 2 calibration applied: %ld\n", savedOffset2);
        }
        else
        {
            scale2.tare();
            Serial.println("[Sensor] Sensor 2 auto-tare");
        }
    }
    else
    {
        // Sensor 2 may not be wired yet — warn but do not block startup
        Serial.println("[Sensor] Warning: Sensor 2 not responding. Check wiring (DT=GPIO25, SCK=GPIO26).");
    }
#else
    Serial.println("[Sensor] Simulation mode: dual sensor active (random)");
    struct tm t;
    if (getLocalTime(&t, 0)) {
        unsigned long seed = (t.tm_year + 1900) * 10000UL + (t.tm_mon + 1) * 100 + t.tm_mday;
        seed = seed * 86400UL + t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec;
        randomSeed(seed);
        Serial.printf("[Sensor] Random seed from time: %lu\n", seed);
    } else {
        randomSeed(micros());
        Serial.println("[Sensor] No time available, seed from micros()");
    }
    sim_weight1 = 25.0f;
    sim_weight2 = 22.0f;
#endif
}

void updateSensor()
{
    if (millis() - last_read_time >= SENSOR_READ_INTERVAL_MS)
    {
        last_read_time = millis();

        // Try-lock: if a record read (readLogWeight) holds the HX711, skip this
        // tick — the displayed median just keeps its last value (no stall).
        if (hx711Mutex != nullptr && xSemaphoreTake(hx711Mutex, 0) != pdTRUE) return;

        float r1 = _doRead1();
        if (r1 >= 0.0f) pushMedian1(r1); // skip the -1.0 "not ready" sentinel
        float r2 = _doRead2();
        if (r2 >= 0.0f) pushMedian2(r2);

        if (hx711Mutex != nullptr) xSemaphoreGive(hx711Mutex);

        cached_weight1 = median1();
        cached_weight2 = median2();
        Serial.printf("[Sensor] S1=%.3f kg  S2=%.3f kg  Total=%.3f kg\n",
                      cached_weight1, cached_weight2,
                      cached_weight1 + cached_weight2);
    }
}

float getCachedWeight1() { return cached_weight1; }
float getCachedWeight2() { return cached_weight2; }

float getCachedWeight()
{
    // Returns the summed weight of both sensors (total beehive weight).
    // Negative values (sensor not ready) are treated as 0.
    float w1 = (cached_weight1 > 0.0f) ? cached_weight1 : 0.0f;
    float w2 = (cached_weight2 > 0.0f) ? cached_weight2 : 0.0f;
    return w1 + w2;
}

// Fresh high-precision read for the permanent record — trimmed mean of LOG_SAMPLE_COUNT
// raw samples (drop LOG_TRIM_COUNT highest + lowest). Spike-robust and averaged.
float readLogWeight1()
{
#if SIMULATE_SENSOR
    return _doRead1();
#else
    // Exclusive HX711 access for the whole multi-sample read (vs Core-1 updateSensor).
    if (hx711Mutex == nullptr || xSemaphoreTake(hx711Mutex, pdMS_TO_TICKS(3000)) != pdTRUE)
        return getCachedWeight1(); // couldn't acquire — fall back to last cached value
    float result;
    float samples[LOG_SAMPLE_COUNT];
    int n = 0;
    for (int i = 0; i < LOG_SAMPLE_COUNT; i++) {
        if (!scale1.wait_ready_timeout(2000)) break; // sensor unresponsive — stop gathering
        samples[n++] = scale1.get_units(1);
    }
    if (n < (2 * LOG_TRIM_COUNT + 1)) {
        result = getCachedWeight1(); // too few — fall back
    } else {
        sortFloats(samples, n);
        float sum = 0.0f;
        int cnt = 0;
        for (int i = LOG_TRIM_COUNT; i < n - LOG_TRIM_COUNT; i++) { sum += samples[i]; cnt++; }
        result = sum / cnt;
        if (fabsf(result) < 0.01f) result = 0.0f;
    }
    xSemaphoreGive(hx711Mutex);
    return result;
#endif
}

float readLogWeight2()
{
#if SIMULATE_SENSOR
    return _doRead2();
#else
    if (hx711Mutex == nullptr || xSemaphoreTake(hx711Mutex, pdMS_TO_TICKS(3000)) != pdTRUE)
        return getCachedWeight2();
    float result;
    float samples[LOG_SAMPLE_COUNT];
    int n = 0;
    for (int i = 0; i < LOG_SAMPLE_COUNT; i++) {
        if (!scale2.wait_ready_timeout(2000)) break;
        samples[n++] = scale2.get_units(1);
    }
    if (n < (2 * LOG_TRIM_COUNT + 1)) {
        result = getCachedWeight2();
    } else {
        sortFloats(samples, n);
        float sum = 0.0f;
        int cnt = 0;
        for (int i = LOG_TRIM_COUNT; i < n - LOG_TRIM_COUNT; i++) { sum += samples[i]; cnt++; }
        result = sum / cnt;
        if (fabsf(result) < 0.01f) result = 0.0f;
    }
    xSemaphoreGive(hx711Mutex);
    return result;
#endif
}

void tareSensor1()
{
#if SIMULATE_SENSOR
    sim_weight1 = 0.0f;
    cached_weight1 = 0.0f;
#else
    if (scale1.is_ready() || scale1.wait_ready_timeout(2000))
        scale1.tare();
#endif
    Serial.println("[Sensor] Sensor 1 tare performed");
}

void tareSensor2()
{
#if SIMULATE_SENSOR
    sim_weight2 = 0.0f;
    cached_weight2 = 0.0f;
#else
    if (scale2.is_ready() || scale2.wait_ready_timeout(2000))
        scale2.tare();
#endif
    Serial.println("[Sensor] Sensor 2 tare performed");
}

void tareSensor()
{
    tareSensor1();
    tareSensor2();
}

long captureAbsoluteOffset1()
{
#if !SIMULATE_SENSOR
    if (scale1.is_ready() || scale1.wait_ready_timeout(2000))
    {
        scale1.tare();
        Serial.println("[Sensor] Capturing sensor 1 absolute offset...");
        return scale1.get_offset();
    }
    Serial.println("[Sensor] Sensor 1 not ready, cannot capture offset");
    return 0;
#else
    return 123456;
#endif
}

long captureAbsoluteOffset2()
{
#if !SIMULATE_SENSOR
    if (scale2.is_ready() || scale2.wait_ready_timeout(2000))
    {
        scale2.tare();
        Serial.println("[Sensor] Capturing sensor 2 absolute offset...");
        return scale2.get_offset();
    }
    Serial.println("[Sensor] Sensor 2 not ready, cannot capture offset");
    return 0;
#else
    return 234567;
#endif
}

long captureAbsoluteOffset() { return captureAbsoluteOffset1(); }

float calibrateScaleFactor1(float knownKg, String &errorOut)
{
#if SIMULATE_SENSOR
    Serial.printf("[Sensor] S1 simulated calibration with known=%.2f kg\n", knownKg);
    return 85000.0f;
#else
    if (!scale1.is_ready() && !scale1.wait_ready_timeout(2000)) {
        errorOut = "感測器無回應";
        return -1.0f;
    }
    if (knownKg <= 0.0f) {
        errorOut = "重量必須大於 0";
        return -1.0f;
    }
    if (knownKg > 200.0f) {
        errorOut = "重量超出合理範圍 (>200 kg)";
        return -1.0f;
    }

    long rawAvg = scale1.read_average(10);
    long offset = scale1.get_offset();
    long delta  = rawAvg - offset;

    if (abs(delta) < 1000) {
        errorOut = "未偵測到負載 — 確認重物已放上";
        return -1.0f;
    }

    float newFactor = (float)delta / knownKg;

    if (newFactor < 1000.0f || newFactor > 500000.0f) {
        errorOut = "計算倍率超出合理範圍 — 檢查接線";
        return -1.0f;
    }

    scale1.set_scale(newFactor);
    saveScaleFactor1(newFactor);
    Serial.printf("[Sensor] S1 calibrated: factor=%.2f (raw=%ld, offset=%ld, delta=%ld, kg=%.2f)\n",
                  newFactor, rawAvg, offset, delta, knownKg);
    return newFactor;
#endif
}

float calibrateScaleFactor2(float knownKg, String &errorOut)
{
#if SIMULATE_SENSOR
    Serial.printf("[Sensor] S2 simulated calibration with known=%.2f kg\n", knownKg);
    return 85000.0f;
#else
    if (!scale2.is_ready() && !scale2.wait_ready_timeout(2000)) {
        errorOut = "感測器無回應";
        return -1.0f;
    }
    if (knownKg <= 0.0f) {
        errorOut = "重量必須大於 0";
        return -1.0f;
    }
    if (knownKg > 200.0f) {
        errorOut = "重量超出合理範圍 (>200 kg)";
        return -1.0f;
    }

    long rawAvg = scale2.read_average(10);
    long offset = scale2.get_offset();
    long delta  = rawAvg - offset;

    if (abs(delta) < 1000) {
        errorOut = "未偵測到負載 — 確認重物已放上";
        return -1.0f;
    }

    float newFactor = (float)delta / knownKg;

    if (newFactor < 1000.0f || newFactor > 500000.0f) {
        errorOut = "計算倍率超出合理範圍 — 檢查接線";
        return -1.0f;
    }

    scale2.set_scale(newFactor);
    saveScaleFactor2(newFactor);
    Serial.printf("[Sensor] S2 calibrated: factor=%.2f (raw=%ld, offset=%ld, delta=%ld, kg=%.2f)\n",
                  newFactor, rawAvg, offset, delta, knownKg);
    return newFactor;
#endif
}

void powerDownSensors()
{
#if !SIMULATE_SENSOR
    scale1.power_down();
    scale2.power_down();
    Serial.println("[Sensor] HX711 modules powered down");
#endif
}
