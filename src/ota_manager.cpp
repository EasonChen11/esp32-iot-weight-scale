#include "config.h"
#if OTA_ENABLED
#include "ota_manager.h"
#include <Arduino.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Update.h>
#include <mbedtls/sha256.h>
#include <esp_ota_ops.h>

struct OtaManifest {
    String version;
    String url;
    String sha256;
};

static bool otaInProgress = false;

void initOTA()
{
    Serial.printf("[OTA] Running firmware version: %s\n", FIRMWARE_VERSION);

    // Rollback safety: if this image booted in PENDING_VERIFY (just OTA-flashed),
    // confirm it is healthy. Reaching setup() with WiFi/storage initialized is our
    // self-test; mark valid so it is not rolled back on the next reboot.
    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t state;
    if (esp_ota_get_state_partition(running, &state) == ESP_OK &&
        state == ESP_OTA_IMG_PENDING_VERIFY)
    {
        esp_ota_mark_app_valid_cancel_rollback();
        Serial.println("[OTA] new image self-test passed — marked valid");
    }
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

static String toHex(const uint8_t *buf, size_t len)
{
    static const char *hex = "0123456789abcdef";
    String s;
    s.reserve(len * 2);
    for (size_t i = 0; i < len; i++)
    {
        s += hex[buf[i] >> 4];
        s += hex[buf[i] & 0x0F];
    }
    return s;
}

// Streams firmware.bin into the inactive OTA slot while hashing it.
// Only finalizes the image if the sha256 matches the manifest. Returns true
// if the new image was written and the device is about to reboot.
static bool downloadAndApply(const OtaManifest &m)
{
    WiFiClientSecure client;
    client.setCACertBundle(rootca_crt_bundle_start);

    HTTPClient http;
    http.setTimeout(OTA_HTTP_TIMEOUT_MS);
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    if (!http.begin(client, m.url))
    {
        Serial.println("[OTA] firmware begin() failed");
        return false;
    }

    int code = http.GET();
    if (code != HTTP_CODE_OK)
    {
        Serial.printf("[OTA] firmware HTTP %d\n", code);
        http.end();
        return false;
    }

    int total = http.getSize();
    if (total <= 0 || !Update.begin((size_t)total))
    {
        Serial.printf("[OTA] Update.begin failed (size=%d): %s\n", total, Update.errorString());
        Update.abort();
        http.end();
        return false;
    }

    mbedtls_sha256_context sha;
    mbedtls_sha256_init(&sha);
    mbedtls_sha256_starts(&sha, 0); // 0 = SHA-256

    WiFiClient *stream = http.getStreamPtr();
    uint8_t buf[1024];
    size_t total_s = (size_t)total;
    size_t written = 0;
    uint32_t lastDataMs = millis();
    while (http.connected() && written < total_s)
    {
        size_t avail = stream->available();
        if (avail == 0)
        {
            // Guard against a hung connection burning the deep-sleep awake window.
            if (millis() - lastDataMs > OTA_HTTP_TIMEOUT_MS)
            {
                Serial.println("[OTA] download stalled — timed out");
                break;
            }
            delay(1);
            continue;
        }
        size_t n = stream->readBytes(buf, avail > sizeof(buf) ? sizeof(buf) : avail);
        if (n == 0) break;
        mbedtls_sha256_update(&sha, buf, n);
        if (Update.write(buf, n) != n)
        {
            Serial.printf("[OTA] Update.write failed: %s\n", Update.errorString());
            Update.abort();
            mbedtls_sha256_free(&sha);
            http.end();
            return false;
        }
        written += n;
        lastDataMs = millis();
    }
    http.end();

    if (written != total_s)
    {
        Serial.printf("[OTA] truncated: %d/%d bytes\n", (int)written, total);
        mbedtls_sha256_free(&sha);
        Update.abort();
        return false;
    }

    uint8_t digest[32];
    mbedtls_sha256_finish(&sha, digest);
    mbedtls_sha256_free(&sha);

    String got = toHex(digest, 32);
    if (!got.equalsIgnoreCase(m.sha256))
    {
        Serial.printf("[OTA] sha256 mismatch\n  expect %s\n  got    %s\n",
                      m.sha256.c_str(), got.c_str());
        Update.abort();
        return false;
    }

    if (!Update.end(true))
    {
        Serial.printf("[OTA] Update.end failed: %s\n", Update.errorString());
        Update.abort();
        return false;
    }

    Serial.printf("[OTA] verified + written %s, rebooting...\n", m.version.c_str());
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
    otaInProgress = true;
    bool ok = downloadAndApply(m);
    otaInProgress = false;
    if (ok)
    {
        delay(500);
        ESP.restart();
    }
}

bool isOtaInProgress()
{
    return otaInProgress;
}

#endif // OTA_ENABLED
