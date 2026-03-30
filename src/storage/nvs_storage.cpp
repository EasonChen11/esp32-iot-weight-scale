#include "storage/nvs_storage.h"
#include <Preferences.h>
#include <Arduino.h>

void saveAbsoluteOffset(long offset)
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  preferences.putLong("offset", offset);
  preferences.end();

  Serial.printf("[Storage] Sensor 1 absolute offset saved: %ld\n", offset);
}

void saveAbsoluteOffset2(long offset)
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  preferences.putLong("offset2", offset);
  preferences.end();

  Serial.printf("[Storage] Sensor 2 absolute offset saved: %ld\n", offset);
}

long getAbsoluteOffset()
{
  Preferences preferences;
  preferences.begin("scale_data", true);
  long offset = preferences.getLong("offset", 0);
  preferences.end();

  return offset;
}

long getAbsoluteOffset2()
{
  Preferences preferences;
  preferences.begin("scale_data", true);
  long offset = preferences.getLong("offset2", 0);
  preferences.end();

  return offset;
}

long getNextRecordId()
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  long id = preferences.getLong("record_id", 0) + 1;
  preferences.putLong("record_id", id);
  preferences.end();

  return id;
}
