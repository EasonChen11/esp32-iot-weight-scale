#include "config.h"
#if AUTO_LOGGER_ENABLED
#include "auto_logger.h"
#include "sensor_manager.h"
#include "storage/littlefs_storage.h"
#include "storage/nvs_storage.h"
#if NTP_ENABLED
#include "wifi_manager.h"
#endif
#include <time.h>

// State tracking for logging events
static unsigned long bootTime = 0;
static bool startupRecordDone = false;

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

static String getLogDate()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        return "unknown";
    }
    char buf[12];
    strftime(buf, sizeof(buf), "%Y-%m-%d", &timeinfo);
    return String(buf);
}

void initAutoLogger()
{
    bootTime = millis();
    startupRecordDone = false;
    Serial.println("[AutoLogger] Service initialized. Waiting for stability and time sync...");
}

bool isStartupRecordDone()
{
    return startupRecordDone;
}

void handleAutoLogging()
{
    unsigned long currentMillis = millis();
    struct tm timeinfo;
    bool hasTime = getLocalTime(&timeinfo);
#if NTP_ENABLED
    // NTP 成功 → 可靠；NTP 失敗但 RTC 有舊時間（深度睡眠保留）→ 也接受
    bool timeReliable = (hasTime && isTimeSynced()) || (hasTime && timeinfo.tm_year > (2024 - 1900));
#else
    bool timeReliable = hasTime;
#endif

    // --- Logic A: Initial startup record ---
    if (!startupRecordDone && (currentMillis - bootTime >= STARTUP_RECORD_DELAY_MS))
    {
        bool timedOut = (currentMillis - bootTime >= TIME_SYNC_TIMEOUT_MS);

        if (timeReliable || timedOut)
        {
            long id = getNextRecordId();
            String dateStr = timeReliable ? getLogDate() : "no-sync";
            String timeStr = timeReliable ? getLogTimestamp() : "no-sync";
            String s1Str = String(getCachedWeight1(), 3);
            String s2Str = String(getCachedWeight2(), 3);

            addRecordToStorage(id, dateStr, timeStr, s1Str, s2Str);

            startupRecordDone = true;
            if (timedOut && !timeReliable)
            {
                Serial.printf("[AutoLogger] Time sync timeout — record saved without time (ID=%ld)\n", id);
            }
            else
            {
                Serial.printf("[AutoLogger] Initial record saved (ID=%ld, %s %s): S1=%.3f S2=%.3f kg\n",
                              id, dateStr.c_str(), timeStr.c_str(),
                              getCachedWeight1(), getCachedWeight2());
            }
        }
        else
        {
            static unsigned long lastWaitMsg = 0;
            if (currentMillis - lastWaitMsg > 5000)
            {
                Serial.println("[AutoLogger] Delay passed. Waiting for time sync to save initial record...");
                lastWaitMsg = currentMillis;
            }
        }
    }

    // --- Logic B: Hourly logging (disabled in deep sleep mode) ---
#if !DEEP_SLEEP_ENABLED
    // Last hour we logged. Declared here because hourly logging only exists
    // when deep sleep is disabled — keeps it out of the build otherwise.
    static int lastRecordedHour = -1;
    if (timeReliable)
    {
        if (timeinfo.tm_hour != lastRecordedHour)
        {
            if (lastRecordedHour == -1)
            {
                lastRecordedHour = timeinfo.tm_hour;
                return;
            }

            lastRecordedHour = timeinfo.tm_hour;

            long id = getNextRecordId();
            String dateStr = getLogDate();
            String timeStr = getLogTimestamp();
            String s1Str = String(getCachedWeight1(), 3);
            String s2Str = String(getCachedWeight2(), 3);

            addRecordToStorage(id, dateStr, timeStr, s1Str, s2Str);
            Serial.printf("[AutoLogger] Hourly record saved (ID=%ld, %s): S1=%.3f S2=%.3f kg\n",
                          id, timeStr.c_str(), getCachedWeight1(), getCachedWeight2());
        }
    }
#endif
}

#endif // AUTO_LOGGER_ENABLED
