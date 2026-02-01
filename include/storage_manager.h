#pragma once
#include <Arduino.h>

void initStorage();
String getRecordsJson();
// record data management
void addRecordToStorage(String time, String weight);
void deleteRecordFromStorage(int index);
void clearRecordsInStorage();

// record offset management
void saveAbsoluteOffset(long offset);
long getAbsoluteOffset();