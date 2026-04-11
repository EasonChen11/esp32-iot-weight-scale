#include "config.h"
#if WIFI_ENABLED
#include <WiFi.h>
#include "wifi_manager.h"
#if WIFI_CONFIG_ENABLED
#include "storage/nvs_storage.h"
#endif

// ── Runtime WiFi state (single writer: Core 0 web/task loop) ────────
static volatile WifiStatus    g_wifiStatus     = WIFI_STATUS_DISCONNECTED;
#if WIFI_CONFIG_ENABLED
static volatile bool          g_wifiOpBusy     = false;
static volatile unsigned long g_connectStartMs = 0;
static volatile bool          g_pendingNtpSync = false;
static String                 g_currentSsid;
static String                 g_pendingSsid;
static String                 g_pendingPass;
#endif

/*
Attempt to join the given SSID for up to timeoutMs. Returns true on success.
Polls every 100 ms; uses millis() rather than a fixed loop count so the timeout
is honoured precisely.
*/
static bool connectToStation(const char *ssid, const char *pass, unsigned long timeoutMs)
{
    WiFi.begin(ssid, pass);
    unsigned long startMs = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (millis() - startMs >= timeoutMs) {
            return false;
        }
        delay(100);
        Serial.print(".");
    }
    return true;
}

/*
Initialize WiFi in AP+STA mode so both interfaces are active simultaneously.

Connection flow:
  1. Start Soft-AP immediately (AP_WIFI_SSID / AP_WIFI_PASS).
     → Web server is reachable at the AP gateway IP (default 192.168.4.1)
       by any phone or device that connects to the ESP32 hotspot.
  2. If WIFI_CONFIG_ENABLED: try NVS-stored SSID/pass for WIFI_CONNECT_TIMEOUT_MS.
  3. Fall back to compile-time STA_WIFI_SSID for WIFI_CONNECT_TIMEOUT_MS.
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

    // ── AP interface (always on) ───────────────────────────────────────
    WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS);
    Serial.printf("[WiFi] AP started: SSID='%s'  IP: %s\n",
                  AP_WIFI_SSID, WiFi.softAPIP().toString().c_str());
    Serial.printf("[WiFi] Web interface (AP): http://%s\n",
                  WiFi.softAPIP().toString().c_str());

#if WIFI_CONFIG_ENABLED
    // ── Step 1: try NVS-stored credentials ─────────────────────────────
    if (hasStoredCredentials()) {
        String nvsSsid, nvsPass;
        getStaCredentials(nvsSsid, nvsPass);
        Serial.printf("[WiFi] Trying stored SSID: '%s'", nvsSsid.c_str());
        if (connectToStation(nvsSsid.c_str(), nvsPass.c_str(), WIFI_CONNECT_TIMEOUT_MS)) {
            g_wifiStatus  = WIFI_STATUS_CONNECTED;
            g_currentSsid = nvsSsid;
            Serial.printf("\n[WiFi] STA connected via NVS! IP: %s\n",
                          WiFi.localIP().toString().c_str());
            return;
        }
        Serial.println("\n[WiFi] Stored SSID timeout, falling back to compile-time");
    } else {
        Serial.println("[WiFi] No stored credentials, trying compile-time");
    }
#endif

    // ── Step 2: compile-time fallback ──────────────────────────────────
    Serial.printf("[WiFi] Trying compile-time SSID: '%s'", STA_WIFI_SSID);
    if (connectToStation(STA_WIFI_SSID, STA_WIFI_PASS, WIFI_CONNECT_TIMEOUT_MS)) {
        g_wifiStatus = WIFI_STATUS_CONNECTED;
#if WIFI_CONFIG_ENABLED
        g_currentSsid = STA_WIFI_SSID;
#endif
        Serial.printf("\n[WiFi] STA connected via compile-time! IP: %s\n",
                      WiFi.localIP().toString().c_str());
#if MQTT_ENABLED
        Serial.printf("[WiFi] MQTT broker: %s:%d\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);
#endif
        return;
    }

    Serial.println("\n[WiFi] Boot fallback exhausted — AP only");
    g_wifiStatus = WIFI_STATUS_DISCONNECTED;
}

#if WIFI_CONFIG_ENABLED

String getCurrentSsid()
{
    return g_currentSsid;
}

/*
Validate inputs and kick off a runtime STA switch. Non-blocking: returns true
if the attempt is now in progress; false (with error message) if rejected.
NVS is NOT written here — only after processWifiTasks() observes success.
*/
bool requestStaChange(const String &ssid, const String &pass, String &errorOut)
{
    if (g_wifiOpBusy) {
        errorOut = "busy";
        return false;
    }
    if (ssid.length() == 0) {
        errorOut = "SSID required";
        return false;
    }
    if (pass.length() > 0 && (pass.length() < 8 || pass.length() > 63)) {
        errorOut = "Password must be 8-63 characters";
        return false;
    }

    g_wifiOpBusy     = true;
    g_pendingSsid    = ssid;
    g_pendingPass    = pass;
    g_connectStartMs = millis();
    g_wifiStatus     = WIFI_STATUS_CONNECTING;

    WiFi.disconnect(false, true);  // keep AP up; stop current STA connection
    WiFi.begin(ssid.c_str(), pass.c_str());
    Serial.printf("[WiFi] Runtime switch requested: '%s'\n", ssid.c_str());
    return true;
}

/*
State machine driver — call from the WebAndTasks loop on every iteration.
Detects CONNECTING → CONNECTED transition (writes NVS, triggers NTP re-sync),
CONNECTING → FAILED transition (timeout), and router-side
disconnect/reconnect for the heartbeat indicator.
*/
void processWifiTasks()
{
    // Runtime CONNECTING → CONNECTED (success)
    if (g_wifiStatus == WIFI_STATUS_CONNECTING && WiFi.status() == WL_CONNECTED) {
        saveStaCredentials(g_pendingSsid, g_pendingPass);  // NVS write only on success
        g_currentSsid    = g_pendingSsid;
        g_pendingSsid    = "";
        g_pendingPass    = "";
        g_wifiStatus     = WIFI_STATUS_CONNECTED;
        g_wifiOpBusy     = false;
        g_pendingNtpSync = true;
        Serial.printf("[WiFi] Runtime connect succeeded: '%s' @ %s\n",
                      g_currentSsid.c_str(), WiFi.localIP().toString().c_str());
    }
    // Runtime CONNECTING → FAILED (timeout)
    else if (g_wifiStatus == WIFI_STATUS_CONNECTING &&
             millis() - g_connectStartMs > WIFI_CONNECT_TIMEOUT_MS) {
        g_pendingSsid = "";
        g_pendingPass = "";
        g_wifiStatus  = WIFI_STATUS_FAILED;
        g_wifiOpBusy  = false;
        Serial.println("[WiFi] Runtime connect failed (timeout)");
    }

    // Router-side disconnect detection
    if (g_wifiStatus == WIFI_STATUS_CONNECTED && WiFi.status() != WL_CONNECTED) {
        g_wifiStatus = WIFI_STATUS_DISCONNECTED;
        Serial.println("[WiFi] STA link lost (router/AP went away?)");
    }
    // Auto-recovery
    else if (g_wifiStatus == WIFI_STATUS_DISCONNECTED && WiFi.status() == WL_CONNECTED) {
        g_wifiStatus = WIFI_STATUS_CONNECTED;
        Serial.println("[WiFi] STA auto-reconnected");
    }

    // Deferred NTP sync after a successful runtime change
    if (g_pendingNtpSync) {
        g_pendingNtpSync = false;
#if NTP_ENABLED
        triggerNtpResync();
#endif
    }
}

#endif // WIFI_CONFIG_ENABLED

#if NTP_ENABLED
#include <time.h>
#include "esp_sntp.h"

static volatile bool _timeSynced = false;

// SNTP callback — fired by ESP-IDF when NTP response arrives
static void ntpSyncCallback(struct timeval *tv)
{
    _timeSynced = true;
    struct tm timeinfo;
    localtime_r(&tv->tv_sec, &timeinfo);
    char buf[20];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
    Serial.printf("[NTP] Callback: time synced → %s\n", buf);
}

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
    if (getLocalTime(&timeinfo, 0)) {  // timeout=0: don't block
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("[NTP] RTC time before sync: %s\n", buf);
    } else {
        Serial.println("[NTP] No RTC time available");
    }

    // Register callback before starting SNTP
    sntp_set_time_sync_notification_cb(ntpSyncCallback);

    // 重置同步狀態，避免深度睡眠醒來後繼承上次 boot 的舊 COMPLETED 狀態
    sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);

    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    Serial.printf("[NTP] Servers: %s, %s, %s\n", NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    Serial.print("[NTP] Waiting for sync");

    // Poll with callback — loop is just for blocking setup(), callback does the real detection
    for (int i = 0; i < 60; i++) {
        if (_timeSynced) break;
        delay(500);
        Serial.print(".");
    }

    if (_timeSynced) {
        getLocalTime(&timeinfo, 0);
        char buf[20];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &timeinfo);
        Serial.printf("\n[NTP] Time synced: %s\n", buf);
    } else {
        Serial.println("\n[NTP] Sync failed after 30s — RTC time preserved as fallback");
    }
}

/*
Non-blocking NTP re-sync. Configures SNTP and returns immediately.
The existing ntpSyncCallback handles the actual time update asynchronously.
Safe to call from processWifiTasks() in the WebAndTasks loop.
*/
void triggerNtpResync()
{
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[NTP] Re-sync skipped — STA not connected");
        return;
    }
    sntp_set_sync_status(SNTP_SYNC_STATUS_RESET);
    _timeSynced = false;
    configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3);
    Serial.println("[NTP] Re-sync triggered (non-blocking)");
}
#endif // NTP_ENABLED

#endif // WIFI_ENABLED
