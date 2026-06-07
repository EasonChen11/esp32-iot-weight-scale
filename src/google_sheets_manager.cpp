#include "config.h"
#if GOOGLE_SHEETS_ENABLED

#include "google_sheets_manager.h"
#include "storage/littlefs_storage.h"
#include "auto_logger.h"
#include "storage/nvs_storage.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static bool syncAttempted = false;

void initGoogleSheets()
{
    syncAttempted = false;
    Serial.printf("[GSheets] Service initialized (config source: %s). Waiting for startup record...\n",
                  hasSheetsConfig() ? "NVS" : "compile-time fallback");
}

// POST to GAS, then GET the redirect URL to retrieve the JSON response
static int gasPost(WiFiClientSecure &client, const String &url, const String &body, String &response)
{
    HTTPClient http;
    http.setFollowRedirects(HTTPC_DISABLE_FOLLOW_REDIRECTS);
    http.begin(client, url);
    http.addHeader("Content-Type", "application/json");
    http.setTimeout(15000);
    const char *headerKeys[] = {"Location"};
    http.collectHeaders(headerKeys, 1);

    int httpCode = http.POST(body);
    Serial.printf("[GSheets] POST → %d\n", httpCode);

    if (httpCode != 302)
    {
        if (httpCode > 0) response = http.getString();
        http.end();
        return httpCode;
    }

    String location = http.header("Location");
    http.end();

    if (location.length() == 0)
    {
        Serial.println("[GSheets] Redirect with no Location header");
        return -1;
    }

    Serial.println("[GSheets] Following redirect with GET...");

    HTTPClient http2;
    http2.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);
    http2.begin(client, location);
    http2.setTimeout(15000);

    httpCode = http2.GET();
    Serial.printf("[GSheets] GET → %d\n", httpCode);

    if (httpCode > 0)
    {
        response = http2.getString();
    }

    http2.end();
    return httpCode;
}

// Core sync logic — returns status message for web endpoint
String triggerGoogleSheetsSync()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        return "No WiFi connection";
    }

    String unsyncedJson = getUnsyncedRecordsJson();

    JsonDocument checkDoc;
    deserializeJson(checkDoc, unsyncedJson);
    JsonArray unsyncedArr = checkDoc.as<JsonArray>();

    if (unsyncedArr.size() == 0)
    {
        return "No unsynced records";
    }

    Serial.printf("[GSheets] Syncing %d records...\n", unsyncedArr.size());

    // Resolve URL + token: NVS-provisioned values take precedence over the
    // compile-time defaults. This lets OTA-built firmware (which ships with
    // placeholder secrets) keep syncing once the operator runs `sheets-set`.
    String sheetsUrl, sheetsToken;
    if (getSheetsConfig(sheetsUrl, sheetsToken)) {
        Serial.println("[GSheets] using NVS-provisioned URL/token");
    } else {
        sheetsUrl = String(GOOGLE_SHEETS_URL);
        sheetsToken = String(GOOGLE_SHEETS_TOKEN);
        Serial.println("[GSheets] using compile-time URL/token");
    }

    JsonDocument postDoc;
    postDoc["token"] = sheetsToken; // shared secret validated by the GAS receiver
    JsonArray records = postDoc["records"].to<JsonArray>();
    // Send oldest first (flash stores newest first) so GAS appends in chronological order
    for (int i = unsyncedArr.size() - 1; i >= 0; i--)
    {
        JsonObject src = unsyncedArr[i].as<JsonObject>();
        JsonObject dest = records.add<JsonObject>();
        dest["id"] = src["id"];
        dest["date"] = src["date"];
        dest["time"] = src["time"];
        dest["sensor1"] = src["sensor1"];
        dest["sensor2"] = src["sensor2"];
    }
    String postBody;
    serializeJson(postDoc, postBody);

    WiFiClientSecure client;
    client.setInsecure();

    String response;
    int httpCode = gasPost(client, sheetsUrl, postBody, response);

    if (httpCode == 200)
    {
        JsonDocument resDoc;
        DeserializationError err = deserializeJson(resDoc, response);

        if (!err && resDoc["success"].as<bool>() && resDoc["received_ids"].is<JsonArray>())
        {
            JsonArray ids = resDoc["received_ids"].as<JsonArray>();
            for (JsonVariant v : ids)
            {
                markRecordSynced(v.as<long>());
            }
            Serial.printf("[GSheets] Sync success: %d records marked synced\n", ids.size());
            return "Synced " + String(ids.size()) + " records";
        }
        return "Unexpected response";
    }

    return "HTTP error " + String(httpCode);
}

// Auto sync on boot — runs once after startup record
void handleGoogleSheetsSync()
{
    if (syncAttempted) return;
    if (!isStartupRecordDone()) return;

    syncAttempted = true;
    triggerGoogleSheetsSync();
}

#endif // GOOGLE_SHEETS_ENABLED
