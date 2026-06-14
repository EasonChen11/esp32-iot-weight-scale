#pragma once

#include <Arduino.h>

void saveAbsoluteOffset(long offset);
long getAbsoluteOffset();
void saveAbsoluteOffset2(long offset);
long getAbsoluteOffset2();

long getNextRecordId();
void resetRecordId();

// WiFi credential storage (namespace "wifi_cfg")
void saveStaCredentials(const String &ssid, const String &pass);
bool getStaCredentials(String &ssidOut, String &passOut);
bool hasStoredCredentials();
void clearStaCredentials();

// Google Sheets config storage (namespace "sheets_cfg", keys "url" "token")
void saveSheetsConfig(const String &url, const String &token);
bool getSheetsConfig(String &urlOut, String &tokenOut);  // true only if BOTH stored & non-empty
bool hasSheetsConfig();
void clearSheetsConfig();

// AP (Soft-AP) config storage (namespace "ap_cfg", keys "ssid" "pass")
void saveApConfig(const String &ssid, const String &pass);
bool getApConfig(String &ssidOut, String &passOut);  // true only if ssid non-empty AND pass >= 8 chars
bool hasApConfig();
void clearApConfig();

// MQTT broker config storage (namespace "mqtt_cfg", keys "ip" "port")
void saveMqttConfig(const String &ip, uint16_t port);
bool getMqttConfig(String &ipOut, uint16_t &portOut);  // true only if ip non-empty AND port != 0
bool hasMqttConfig();
void clearMqttConfig();

// Scale factor storage (namespace "scale_data", keys "scale1" "scale2")
void  saveScaleFactor1(float factor);
float getScaleFactor1();
bool  hasScaleFactor1();
void  saveScaleFactor2(float factor);
float getScaleFactor2();
bool  hasScaleFactor2();
