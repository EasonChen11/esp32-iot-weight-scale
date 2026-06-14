#pragma once

#include <Arduino.h>

/*
Initialize both weight sensors with optional absolute zero offsets.
If an offset is 0 the corresponding sensor will auto-tare on startup.

Parameters:
  savedOffset1 (long): Calibration offset for sensor 1. Default 0 = auto-tare.
  savedOffset2 (long): Calibration offset for sensor 2. Default 0 = auto-tare.
*/
void initSensor(long savedOffset1 = 0, long savedOffset2 = 0);

/*
Update cached weight values for both sensors at 500ms intervals.
Call this continuously from loop().
*/
void updateSensor();

/*
Return the combined (summed) weight of both sensors in kg.
Used for total beehive weight and data logging.
*/
float getCachedWeight();

/*
Return sensor 1 cached weight in kg.
*/
float getCachedWeight1();

/*
Return sensor 2 cached weight in kg.
*/
float getCachedWeight2();

/*
Take a fresh, high-precision reading for the permanent record (NOT the live display).
Reads LOG_SAMPLE_COUNT raw samples, drops LOG_TRIM_COUNT highest + lowest, and averages
the rest (trimmed mean) — accurate and robust to occasional spikes. Each sample is guarded
by wait_ready_timeout; if too few samples arrive it falls back to the cached value.
*/
float readLogWeight1();
float readLogWeight2();

/*
Tare sensor 1 only (temporary, not persisted).
*/
void tareSensor1();

/*
Tare sensor 2 only (temporary, not persisted).
*/
void tareSensor2();

/*
Tare both sensors simultaneously.
*/
void tareSensor();

/*
Capture and return the absolute zero-point offset for sensor 1.
*/
long captureAbsoluteOffset1();

/*
Capture and return the absolute zero-point offset for sensor 2.
*/
long captureAbsoluteOffset2();

/*
Alias for captureAbsoluteOffset1() – kept for backward compatibility.
*/
long captureAbsoluteOffset();

// Scale factor calibration: reads 10 raw samples, computes factor from known weight,
// applies to HX711 in-memory and persists to NVS. Returns the new factor on success,
// or -1.0f on failure (errorOut populated with a Chinese error message for the user).
float calibrateScaleFactor1(float knownKg, String &errorOut);
float calibrateScaleFactor2(float knownKg, String &errorOut);

/*
Power down both HX711 modules to save ~1.5 mA each during deep sleep.
Call before esp_deep_sleep_start(). Sensors are re-initialized by
initSensor() on the next boot cycle, which calls scale.begin() internally.
*/
void powerDownSensors();
