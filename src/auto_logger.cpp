#include "auto_logger.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "config.h"
#include <time.h>

// State tracking for logging events
static int lastRecordedHour = -1;
static unsigned long bootTime = 0;
static bool startupRecordDone = false;

/*
getLogTimestamp

Return a formatted timestamp string. If the ESP32 has not synchronized
time via NTP or Web API, return a boot-relative label instead.

Returns:
    String: "HH:MM:SS" or "Boot-<seconds>s"
*/
String getLogTimestamp()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "Boot-" + String(millis() / 1000) + "s";
    }
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    return String(buf);
}

/*
initAutoLogger

Initialize the automatic logging subsystem and record the boot timestamp.
*/
void initAutoLogger()
{
    bootTime = millis();
    startupRecordDone = false;
    Serial.println("[AutoLogger] Service initialized. Waiting for stability and time sync...");
}

/*
handleAutoLogging

Handles scheduled tasks on Core 0.
1. Startup Record: Waits for STARTUP_RECORD_DELAY_MS AND valid system time.
2. Hourly Record: Detects hour transitions and saves a measurement.
*/
void handleAutoLogging()
{
    unsigned long currentMillis = millis();
    struct tm timeinfo;
    bool isTimeSynced = getLocalTime(&timeinfo);

    // --- Logic A: Initial startup record ---
    // Condition: Delay has passed AND system time is synchronized
    if (!startupRecordDone && (currentMillis - bootTime >= STARTUP_RECORD_DELAY_MS))
    {
        if (isTimeSynced)
        {
            float weight = getCachedWeight();
            String timeStr = getLogTimestamp();

            addRecordToStorage(timeStr, String(weight, 3));

            startupRecordDone = true;
            Serial.printf("[AutoLogger] Initial record saved with synced time (%s): %.3f kg\n", timeStr.c_str(), weight);
        }
        else
        {
            // Optional: Periodic reminder that sync is pending
            static unsigned long lastWaitMsg = 0;
            if (currentMillis - lastWaitMsg > 5000)
            {
                Serial.println("[AutoLogger] Delay passed. Waiting for time sync to save initial record...");
                lastWaitMsg = currentMillis;
            }
        }
    }

    // --- Logic B: Hourly logging ---
    if (isTimeSynced)
    {
        if (timeinfo.tm_hour != lastRecordedHour)
        {
            // Calibrate hour tracker on first valid sync
            if (lastRecordedHour == -1)
            {
                lastRecordedHour = timeinfo.tm_hour;
                return;
            }

            lastRecordedHour = timeinfo.tm_hour;
            float weight = getCachedWeight();
            String timeStr = getLogTimestamp();

            addRecordToStorage(timeStr, String(weight, 3));
            Serial.printf("[AutoLogger] Hourly record saved (%s): %.3f kg\n", timeStr.c_str(), weight);
        }
    }
}