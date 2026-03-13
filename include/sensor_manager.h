#pragma once

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
Used for total bee-box weight and data logging.
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
