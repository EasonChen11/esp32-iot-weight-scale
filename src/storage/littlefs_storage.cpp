#include "storage/littlefs_storage.h"
#include "config.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

static const char *FILE_PATH = "/records.json";
static String cachedRecordsJson = "[]";

static void saveJsonToFile(JsonDocument &doc)
{
  File file = LittleFS.open(FILE_PATH, "w");
  serializeJson(doc, file);
  file.close();
}

static void updateCache(JsonDocument &doc)
{
  cachedRecordsJson = "";
  serializeJson(doc, cachedRecordsJson);
}

void loadRecordsCache()
{
  File file = LittleFS.open(FILE_PATH, "r");
  if (!file || file.size() == 0)
  {
    cachedRecordsJson = "[]";
    return;
  }
  cachedRecordsJson = file.readString();
  file.close();
  Serial.printf("[Storage] Records cache loaded (%d bytes)\n", cachedRecordsJson.length());
}

String getRecordsJson()
{
  return cachedRecordsJson;
}

void addRecordToStorage(String time, String weight)
{
  JsonDocument doc;
  deserializeJson(doc, cachedRecordsJson);
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
  updateCache(nextDoc);
}

void deleteRecordFromStorage(int index)
{
  JsonDocument doc;
  deserializeJson(doc, cachedRecordsJson);
  JsonArray array = doc.as<JsonArray>();
  array.remove(index);
  saveJsonToFile(doc);
  updateCache(doc);
}

void clearRecordsInStorage()
{
  JsonDocument doc;
  doc.to<JsonArray>();
  saveJsonToFile(doc);
  cachedRecordsJson = "[]";
}
