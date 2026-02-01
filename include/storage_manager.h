#pragma once
#include <Arduino.h>

void initStorage();
String getRecordsJson();
void addRecordToStorage(String time, String weight);
void deleteRecordFromStorage(int index);