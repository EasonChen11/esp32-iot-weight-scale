#include "config.h"
#if WIFI_ENABLED
#include <WiFi.h>
#include "wifi_manager.h"

/*
Initialize WiFi in Station (STA) mode so the ESP32 joins the home/lab network
and can reach the MQTT broker running on the PC.

Connection flow:
  1. Attempt to join STA_WIFI_SSID for up to 15 seconds.
  2. On success: print the DHCP IP – use this to open the web interface.
  3. On failure: fall back to Soft-AP mode (AP_WIFI_SSID / AP_WIFI_PASS) so
     the web interface remains accessible; MQTT will not be available.

Parameters:
  none

Returns:
  void
*/
void initWiFi()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(STA_WIFI_SSID, STA_WIFI_PASS);

    Serial.printf("[WiFi] Connecting to '%s'", STA_WIFI_SSID);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 30) // 30 × 500ms = 15s
    {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] Web interface: http://%s\n", WiFi.localIP().toString().c_str());
        Serial.printf("[WiFi] MQTT broker:   %s:%d\n", MQTT_BROKER_IP, MQTT_BROKER_PORT);
    }
    else
    {
        Serial.println("\n[WiFi] STA connection failed. Starting fallback AP...");
        WiFi.mode(WIFI_AP);
        WiFi.softAP(AP_WIFI_SSID, AP_WIFI_PASS);
        Serial.printf("[WiFi] AP started: SSID='%s'  IP: %s\n",
                      AP_WIFI_SSID, WiFi.softAPIP().toString().c_str());
        Serial.println("[WiFi] WARNING: MQTT unavailable in AP-fallback mode.");
    }
}

#endif // WIFI_ENABLED
