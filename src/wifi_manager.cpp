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

#endif // WIFI_ENABLED
