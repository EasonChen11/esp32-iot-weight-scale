#pragma once
#include "config.h"
#if WIFI_ENABLED

/*
Initialize and start the WiFi access point.
This function configures the ESP32 as a WiFi access point with the SSID
and password defined in config.h, allowing clients to connect wirelessly.

Parameters:
  none

Returns:
  void

Example:
  initWiFi();  // Start WiFi AP with "ESP32_Weight_Scale" SSID
*/
void initWiFi();

#endif // WIFI_ENABLED
