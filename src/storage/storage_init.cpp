#include "storage/storage_init.h"
#include "storage/littlefs_storage.h"
#include <LittleFS.h>
#include <Arduino.h>

void initStorage()
{
  if (!LittleFS.begin(true))
  {
    Serial.println("LittleFS mount failed");
    return;
  }
  loadRecordsCache();
}
