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
