#pragma once
#include "config.h"
#if SCHEDULE_ENABLED
#include <Arduino.h>

struct ScheduleEntry {
    uint8_t hour;    // 0-23
    uint8_t minute;  // 0-59
};

void   initSchedule();
int    getScheduleCount();
bool   addScheduleEntry(uint8_t hour, uint8_t minute);
bool   removeScheduleEntry(int index);
void   clearAllScheduleEntries();
String getScheduleJson();
int    getNextWakeupSeconds();   // seconds until next scheduled time (-1 if none)

#endif // SCHEDULE_ENABLED
