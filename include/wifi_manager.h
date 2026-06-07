#pragma once
#include "config.h"
#if WIFI_ENABLED

// WiFi runtime status (single writer: processWifiTasks on Core 0)
enum WifiStatus {
    WIFI_STATUS_DISCONNECTED = 0,
    WIFI_STATUS_CONNECTING   = 1,
    WIFI_STATUS_CONNECTED    = 2,
    WIFI_STATUS_FAILED       = 3
};

/*
Initialize WiFi in AP+STA mode (both interfaces active simultaneously).
  - AP (AP_WIFI_SSID): always on — phones/devices connect here to reach the web server.
  - STA (STA_WIFI_SSID): joins home network — needed for MQTT broker connectivity.

Parameters:
  none

Returns:
  void

Example:
  initWiFi();     (starts AP hotspot + attempts STA connection for MQTT)
*/
void initWiFi();

#if WIFI_CONFIG_ENABLED
// Runtime control (called from web handlers)
bool   requestStaChange(const String &ssid, const String &pass, String &errorOut);
String getWifiStatusJson();
String scanNetworksJson();
String getCurrentSsid();

// Periodic state machine driver — call from WebAndTasks loop
void   processWifiTasks();
#endif

#if NTP_ENABLED
void initNTP();
void triggerNtpResync();
bool isTimeSynced();
void setTimeSynced(bool synced);
#endif

#endif // WIFI_ENABLED
