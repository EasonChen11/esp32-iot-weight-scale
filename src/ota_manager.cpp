#include "config.h"
#if OTA_ENABLED
#include "ota_manager.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct OtaManifest {
    String version;
    String url;
    String sha256;
};

static bool otaInProgress = false;

void initOTA()
{
    Serial.printf("[OTA] Running firmware version: %s\n", FIRMWARE_VERSION);
}

// Returns true if `candidate` is a strictly newer semver than `current`.
// Parses up to three dot-separated integer fields ("1.2.0"); missing fields = 0.
// Non-numeric / malformed input compares as 0, so it never updates on garbage.
static bool isNewerVersion(const char *current, const char *candidate)
{
    int cur[3] = {0, 0, 0};
    int cand[3] = {0, 0, 0};
    sscanf(current, "%d.%d.%d", &cur[0], &cur[1], &cur[2]);
    sscanf(candidate, "%d.%d.%d", &cand[0], &cand[1], &cand[2]);
    for (int i = 0; i < 3; i++)
    {
        if (cand[i] > cur[i]) return true;
        if (cand[i] < cur[i]) return false;
    }
    return false; // equal
}

// Attach the Arduino-ESP32 built-in Mozilla CA bundle so any valid public
// certificate verifies — this also survives GitHub's 302 redirect to
// objects.githubusercontent.com (a different CA than github.com).
extern const uint8_t rootca_crt_bundle_start[] asm("_binary_data_cert_x509_crt_bundle_bin_start");

static bool fetchManifest(OtaManifest &out)
{
    WiFiClientSecure client;
    client.setCACertBundle(rootca_crt_bundle_start);

    HTTPClient http;
    http.setTimeout(OTA_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    if (!http.begin(client, OTA_MANIFEST_URL))
    {
        Serial.println("[OTA] manifest begin() failed");
        return false;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        Serial.printf("[OTA] manifest HTTP %d\n", code);
        http.end();
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();
    if (err)
    {
        Serial.printf("[OTA] manifest JSON error: %s\n", err.c_str());
        return false;
    }

    out.version = doc["version"].as<String>();
    out.url = doc["url"].as<String>();
    out.sha256 = doc["sha256"].as<String>();
    if (out.version.isEmpty() || out.url.isEmpty() || out.sha256.length() != 64)
    {
        Serial.println("[OTA] manifest missing/invalid fields");
        return false;
    }
    return true;
}

void checkOtaUpdate()
{
    OtaManifest m;
    if (!fetchManifest(m)) return;

    Serial.printf("[OTA] running %s, latest %s\n", FIRMWARE_VERSION, m.version.c_str());
    if (!isNewerVersion(FIRMWARE_VERSION, m.version.c_str()))
    {
        Serial.println("[OTA] already up to date");
        return;
    }
    Serial.printf("[OTA] update available -> %s\n", m.version.c_str());
    // Download + verify + apply implemented in Task 6.
}

bool isOtaInProgress()
{
    return otaInProgress;
}

#endif // OTA_ENABLED
