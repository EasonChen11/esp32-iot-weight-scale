#pragma once

#include <Arduino.h>
#include <WebServer.h>

/**
 * WiFi Configuration
 */
const char *const WIFI_SSID = "ESP32_Weight_Scale";
const char *const WIFI_PASS = "YOUR_AP_PASS";

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
