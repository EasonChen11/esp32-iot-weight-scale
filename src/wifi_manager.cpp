#include <WiFi.h>
#include "wifi_manager.h"
#include "config.h"

void initWiFi()
{
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    Serial.println("WiFi Access Point Started");
    Serial.print("IP Address: ");
    Serial.println(WiFi.softAPIP());
}