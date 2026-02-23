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
