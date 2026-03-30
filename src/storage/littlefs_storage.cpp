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

  // Detect old format (no "id" key) and clear for migration
  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, cachedRecordsJson);
  if (!err && doc.is<JsonArray>() && doc.as<JsonArray>().size() > 0)
  {
    JsonObject first = doc.as<JsonArray>()[0].as<JsonObject>();
    if (!first["id"].is<long>())
    {
      Serial.println("[Storage] Old format detected -- records cleared for migration");
      cachedRecordsJson = "[]";
      File f = LittleFS.open(FILE_PATH, "w");
      f.print("[]");
      f.close();
      return;
    }
  }

  Serial.printf("[Storage] Records cache loaded (%d bytes)\n", cachedRecordsJson.length());
}

String getRecordsJson()
{
  return cachedRecordsJson;
}

void addRecordToStorage(long id, String date, String time, String sensor1, String sensor2)
{
  JsonDocument doc;
  deserializeJson(doc, cachedRecordsJson);
  JsonArray array = doc.as<JsonArray>();

  JsonDocument newEntryDoc;
  JsonObject newEntry = newEntryDoc.to<JsonObject>();
  newEntry["id"] = id;
  newEntry["date"] = date;
  newEntry["time"] = time;
  newEntry["sensor1"] = sensor1;
  newEntry["sensor2"] = sensor2;
  newEntry["synced"] = false;

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

void markRecordSynced(long id)
{
  JsonDocument doc;
  deserializeJson(doc, cachedRecordsJson);
  JsonArray array = doc.as<JsonArray>();

  bool found = false;
  for (JsonVariant v : array)
  {
    if (v["id"].as<long>() == id)
    {
      v["synced"] = true;
      found = true;
      break;
    }
  }

  if (found)
  {
    saveJsonToFile(doc);
    updateCache(doc);
  }
  else
  {
    Serial.printf("[Storage] Warning: record ID %ld not found for sync mark\n", id);
  }
}

String getUnsyncedRecordsJson()
{
  JsonDocument doc;
  deserializeJson(doc, cachedRecordsJson);
  JsonArray array = doc.as<JsonArray>();

  JsonDocument filterDoc;
  JsonArray filtered = filterDoc.to<JsonArray>();

  for (JsonVariant v : array)
  {
    if (!v["synced"].as<bool>())
    {
      filtered.add(v);
    }
  }

  String result;
  serializeJson(filterDoc, result);
  return result;
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
