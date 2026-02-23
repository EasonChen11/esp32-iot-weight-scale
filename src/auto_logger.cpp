#include "auto_logger.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include "config.h"
#include <time.h>

// 狀態追蹤
static int lastRecordedHour = -1;
static unsigned long bootTime = 0;
static bool startupRecordDone = false;

/**
 * 取得格式化時間，若未同步則回傳開機後的相對時間
 */
String getLogTimestamp()
{
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo))
    {
        // 如果 10 秒到的時候還沒同步時間，就回傳開機計時標籤
        return "Boot-" + String(millis() / 1000) + "s";
    }
    char buf[20];
    strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
    return String(buf);
}

void initAutoLogger()
{
    bootTime = millis();
    startupRecordDone = false;
    Serial.println("[AutoLogger] 服務已啟動：10秒後執行初始紀錄，之後每整點紀錄。");
}

void handleAutoLogging()
{
    unsigned long currentMillis = millis();

    // --- 邏輯 A：開機 10 秒初始紀錄 ---
    if (!startupRecordDone && (currentMillis - bootTime >= STARTUP_RECORD_DELAY_MS))
    {
        float weight = getCachedWeight();
        String timeStr = getLogTimestamp();

        addRecordToStorage(timeStr, String(weight, 3));

        startupRecordDone = true;
        Serial.printf("[AutoLogger] 啟動初始紀錄成功 (%s): %.3f kg\n", timeStr.c_str(), weight);
        // 執行完 A 之後，本圈 loop 繼續往下檢查 B
    }

    // --- 邏輯 B：每小時整點紀錄 ---
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
        // 只有當小時數改變時才觸發
        if (timeinfo.tm_hour != lastRecordedHour)
        {

            // 初次同步時間時的處理
            if (lastRecordedHour == -1)
            {
                lastRecordedHour = timeinfo.tm_hour;
                return; // 初次同步先不紀錄，除非你希望立刻補一筆
            }

            lastRecordedHour = timeinfo.tm_hour;

            float weight = getCachedWeight();
            String timeStr = getLogTimestamp();

            // 存入 LittleFS (這是在 Core 0 執行，不會卡到 Core 1 的感測器)
            addRecordToStorage(timeStr, String(weight, 3));

            Serial.printf("[AutoLogger] 整點紀錄成功 (%s): %.3f kg\n", timeStr.c_str(), weight);
        }
    }
}