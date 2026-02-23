#include "storage_manager.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <Preferences.h>

const char *FILE_PATH = "/records.json";

/*
Internal helper function to save a JSON document to the storage file.
This is a private function used by other storage functions to persist data.

Parameters:
  doc (JsonDocument&): The JSON document to save

Returns:
  void
*/
static void saveJsonToFile(JsonDocument &doc)
{
  File file = LittleFS.open(FILE_PATH, "w");
  serializeJson(doc, file);
  file.close();
}

/*
Initialize the SPIFFS (SPI Flash File System) for data storage.
This function must be called once during setup() to mount the filesystem
and prepare for reading/writing persistent data.

Parameters:
  none

Returns:
  void
*/
void initStorage()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS mount failed");
    return;
  }
}

/*
Retrieve all stored weight records as a JSON string.
Returns a JSON array containing all stored measurement records, or an empty
array "[]" if no records exist or if the file cannot be read.

Parameters:
  none

Returns:
  String: JSON array of records
*/
String getRecordsJson()
{
  File file = LittleFS.open(FILE_PATH, "r");
  if (!file || file.size() == 0)
    return "[]";
  String content = file.readString();
  file.close();
  return content;
}

/*
Add a new weight measurement record to persistent storage.
This function inserts a new record at the beginning of the records array
and maintains a maximum of 10 recent records by removing the oldest entries.

Parameters:
  time (String): Timestamp string for the measurement (e.g., "10:30:45")
  weight (String): Weight value as a string (e.g., "2.45")

Returns:
  void
*/
void addRecordToStorage(String time, String weight)
{
  JsonDocument doc;
  deserializeJson(doc, getRecordsJson());
  JsonArray array = doc.as<JsonArray>();

  JsonDocument newEntryDoc;
  JsonObject newEntry = newEntryDoc.to<JsonObject>();
  newEntry["time"] = time;
  newEntry["weight"] = weight;

  JsonDocument nextDoc;
  JsonArray nextArray = nextDoc.to<JsonArray>();
  nextArray.add(newEntry);
  for (JsonVariant v : array)
  {
    if (nextArray.size() < MAX_RECORDS)
      nextArray.add(v);
  }
  saveJsonToFile(nextDoc);
}

/*
Delete a specific weight record by index from persistent storage.
Removes the record at the given index position from the records array.

Parameters:
  index (int): Zero-based index of the record to delete

Returns:
  void
*/
void deleteRecordFromStorage(int index)
{
  JsonDocument doc;
  deserializeJson(doc, getRecordsJson());
  JsonArray array = doc.as<JsonArray>();
  array.remove(index);
  saveJsonToFile(doc);
}

/*
Clear all weight records from persistent storage.
Deletes all measurement records and leaves an empty array in storage.

Parameters:
  none

Returns:
  void
*/
void clearRecordsInStorage()
{
  JsonDocument doc;
  doc.to<JsonArray>();
  saveJsonToFile(doc);
}

/*
Save the absolute zero-point offset value to persistent storage.
This stores the sensor calibration offset for use during the next startup,
ensuring consistent measurements across power cycles.

Parameters:
  offset (long): The raw offset value from the sensor

Returns:
  void
*/
void saveAbsoluteOffset(long offset)
{
  Preferences preferences;
  // Open namespace "scale_data"; false = read/write mode
  preferences.begin("scale_data", false);
  preferences.putLong("offset", offset);
  preferences.end();

  Serial.printf("[Storage] Absolute offset saved: %ld\n", offset);
}

/*
Retrieve the previously saved absolute zero-point offset from storage.
Returns the stored calibration offset, or 0 if no offset has been saved.
This value is used to initialize the sensor during startup.

Parameters:
  none

Returns:
  long: The stored offset value, or 0 if not previously set
*/
long getAbsoluteOffset()
{
  Preferences preferences;
  // true = open in read-only mode
  preferences.begin("scale_data", true);
  long offset = preferences.getLong("offset", 0);
  preferences.end();

  return offset;
}