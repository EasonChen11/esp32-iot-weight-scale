#include "storage/nvs_storage.h"
#include <Preferences.h>
#include <Arduino.h>

void saveAbsoluteOffset(long offset)
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  preferences.putLong("offset", offset);
  preferences.end();

  Serial.printf("[Storage] Sensor 1 absolute offset saved: %ld\n", offset);
}

void saveAbsoluteOffset2(long offset)
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  preferences.putLong("offset2", offset);
  preferences.end();

  Serial.printf("[Storage] Sensor 2 absolute offset saved: %ld\n", offset);
}

long getAbsoluteOffset()
{
  Preferences preferences;
  preferences.begin("scale_data", true);
  long offset = preferences.getLong("offset", 0);
  preferences.end();

  return offset;
}

long getAbsoluteOffset2()
{
  Preferences preferences;
  preferences.begin("scale_data", true);
  long offset = preferences.getLong("offset2", 0);
  preferences.end();

  return offset;
}

long getNextRecordId()
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  long id = preferences.getLong("record_id", 0) + 1;
  preferences.putLong("record_id", id);
  preferences.end();

  return id;
}

void resetRecordId()
{
  Preferences preferences;
  preferences.begin("scale_data", false);
  preferences.remove("record_id");
  preferences.end();

  Serial.println("[Storage] Record ID counter reset to 0");
}

void saveStaCredentials(const String &ssid, const String &pass)
{
  Preferences prefs;
  prefs.begin("wifi_cfg", false);
  prefs.putString("ssid", ssid);
  prefs.putString("pass", pass);
  prefs.end();
  Serial.printf("[Storage] WiFi credentials saved: %s\n", ssid.c_str());
}

bool getStaCredentials(String &ssidOut, String &passOut)
{
  Preferences prefs;
  prefs.begin("wifi_cfg", true);
  ssidOut = prefs.getString("ssid", "");
  passOut = prefs.getString("pass", "");
  prefs.end();
  return ssidOut.length() > 0;
}

bool hasStoredCredentials()
{
  Preferences prefs;
  prefs.begin("wifi_cfg", true);
  bool has = prefs.isKey("ssid") && prefs.getString("ssid", "").length() > 0;
  prefs.end();
  return has;
}

void clearStaCredentials()
{
  Preferences prefs;
  prefs.begin("wifi_cfg", false);
  prefs.remove("ssid");
  prefs.remove("pass");
  prefs.end();
  Serial.println("[Storage] WiFi credentials cleared");
}
