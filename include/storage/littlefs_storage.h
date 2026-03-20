#pragma once
#include <Arduino.h>

void loadRecordsCache();
String getRecordsJson();
void addRecordToStorage(String time, String weight);
void deleteRecordFromStorage(int index);
void clearRecordsInStorage();
