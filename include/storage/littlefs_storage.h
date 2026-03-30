#pragma once
#include <Arduino.h>

void loadRecordsCache();
String getRecordsJson();
void addRecordToStorage(long id, String date, String time, String sensor1, String sensor2);
void markRecordSynced(long id);
String getUnsyncedRecordsJson();
void deleteRecordFromStorage(int index);
void clearRecordsInStorage();
