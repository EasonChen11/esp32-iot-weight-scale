#pragma once


#include <Arduino.h>
#include <WebServer.h>

// --- WiFi settings ---
const char *const WIFI_SSID = "ESP32_Weight_Scale";
const char *const WIFI_PASS = "YOUR_AP_PASS";

// --- Hardware pin assignments ---
const int LOADCELL_DOUT_PIN = 18;
const int LOADCELL_SCK_PIN = 19;

// --- HX711 scale factor ---
#define SIMULATE_SENSOR true               // Set to true to simulate sensor data without hardware
const float LOADCELL_SCALE_FACTOR = 420.0; // factor for calibration if read is 210000, the actual weight is 500g, then scale factor = 210000 / 500 = 420.0
