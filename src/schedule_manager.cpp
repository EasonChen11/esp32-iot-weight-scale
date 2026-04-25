#include "config.h"
#if SCHEDULE_ENABLED
#include "schedule_manager.h"
#include <Preferences.h>

static ScheduleEntry entries[MAX_SCHEDULE_ENTRIES];
static int entryCount = 0;

/* ── Internal helpers ─────────────────────────────────────────────────── */

static void _sortEntries()
{
    // Simple insertion sort (max 10 entries)
    for (int i = 1; i < entryCount; i++) {
        ScheduleEntry key = entries[i];
        int j = i - 1;
        while (j >= 0 && (entries[j].hour > key.hour ||
               (entries[j].hour == key.hour && entries[j].minute > key.minute))) {
            entries[j + 1] = entries[j];
            j--;
        }
        entries[j + 1] = key;
    }
}

static void _saveToNVS()
{
    String csv = "";
    for (int i = 0; i < entryCount; i++) {
        if (i > 0) csv += ",";
        char buf[6];
        snprintf(buf, sizeof(buf), "%02d:%02d", entries[i].hour, entries[i].minute);
        csv += buf;
    }

    Preferences prefs;
    prefs.begin("schedule", false);
    prefs.putString("times", csv);
    prefs.end();

    Serial.printf("[Schedule] Saved %d entries: %s\n", entryCount, csv.c_str());
}

static void _loadFromNVS()
{
    Preferences prefs;
    prefs.begin("schedule", true);
    String csv = prefs.getString("times", "");
    prefs.end();

    entryCount = 0;
    if (csv.length() == 0) return;

    int start = 0;
    while (start < (int)csv.length() && entryCount < MAX_SCHEDULE_ENTRIES) {
        int comma = csv.indexOf(',', start);
        String token = (comma == -1) ? csv.substring(start) : csv.substring(start, comma);

        int colon = token.indexOf(':');
        if (colon > 0) {
            int h = token.substring(0, colon).toInt();
            int m = token.substring(colon + 1).toInt();
            if (h >= 0 && h <= 23 && m >= 0 && m <= 59) {
                entries[entryCount].hour = h;
                entries[entryCount].minute = m;
                entryCount++;
            }
        }

        if (comma == -1) break;
        start = comma + 1;
    }

    _sortEntries();
}

/* ── Public API ───────────────────────────────────────────────────────── */

void initSchedule()
{
    _loadFromNVS();
    Serial.printf("[Schedule] Loaded %d wake-up entries\n", entryCount);
}

int getScheduleCount()
{
    return entryCount;
}

bool addScheduleEntry(uint8_t hour, uint8_t minute)
{
    if (hour > 23 || minute > 59) return false;
    if (entryCount >= MAX_SCHEDULE_ENTRIES) return false;

    // Check for duplicate
    for (int i = 0; i < entryCount; i++) {
        if (entries[i].hour == hour && entries[i].minute == minute) return false;
    }

    entries[entryCount].hour = hour;
    entries[entryCount].minute = minute;
    entryCount++;
    _sortEntries();
    _saveToNVS();
    return true;
}

bool removeScheduleEntry(int index)
{
    if (index < 0 || index >= entryCount) return false;

    for (int i = index; i < entryCount - 1; i++) {
        entries[i] = entries[i + 1];
    }
    entryCount--;
    _saveToNVS();
    return true;
}

void clearAllScheduleEntries()
{
    // Wipe both the in-memory list and the NVS-persisted copy
    // by repeatedly calling removeScheduleEntry on the head until empty.
    while (getScheduleCount() > 0) {
        removeScheduleEntry(0);
    }
    Serial.println("[Schedule] All entries cleared");
}

String getScheduleJson()
{
    String json = "[";
    for (int i = 0; i < entryCount; i++) {
        if (i > 0) json += ",";
        json += "{\"h\":" + String(entries[i].hour) +
                ",\"m\":" + String(entries[i].minute) + "}";
    }
    json += "]";
    return json;
}

int getNextWakeupSeconds()
{
    if (entryCount == 0) return -1;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return -1;

    int nowSec = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
    int bestDelta = -1;

    for (int i = 0; i < entryCount; i++) {
        int targetSec = entries[i].hour * 3600 + entries[i].minute * 60;
        int delta = targetSec - nowSec;
        if (delta <= 0) delta += 86400;   // wrap to tomorrow

        if (bestDelta < 0 || delta < bestDelta) {
            bestDelta = delta;
        }
    }

    return bestDelta;
}

#endif // SCHEDULE_ENABLED
