#include "storage_manager.h"
#include <LittleFS.h>
#include <ArduinoJson.h>

const char *FILE_PATH = "/records.json";

void initStorage()
{
    if (!LittleFS.begin(true))
    {
        Serial.println("LittleFS Mount Failed");
        return;
    }
}

String getRecordsJson()
{
    File file = LittleFS.open(FILE_PATH, "r");
    if (!file || file.size() == 0)
        return "[]";
    String content = file.readString();
    file.close();
    return content;
}

void saveJsonToFile(JsonDocument &doc)
{
    File file = LittleFS.open(FILE_PATH, "w");
    serializeJson(doc, file);
    file.close();
}

void addRecordToStorage(String time, String weight)
{
    JsonDocument doc;
    deserializeJson(doc, getRecordsJson());
    JsonArray array = doc.as<JsonArray>();

    // 建立新紀錄並插入到最前面 (unshift)
    JsonDocument newEntryDoc;
    JsonObject newEntry = newEntryDoc.to<JsonObject>();
    newEntry["time"] = time;
    newEntry["weight"] = weight;

    // 手動模擬 unshift：建立新陣列並複製
    JsonDocument nextDoc;
    JsonArray nextArray = nextDoc.to<JsonArray>();
    nextArray.add(newEntry);
    for (JsonVariant v : array)
    {
        if (nextArray.size() < 10)
            nextArray.add(v);
    }
    saveJsonToFile(nextDoc);
}

void deleteRecordFromStorage(int index)
{
    JsonDocument doc;
    deserializeJson(doc, getRecordsJson());
    JsonArray array = doc.as<JsonArray>();
    array.remove(index);
    saveJsonToFile(doc);
}