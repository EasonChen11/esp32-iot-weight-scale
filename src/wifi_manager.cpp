#include <WiFi.h>
#include "wifi_manager.h"
#include "config.h"

/*
Initialize and start the WiFi access point.
This function configures the ESP32 as a WiFi access point with the SSID
and password defined in config.h, allowing clients to connect wirelessly.

Parameters:
  none

Returns:
  void
*/
void initWiFi()
{
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    Serial.println("WiFi access point started");
    Serial.print("IP address: ");
    Serial.println(WiFi.softAPIP());
}