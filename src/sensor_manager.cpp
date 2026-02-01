#include "sensor_manager.h"
#include "HX711.h"
#include "config.h"

// --- 內部私有變數 (僅限此檔案使用) ---
static HX711 scale;
static float internal_cached_weight = 0.0;
static unsigned long last_read_time = 0;

#if SIMULATE_SENSOR
static float sim_weight = 0.0;
#endif

// --- 內部私有函式：負責判斷讀取模擬還是真實硬體 ---
static float _readRawWeight()
{
#if SIMULATE_SENSOR
    // 模擬模式：每次呼叫微增
    sim_weight += 0.05;
    if (sim_weight > 5.0)
        sim_weight = 0.0;
    return sim_weight;
#else
    // 真實模式
    if (!scale.is_ready())
        return -1.0;

    float raw = scale.get_units(5); // 讀取 5 次平均
    // 雜訊過濾
    if (abs(raw) < 0.5)
        raw = 0.0;
    return raw / 1000.0; // 轉公斤
#endif
}

// --- 公開介面實作 ---

void initSensor()
{
#if !SIMULATE_SENSOR
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(LOADCELL_SCALE_FACTOR);
    scale.tare();
    Serial.println("HX711 Hardware Initialized.");
#else
    Serial.println("Sensor Simulation Mode Active.");
    sim_weight = 0.0;
#endif
}

void updateSensor()
{
    // 這裡管理採樣頻率 (500ms)
    if (millis() - last_read_time > 500)
    {
        internal_cached_weight = _readRawWeight();
        last_read_time = millis();
    }
}

float getCachedWeight()
{
    return internal_cached_weight;
}

void tareSensor()
{
#if SIMULATE_SENSOR
    sim_weight = 0.0;
    internal_cached_weight = 0.0;
#else
    if (scale.is_ready())
        scale.tare();
#endif
    Serial.println("Sensor Tared.");
}