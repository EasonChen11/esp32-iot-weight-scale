#include "auto_logger.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "config.h"
#include <time.h>

// State tracking
static int lastRecordedHour = -1;
static unsigned long bootTime = 0;
static bool startupRecordDone = false;

/*
getLogTimestamp

Return a formatted timestamp string. If the ESP32 has not synchronized
time via NTP, return a boot-relative label instead.

Returns:
  String: "HH:MM:SS" or "Boot-<seconds>s" when time is not available
*/
String getLogTimestamp()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // If time is not yet synchronized, return a boot-relative timestamp
        return "Boot-" + String(millis() / 1000) + "s";
    }
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    return String(buf);
}

/*
initAutoLogger

Initialize the automatic logging subsystem. This schedules an initial
startup record after `STARTUP_RECORD_DELAY_MS` and enables hourly logging.

Parameters:
  none

Returns:
  void
*/
void initAutoLogger()
{
    bootTime = millis();
    startupRecordDone = false;
    Serial.println("[AutoLogger] Service started: initial record after 10s, then hourly.");
}

/*
handleAutoLogging

Perform automatic logging tasks. There are two behaviors:
A) Create an initial record after boot delay.
B) Record one measurement at each hour change.

Parameters:
  none

Returns:
  void
*/
void handleAutoLogging()
{
    unsigned long currentMillis = millis();

    // --- Logic A: Startup initial record after delay ---
    if (!startupRecordDone && (currentMillis - bootTime >= STARTUP_RECORD_DELAY_MS))
    {
        float weight = getCachedWeight();
        String timeStr = getLogTimestamp();

        addRecordToStorage(timeStr, String(weight, 3));

        startupRecordDone = true;
        Serial.printf("[AutoLogger] Initial startup record saved (%s): %.3f kg\n", timeStr.c_str(), weight);
        // After A completes, continue to check logic B
    }

    // --- Logic B: Hourly record on hour change ---
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        // Trigger only when the hour changes
        if (timeinfo.tm_hour != lastRecordedHour)
        {

            // Handle first time synchronization
            if (lastRecordedHour == -1)
            {
                lastRecordedHour = timeinfo.tm_hour;
                return; // Do not record immediately on first sync unless desired
            }

            lastRecordedHour = timeinfo.tm_hour;

            float weight = getCachedWeight();
            String timeStr = getLogTimestamp();

            // Save to LittleFS (runs on Core 0, does not block Core 1 sensor)
            addRecordToStorage(timeStr, String(weight, 3));

            Serial.printf("[AutoLogger] Hourly record saved (%s): %.3f kg\n", timeStr.c_str(), weight);
        }
    }
}