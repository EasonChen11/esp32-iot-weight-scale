#include "config.h"
#if WIFI_ENABLED
#include <WiFi.h>
#include "wifi_manager.h"

/*
Initialize WiFi in AP+STA mode so both interfaces are active simultaneously.

Connection flow:
  1. Start Soft-AP immediately (AP_WIFI_SSID / AP_WIFI_PASS).
     → Web server is reachable at the AP gateway IP (default 192.168.4.1)
       by any phone or device that connects to the ESP32 hotspot.
  2. Attempt to join STA_WIFI_SSID for up to 15 seconds.
     → On success: MQTT broker becomes reachable via the STA DHCP IP.
     → On failure: AP remains up; web server still works; MQTT unavailable.

Parameters:
  none

Returns:
  void
*/
void initWiFi()
{
    // Run both AP and STA interfaces concurrently
    WiFi.mode(WIFI_AP_STA);

    // ── AP interface (web server for phones / local devices) ──────────
    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS);
    Serial.printf("[WiFi] AP started: SSID='%s'  IP: %s\n",
                  AP_WIFI_SSID, WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFi] Web interface (AP): http://%s\n",
                  WiFi.softAPIP().toString().c_str());

    // ── STA interface (home network → MQTT broker) ────────────────────
    WiFi.begin(STA_WIFI_SSID, STA_WIFI_PASS);
    Serial.printf("[WiFi] Connecting to '%s' for MQTT", STA_WIFI_SSID);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) // 30 × 500 ms = 15 s
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\n[WiFi] STA connected! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] MQTT broker: %s:%d\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);
    }
    else
    {
        Serial.println("\n[WiFi] STA connection failed — MQTT unavailable.");
        Serial.println("[WiFi] Web server still accessible via AP.");
    }
}

#if NTP_ENABLED
#include <time.h>
#include "esp_sntp.h"

static bool _timeSynced = false;

bool isTimeSynced()
{
    return _timeSynced;
}

void setTimeSynced(bool synced)
{
    _timeSynced = synced;
}

void initNTP()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] Skipped — no STA connection");
        return;
    }

    // Log old RTC time (kept as fallback if NTP fails)
    struct tm timeinfo;
    if (getLocalTime(&timeinfo)) {
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] RTC time before sync: %s\n", buf);
    } else {
        Serial.println("[NTP] No RTC time available");
    }

    // 重置同步狀態，避免深度睡眠醒來後繼承上次 boot 的舊 COMPLETED 狀態
    sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);

    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2);
    Serial.print("[NTP] Synchronizing");

    // 用 sntp_get_sync_status() 取代 getLocalTime() 來偵測同步
    // getLocalTime() 在深度睡眠後會因 RTC 舊時間立刻回傳 true（假陽性）
    // sntp_get_sync_status() 只有收到真正 NTP 回應才回傳 COMPLETED
    int retries = 0;
    while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED && retries < 20) {
        delay(500);
        Serial.print(".");
        retries++;
    }

    if (sntp_get_sync_status() == SNTP_SYNC_STATUS_COMPLETED) {
        _timeSynced = true;
        getLocalTime(&timeinfo);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("\n[NTP] Time synced: %s\n", buf);
    } else {
        _timeSynced = false;
        Serial.println("\n[NTP] Sync failed — RTC time preserved as fallback");
    }
}
#endif // NTP_ENABLED

#endif // WIFI_ENABLED
