#pragma once
#include "config.h"
#if WIFI_ENABLED

/*
Initialize WiFi in AP+STA mode (both interfaces active simultaneously).
  - AP (AP_WIFI_SSID): always on — phones/devices connect here to reach the web server.
  - STA (STA_WIFI_SSID): joins home network — needed for MQTT broker connectivity.

Parameters:
  none

Returns:
  void

Example:
  initWiFi();  // Start AP hotspot + attempt STA connection for MQTT
*/
void initWiFi();

#endif // WIFI_ENABLED
