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

// Scale factor storage (namespace "scale_data", keys "scale1" "scale2")
void  saveScaleFactor1(float factor);
float getScaleFactor1();
bool  hasScaleFactor1();
void  saveScaleFactor2(float factor);
float getScaleFactor2();
bool  hasScaleFactor2();
