#pragma once
#include <Arduino.h>

// =============================================================================
// Feature Switches (true / false)
// Note: MQTT_ENABLED and WEB_SERVER_ENABLED require WIFI_ENABLED true.
// =============================================================================
#define WIFI_ENABLED        true
#define WEB_SERVER_ENABLED  true
#define MQTT_ENABLED        false
#define AUTO_LOGGER_ENABLED true
#define OLED_ENABLED        true
#define SIMULATE_SENSOR     false

// WiFi
const char *const STA_WIFI_SSID = "YOUR_AP_PASS";
const char *const STA_WIFI_PASS = "YOUR_AP_PASS";
const char *const AP_WIFI_SSID  = "ESP32_Weight_Scale";
const char *const AP_WIFI_PASS  = "YOUR_AP_PASS";

// MQTT  (broker runs on PC via docker/docker-compose.mqtt.yml)
const char *const MQTT_BROKER_IP     = "YOUR_AP_PASS";
const int         MQTT_BROKER_PORT   = 1884;
const char *const MQTT_CLIENT_ID     = "esp32-weight-scale";
const char *const MQTT_TOPIC_SENSOR1 = "weight-scale/sensor1";
const char *const MQTT_TOPIC_SENSOR2 = "weight-scale/sensor2";
const char *const MQTT_TOPIC_TOTAL   = "weight-scale/total";
const unsigned long MQTT_PUBLISH_INTERVAL_MS = 5000;

// OLED (SSD1306 via I2C — remapped away from HX711 pins)
const int OLED_SDA_PIN  = 4;
const int OLED_SCL_PIN  = 5;
const int OLED_PWR_PIN  = 15;  // GPIO used as OLED VCC (~20 mA, within 40 mA GPIO limit)

// OLED auto-cycle timing
const unsigned long OLED_TOTAL_SHOW_MS  = 5000;  // How long to show Total  (ms)
const unsigned long OLED_SENSOR_SHOW_MS = 2000;  // How long to show each sensor (ms)

// Deep-sleep + button wake-up (future feature — currently disabled)
// See include/deep_sleep_manager.h and src/deep_sleep_manager.cpp
#define DEEP_SLEEP_ENABLED false
const int WAKE_BTN_PIN = 32;  // Wake-up button signal (INPUT_PULLUP, active LOW — supports ext0)
const int WAKE_BTN_GND = 33;  // GPIO used as button GND (OUTPUT LOW — button draws only µA)

// HX711 pins  (DT / SCK — no ACC pin)
const int LOADCELL1_DOUT_PIN = 21;
const int LOADCELL1_SCK_PIN  = 22;
const int LOADCELL2_DOUT_PIN = 18;
const int LOADCELL2_SCK_PIN  = 19;

// Calibration  (scale_factor = raw_reading / known_weight_kg)
const float LOADCELL1_SCALE_FACTOR = 85000.0;
const float LOADCELL2_SCALE_FACTOR = 85000.0;

// Auto-logger
const unsigned long STARTUP_RECORD_DELAY_MS = 10000;
const unsigned long AUTO_RECORD_INTERVAL_MS = 3600000;
const int           MAX_RECORDS             = 10;
