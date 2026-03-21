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
#define SIMULATE_SENSOR     true

// WiFi & MQTT credentials — kept in a .gitignored file
// Copy config_secrets.h.example → config_secrets.h and fill in your values
#include "config_secrets.h"

// MQTT  (broker runs on PC via docker/docker-compose.mqtt.yml)
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

// NTP time synchronization (requires WIFI_ENABLED)
#define NTP_ENABLED         true
const char *const NTP_SERVER1            = "pool.ntp.org";
const char *const NTP_SERVER2            = "time.nist.gov";
const long        NTP_GMT_OFFSET_SEC     = 28800;   // UTC+8 (Taiwan)
const int         NTP_DAYLIGHT_OFFSET_SEC = 0;

// Wake-up schedule (daily alarm times stored in NVS)
#define SCHEDULE_ENABLED    true
const int MAX_SCHEDULE_ENTRIES = 10;

// Deep-sleep + scheduled wake-up
// Enable to auto-sleep after AWAKE_DURATION_MS and wake at scheduled times.
// See include/deep_sleep_manager.h and src/deep_sleep_manager.cpp
#define DEEP_SLEEP_ENABLED true
const int WAKE_BTN_PIN = 32;  // Wake-up button signal (INPUT_PULLUP, active LOW — supports ext0)
const int WAKE_BTN_GND = 33;  // GPIO used as button GND (OUTPUT LOW — button draws only µA)
const unsigned long AWAKE_DURATION_MS = 600000;  // Stay awake 10 min after boot before sleeping

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
