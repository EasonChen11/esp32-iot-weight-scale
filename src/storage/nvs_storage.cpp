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
  bool has = prefs.isKey("ssid");
  if (has) {
    has = prefs.getString("ssid", "").length() > 0;
  }
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

void saveScaleFactor1(float factor)
{
  Preferences prefs;
  prefs.begin("scale_data", false);
  prefs.putFloat("scale1", factor);
  prefs.end();
  Serial.printf("[Storage] Sensor 1 scale factor saved: %.2f\n", factor);
}

float getScaleFactor1()
{
  Preferences prefs;
  prefs.begin("scale_data", true);
  float factor = prefs.getFloat("scale1", 0.0f);
  prefs.end();
  return factor;
}

bool hasScaleFactor1()
{
  Preferences prefs;
  prefs.begin("scale_data", true);
  bool has = prefs.isKey("scale1");
  prefs.end();
  return has;
}

void saveScaleFactor2(float factor)
{
  Preferences prefs;
  prefs.begin("scale_data", false);
  prefs.putFloat("scale2", factor);
  prefs.end();
  Serial.printf("[Storage] Sensor 2 scale factor saved: %.2f\n", factor);
}

float getScaleFactor2()
{
  Preferences prefs;
  prefs.begin("scale_data", true);
  float factor = prefs.getFloat("scale2", 0.0f);
  prefs.end();
  return factor;
}

bool hasScaleFactor2()
{
  Preferences prefs;
  prefs.begin("scale_data", true);
  bool has = prefs.isKey("scale2");
  prefs.end();
  return has;
}

void saveSheetsConfig(const String &url, const String &token)
{
  Preferences prefs;
  prefs.begin("sheets_cfg", false);
  prefs.putString("url", url);
  prefs.putString("token", token);
  prefs.end();
  // Never log the token.
  Serial.println("[Storage] Google Sheets config saved");
}

bool getSheetsConfig(String &urlOut, String &tokenOut)
{
  Preferences prefs;
  prefs.begin("sheets_cfg", true);
  urlOut = prefs.getString("url", "");
  tokenOut = prefs.getString("token", "");
  prefs.end();
  return urlOut.length() > 0 && tokenOut.length() > 0;
}

// Stricter than the WiFi hasStoredCredentials(): requires BOTH url and token
// non-empty, since a token-less Sheets sync would be rejected by the GAS receiver.
bool hasSheetsConfig()
{
  Preferences prefs;
  prefs.begin("sheets_cfg", true);
  bool has = prefs.isKey("url") && prefs.isKey("token");
  if (has) {
    has = prefs.getString("url", "").length() > 0 &&
          prefs.getString("token", "").length() > 0;
  }
  prefs.end();
  return has;
}

void clearSheetsConfig()
{
  Preferences prefs;
  prefs.begin("sheets_cfg", false);
  prefs.remove("url");
  prefs.remove("token");
  prefs.end();
  Serial.println("[Storage] Google Sheets config cleared");
}
