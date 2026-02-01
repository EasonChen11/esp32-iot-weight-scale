#pragma once

/*
Initialize the weight sensor with optional absolute zero offset.
If no saved offset is provided, the sensor will auto-tare on startup.

Parameters:
  savedOffset (long): The absolute zero-point offset value to apply.
                      If 0, the sensor will perform automatic tare. Default is 0.

Returns:
  void

Example:
  initSensor(0);          // Auto-tare on startup
  initSensor(123456);     // Apply saved calibration offset
*/
void initSensor(long savedOffset = 0);

/*
Update the cached weight value at regular 500ms intervals.
This function should be called continuously in the main loop() to ensure
fresh weight data is available for retrieval.

Parameters:
  none

Returns:
  void

Example:
  updateSensor();  // Call this every iteration of loop()
*/
void updateSensor();

/*
Retrieve the current cached weight value in kilograms.
This returns the most recently read weight from the sensor buffer,
updated every 500ms by the updateSensor() function.

Parameters:
  none

Returns:
  float: The current cached weight in kilograms (kg)

Example:
  float weight = getCachedWeight();  // Returns 2.45 kg
*/
float getCachedWeight();

/*
Perform temporary tare (zero adjustment) in memory only.
This zeros out the current weight reading but does not persist
the absolute offset to storage. Use setAbsoluteZero() for permanent calibration.

Parameters:
  none

Returns:
  void

Example:
  tareSensor();  // Zero out the current reading
*/
void tareSensor();

/*
Capture and return the current absolute zero-point offset value.
This function performs a tare operation and retrieves the raw offset
value that corresponds to the current zero state of the load cell.

Parameters:
  none

Returns:
  long: The raw offset value for the current zero state

Example:
  long offset = captureAbsoluteOffset();  // Returns 123456
*/
long captureAbsoluteOffset();