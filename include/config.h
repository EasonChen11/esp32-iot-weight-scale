#pragma once
#include <Arduino.h>

// =============================================================================
// Feature Switches (true / false)
// Note: MQTT_ENABLED and WEB_SERVER_ENABLED require WIFI_ENABLED true.
// Each switch is wrapped in #ifndef so it can be overridden at build time via
// -D flags (e.g. PLATFORMIO_BUILD_FLAGS="-DMQTT_ENABLED=false"), which the CI
// build matrix uses to verify every #if guard combination still compiles.
// =============================================================================
#ifndef WIFI_ENABLED
#define WIFI_ENABLED true
#endif
#ifndef WEB_SERVER_ENABLED
#define WEB_SERVER_ENABLED true
#endif
#ifndef MQTT_ENABLED
#define MQTT_ENABLED false
#endif
#ifndef AUTO_LOGGER_ENABLED
#define AUTO_LOGGER_ENABLED true
#endif
#ifndef OLED_ENABLED
#define OLED_ENABLED false
#endif
#ifndef DEV_MODE_ENABLED
#define DEV_MODE_ENABLED true   // serial-controlled runtime developer mode (RAM only, no NVS)
#endif
#ifndef GOOGLE_SHEETS_ENABLED
#define GOOGLE_SHEETS_ENABLED true
#endif
#ifndef WIFI_CONFIG_ENABLED
#define WIFI_CONFIG_ENABLED true
#endif
#ifndef SIMULATE_SENSOR
#define SIMULATE_SENSOR false
#endif

// WiFi & MQTT credentials — kept in a .gitignored file
// Copy config_secrets.h.example → config_secrets.h and fill in your values
#include "config_secrets.h"

// MQTT  (broker runs on PC via docker/docker-compose.mqtt.yml)
const char *const MQTT_CLIENT_ID = "esp32-weight-scale";
const char *const MQTT_TOPIC_SENSOR1 = "weight-scale/sensor1";
const char *const MQTT_TOPIC_SENSOR2 = "weight-scale/sensor2";
const char *const MQTT_TOPIC_TOTAL = "weight-scale/total";
const unsigned long MQTT_PUBLISH_INTERVAL_MS = 5000;

// OLED (SSD1306 via I2C — ESP32 default I2C pins)
const int OLED_SDA_PIN = 21;
const int OLED_SCL_PIN = 22;
const int OLED_PWR_PIN = 19; // GPIO used as OLED VCC (~20 mA, within 40 mA GPIO limit)

// OLED auto-cycle timing
const unsigned long OLED_TOTAL_SHOW_MS = 5000;  // How long to show Total  (ms)
const unsigned long OLED_SENSOR_SHOW_MS = 2000; // How long to show each sensor (ms)

// NTP time synchronization (requires WIFI_ENABLED)
#ifndef NTP_ENABLED
#define NTP_ENABLED true
#endif
const char *const NTP_SERVER1 = "216.239.35.0";   // time.google.com IP
const char *const NTP_SERVER2 = "time.google.com";
const char *const NTP_SERVER3 = "pool.ntp.org";
const long NTP_GMT_OFFSET_SEC = 28800; // UTC+8 (Taiwan)
const int NTP_DAYLIGHT_OFFSET_SEC = 0;

// Dynamic WiFi configuration (requires WIFI_ENABLED)
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 8000;

// Wake-up schedule (daily alarm times stored in NVS)
#ifndef SCHEDULE_ENABLED
#define SCHEDULE_ENABLED true
#endif
const int MAX_SCHEDULE_ENTRIES = 10;

// Deep-sleep + scheduled wake-up
// Enable to auto-sleep after AWAKE_DURATION_MS and wake at scheduled times.
// See include/deep_sleep_manager.h and src/deep_sleep_manager.cpp
#ifndef DEEP_SLEEP_ENABLED
#define DEEP_SLEEP_ENABLED true
#endif
const int WAKE_BTN_PIN = 32;                    // Wake-up button signal (INPUT_PULLUP, active LOW — supports ext0)
const int WAKE_BTN_GND = 33;                    // GPIO used as button GND (OUTPUT LOW — button draws only µA)
const unsigned long AWAKE_DURATION_MS = 600000; // Stay awake 10 min after boot before sleeping
const unsigned long WAKE_BTN_DEBOUNCE_MS = 200; // Software debounce window for awake-time button press
const unsigned long WAKE_BTN_STARTUP_GRACE_MS = 2000; // Ignore button presses for the first N ms after boot

// HX711 pins  (DT / SCK — no ACC pin)
const int LOADCELL1_DOUT_PIN = 13;
const int LOADCELL1_SCK_PIN = 14;
const int LOADCELL2_DOUT_PIN = 25;
const int LOADCELL2_SCK_PIN = 26;

// Calibration  (scale_factor = raw_reading / known_weight_kg)
const float LOADCELL1_SCALE_FACTOR = 85000.0;
const float LOADCELL2_SCALE_FACTOR = 85000.0;

// Auto-logger
const unsigned long STARTUP_RECORD_DELAY_MS = 10000;
const unsigned long AUTO_RECORD_INTERVAL_MS = 3600000;
const unsigned long TIME_SYNC_TIMEOUT_MS = 300000; // Max wait for time sync (5 min) — gives user time to configure WiFi via /network
const int MAX_RECORDS = 200;

// Deep-sleep fallback interval when time is unknown (seconds)
const int FALLBACK_WAKEUP_SEC = 3600; // 1 hour
