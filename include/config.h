#pragma once

#include <Arduino.h>
#include <WebServer.h>

// =============================================================================
// Feature Switches — set to true/false to enable or disable each subsystem
//
// Dependency note:
//   MQTT_ENABLED and WEB_SERVER_ENABLED both require WIFI_ENABLED true.
// =============================================================================
#define WIFI_ENABLED        true   // Connect to WiFi (STA + AP fallback)
#define WEB_SERVER_ENABLED  true   // Serve the HTTP web UI on port 80
#define MQTT_ENABLED        true   // Publish weight data to MQTT broker
#define AUTO_LOGGER_ENABLED true   // Log weight hourly and on startup to LittleFS
// =============================================================================

/**
 * WiFi Configuration
 *
 * STA mode: ESP32 joins your home/lab network so it can reach the MQTT broker.
 * The web interface is still accessible at the DHCP-assigned IP shown on the
 * serial monitor after boot.
 *
 * Fallback AP: If STA connection fails, the ESP32 starts a soft-AP so you can
 * still reach the web interface (MQTT will be unavailable in that case).
 */
const char *const STA_WIFI_SSID = "YOUR_AP_PASS";       // Your home/lab network SSID
const char *const STA_WIFI_PASS = "YOUR_AP_PASS";      // Your home/lab network password
const char *const AP_WIFI_SSID  = "ESP32_Weight_Scale"; // Fallback AP SSID
const char *const AP_WIFI_PASS  = "YOUR_AP_PASS";        // Fallback AP password

// Keep legacy alias so other files that include config.h still compile
const char *const WIFI_SSID = AP_WIFI_SSID;
const char *const WIFI_PASS = AP_WIFI_PASS;

/**
 * MQTT Configuration
 *
 * The broker runs on your PC via Docker (docker/docker-compose.mqtt.yml).
 * Topics published by the ESP32:
 *   weight-scale/sensor1  – left load cell (kg)
 *   weight-scale/sensor2  – right load cell (kg)
 *   weight-scale/total    – combined weight  (kg)
 */
const char *const MQTT_BROKER_IP   = "YOUR_AP_PASS";   // PC fixed IP on NOL_WIFI
const int         MQTT_BROKER_PORT = 1884;             // host port mapped in docker-compose.mqtt.yml
const char *const MQTT_CLIENT_ID   = "esp32-weight-scale";
const char *const MQTT_TOPIC_SENSOR1 = "weight-scale/sensor1";
const char *const MQTT_TOPIC_SENSOR2 = "weight-scale/sensor2";
const char *const MQTT_TOPIC_TOTAL   = "weight-scale/total";
const unsigned long MQTT_PUBLISH_INTERVAL_MS = 5000; // Publish every 5 seconds

/**
 * Hardware Pin Assignments for HX711 Load Cell Sensors
 *
 * Sensor 1 (original):
 *   DT  -> GPIO 21
 *   SCK -> GPIO 22
 *
 * Sensor 2 (new):
 *   DT  -> GPIO 18
 *   SCK -> GPIO 19
 *
 * Note: HX711 only needs DT and SCK for data. There is NO "ACC" pin.
 *       Power each HX711 from an external 3.3V/5V source if the ESP32
 *       3V3 pin cannot supply enough current for both modules.
 */
const int LOADCELL1_DOUT_PIN = 21; // Sensor 1 data output
const int LOADCELL1_SCK_PIN  = 22; // Sensor 1 clock

const int LOADCELL2_DOUT_PIN = 18; // Sensor 2 data output
const int LOADCELL2_SCK_PIN  = 19; // Sensor 2 clock

/**
 * Sensor Simulation Mode and Per-Sensor Calibration Scale Factors
 *
 * Calibration procedure:
 *   1. Place a known weight on the load cell
 *   2. Read the raw ADC value from serial output
 *   3. scale_factor = raw_reading / known_weight_in_kg
 *
 * Both sensors may differ depending on load cell batch; calibrate each independently.
 */
#define SIMULATE_SENSOR false
const float LOADCELL1_SCALE_FACTOR = 85000.0;
const float LOADCELL2_SCALE_FACTOR = 85000.0;

/**
 * Auto-recording configuration
 */
const unsigned long STARTUP_RECORD_DELAY_MS = 10000;
const unsigned long AUTO_RECORD_INTERVAL_MS = 3600000;

/**
 * Maximum number of records to store in LittleFS
 */
const int MAX_RECORDS = 10;
