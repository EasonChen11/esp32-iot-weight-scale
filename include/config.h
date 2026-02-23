#pragma once

#include <Arduino.h>
#include <WebServer.h>

/**
 * WiFi Configuration
 */
const char *const WIFI_SSID = "ESP32_Weight_Scale";
const char *const WIFI_PASS = "***REMOVED***";

/**
 * Hardware Pin Assignments for HX711 Load Cell Sensor
 */
const int LOADCELL_DOUT_PIN = 21; // Data output pin
const int LOADCELL_SCK_PIN = 22;  // Clock signal pin

/**
 * Sensor Simulation Mode and Calibration
 */
#define SIMULATE_SENSOR false
const float LOADCELL_SCALE_FACTOR = 85000.0;

/**
 * auto-recording configuration
 */
const unsigned long STARTUP_RECORD_DELAY_MS = 10000; // Delay after boot before first record (milliseconds)
// If you prefer millis-based scheduling in the future, adjust this value
const unsigned long AUTO_RECORD_INTERVAL_MS = 3600000;

/**
 * Maximum number of records to store in LittleFS
 */
const int MAX_RECORDS = 10;