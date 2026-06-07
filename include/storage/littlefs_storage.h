#pragma once
#include <Arduino.h>

void loadRecordsCache();
String getRecordsJson();
void addRecordToStorage(long id, const String &date, const String &time, const String &sensor1, const String &sensor2);
void markRecordSynced(long id);
String getUnsyncedRecordsJson();
void deleteRecordFromStorage(int index);
void clearRecordsInStorage();
