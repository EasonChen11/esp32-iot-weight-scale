#include "sensor_manager.h"
#include "HX711.h"
#include "config.h"
#include <Arduino.h>

// --- 內部私有變數 ---
static HX711 scale;
static float internal_cached_weight = 0.0;
static unsigned long last_read_time = 0;

#if SIMULATE_SENSOR
static float sim_weight = 0.0;
#endif

// --- 內部私有函式：讀取重量的實作層 ---
static float _doRead() {
#if SIMULATE_SENSOR
    sim_weight += 0.05;
    if (sim_weight > 75.0) sim_weight = 10.0; // 模擬 10~75kg 範圍
    return sim_weight;
#else
    if (!scale.is_ready()) return -1.0;
    
    // get_units(5) 回傳的是 (原始讀值 - Offset) / Scale
    float raw = scale.get_units(5); 
    
    // 簡單的小數抖動過濾
    if (abs(raw) < 0.01) raw = 0.0; 
    
    return raw / 1000.0; // 假設 Scale Factor 是以克為單位，轉公斤
#endif
}

// --- 公開介面實作 ---

void initSensor(long savedOffset) {
#if !SIMULATE_SENSOR
    scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
    scale.set_scale(LOADCELL_SCALE_FACTOR);

    if (savedOffset != 0) {
        scale.set_offset(savedOffset);
        Serial.printf("[Sensor] 已套用絕對零點: %ld\n", savedOffset);
    } else {
        scale.tare();
        Serial.println("[Sensor] 無紀錄，執行自動歸零");
    }
#else
    Serial.println("[Sensor] 模擬模式啟動");
    sim_weight = 10.0;
#endif
}

void updateSensor() {
    if (millis() - last_read_time >= 500) {
        internal_cached_weight = _doRead();
        last_read_time = millis();
    }
}

float getCachedWeight() {
    return internal_cached_weight;
}

void tareSensor() {
#if SIMULATE_SENSOR
    sim_weight = 0.0;
    internal_cached_weight = 0.0;
#else
    if (scale.is_ready()) scale.tare();
#endif
    Serial.println("[Sensor] 執行暫時性歸零");
}

long captureAbsoluteOffset() {
#if !SIMULATE_SENSOR
    if (scale.is_ready()) {
        scale.tare(); // 先把當前狀態設為 0
        return scale.get_offset(); // 回傳這個 0 對應的數值給呼叫者
    }
    return 0;
#else
    return 123456; // 模擬模式回傳假數值
#endif
}