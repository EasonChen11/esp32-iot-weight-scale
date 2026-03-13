#pragma once
#include <Arduino.h>

/*
Initialize the SPIFFS (SPI Flash File System) for data storage.
This function must be called once during setup() to mount the filesystem
and prepare for reading/writing persistent data.

Parameters:
  none

Returns:
  void

Example:
  initStorage();  // Call once in setup()
*/
void initStorage();

/*
Retrieve all stored weight records as a JSON string.
Returns a JSON array containing all stored measurement records, or an empty
array "[]" if no records exist or if the file cannot be read.

Parameters:
  none

Returns:
  String: JSON array of records formatted as [{"time":"HH:MM:SS","weight":"2.45"}]

Example:
  String json = getRecordsJson();  // Returns "[{"time":"10:30:00","weight":"5.23"}]"
*/
String getRecordsJson();

/*
Add a new weight measurement record to persistent storage.
This function inserts a new record at the beginning of the records array
and maintains a maximum of 10 recent records by removing the oldest entries.

Parameters:
  time (String): Timestamp string for the measurement (e.g., "10:30:45")
  weight (String): Weight value as a string (e.g., "2.45")

Returns:
  void

Example:
  addRecordToStorage("10:30:45", "5.23");  // Add new measurement record
*/
void addRecordToStorage(String time, String weight);

/*
Delete a specific weight record by index from persistent storage.
Removes the record at the given index position from the records array.

Parameters:
  index (int): Zero-based index of the record to delete

Returns:
  void

Example:
  deleteRecordFromStorage(0);  // Delete the first record
*/
void deleteRecordFromStorage(int index);

/*
Clear all weight records from persistent storage.
Deletes all measurement records and leaves an empty array in storage.

Parameters:
  none

Returns:
  void

Example:
  clearRecordsInStorage();  // Delete all historical records
*/
void clearRecordsInStorage();

/*
Save the absolute zero-point offset value to persistent storage.
This stores the sensor calibration offset for use during the next startup,
ensuring consistent measurements across power cycles.

Parameters:
  offset (long): The raw offset value from the sensor (typically obtained from captureAbsoluteOffset())

Returns:
  void

Example:
  saveAbsoluteOffset(123456);  // Store calibration offset
*/
void saveAbsoluteOffset(long offset);

/*
Retrieve the previously saved absolute zero-point offset from storage.
Returns the stored calibration offset, or 0 if no offset has been saved.
This value is used to initialize the sensor during startup.

Parameters:
  none

Returns:
  long: The stored offset value, or 0 if not previously set

Example:
  long offset = getAbsoluteOffset();  // Returns stored calibration offset
*/
long getAbsoluteOffset();

/*
Save the absolute zero-point offset for sensor 2 to persistent storage.
*/
void saveAbsoluteOffset2(long offset);

/*
Retrieve the saved absolute zero-point offset for sensor 2.
Returns 0 if not previously set.
*/
long getAbsoluteOffset2();