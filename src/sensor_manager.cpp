#include "sensor_manager.h"
#include "HX711.h"
#include "config.h"
#include <Arduino.h>
#include <driver/gpio.h>

static HX711 scale1;
static HX711 scale2;

volatile float cached_weight1 = 0.0;
volatile float cached_weight2 = 0.0;

static unsigned long last_read_time = 0;

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
    float raw = scale1.get_units(3);
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
    float raw = scale2.get_units(3);
    if (fabsf(raw) < 0.01f) raw = 0.0f;
    return raw;
#endif
}

/* ── Public API ───────────────────────────────────────────────────────── */

void initSensor(long savedOffset1, long savedOffset2)
{
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
        scale1.set_scale(LOADCELL1_SCALE_FACTOR);
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
        scale2.set_scale(LOADCELL2_SCALE_FACTOR);
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
        Serial.println("[Sensor] Warning: Sensor 2 not responding. Check wiring (DT=GPIO18, SCK=GPIO19).");
    }
#else
    Serial.println("[Sensor] Simulation mode: dual sensor active (random)");
    randomSeed(analogRead(0));
    sim_weight1 = 25.0f;
    sim_weight2 = 22.0f;
#endif
}

void updateSensor()
{
    if (millis() - last_read_time >= 500)
    {
        cached_weight1 = _doRead1();
        cached_weight2 = _doRead2();
        last_read_time = millis();
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

void powerDownSensors()
{
#if !SIMULATE_SENSOR
    scale1.power_down();
    scale2.power_down();
    Serial.println("[Sensor] HX711 modules powered down");
#endif
}
