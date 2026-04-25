#include "config.h"
#if WEB_SERVER_ENABLED
#include "web_server_logic.h"
#include "web_pages.h"
#include "sensor_manager.h"
#include "storage/littlefs_storage.h"
#include "storage/nvs_storage.h"
#include "storage/littlefs_storage.h"
#if SCHEDULE_ENABLED
#include "schedule_manager.h"
#endif
#include <time.h>
#include <LittleFS.h>
#if NTP_ENABLED || WIFI_CONFIG_ENABLED
#include "wifi_manager.h"
#endif
#if GOOGLE_SHEETS_ENABLED
#include "google_sheets_manager.h"
#endif
#if DEV_MODE_ENABLED
#include "dev_mode.h"
#endif

/*
Initialize and register all HTTP routes for the web server.
Endpoints:
  Static  : /chartjs  (Chart.js from LittleFS — cached 24 h)
  Data    : /, /data, /data1, /data2, /get-records
  Control : /tare1, /tare2, /tare, /sync, /add-record, /del-record,
            /clear-records, /set-zero1, /set-zero2
*/
void initWebRoutes(WebServer &server)
{
#if DEV_MODE_ENABLED
    auto requireDevMode = [&server]() -> bool {
        if (!isDevMode()) {
            server.sendHeader("Cache-Control", "no-store");
            server.send(403, "text/plain", "Developer mode required");
            return false;
        }
        return true;
    };
#endif

    // Root: serve the main dashboard
    server.on("/", [&server]()
              {
                  server.sendHeader("Cache-Control", "max-age=300, must-revalidate");
                  server.send(200, "text/html", getIndexHTML()); });

    // Chart.js served locally from LittleFS (no CDN — works in AP mode)
    // Browser caches for 24 h so subsequent loads are instant
    server.on("/chartjs", [&server]()
              {
                  File f = LittleFS.open("/chart.min.js.gz", "r");
                  if (!f) {
                      server.send(404, "text/plain", "chart.min.js.gz not found — run Upload Filesystem Image");
                      return;
                  }
                  size_t fileSize = f.size();
                  server.setContentLength(fileSize);
                  server.sendHeader("Content-Encoding", "gzip");
                  server.sendHeader("Cache-Control", "max-age=86400");
                  server.send(200, "application/javascript", "");
                  uint8_t buf[512];
                  while (f.available()) {
                      size_t n = f.read(buf, sizeof(buf));
                      server.client().write(buf, n);
                  }
                  f.close();
              });

    // ── Weight data endpoints ──────────────────────────────────────────

    // Total weight (sum of both sensors) — also used by legacy clients
    server.on("/data", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  server.send(200, "text/plain", String(getCachedWeight(), 2)); });

    // Sensor 1 weight
    server.on("/data1", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  server.send(200, "text/plain", String(getCachedWeight1(), 2)); });

    // Sensor 2 weight
    server.on("/data2", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  server.send(200, "text/plain", String(getCachedWeight2(), 2)); });

    // Combined polling endpoint: sensors + wifi status in one response.
    // Halves the polling request rate compared to /data1 + /data2 + /wifi-status.
    server.on("/tick", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  String json = "{\"s1\":" + String(getCachedWeight1(), 2)
                              + ",\"s2\":" + String(getCachedWeight2(), 2);
#if WIFI_CONFIG_ENABLED
                  json += ",\"wifi\":" + getWifiStatusJson();
#endif
#if DEV_MODE_ENABLED
                  json += ",\"dev\":";
                  json += isDevMode() ? "true" : "false";
#endif
                  json += "}";
                  server.send(200, "application/json", json); });

    // ── Tare endpoints ────────────────────────────────────────────────

    server.on("/tare1", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  tareSensor1();
                  server.send(200, "text/plain", "Sensor 1 tare completed"); });

    server.on("/tare2", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  tareSensor2();
                  server.send(200, "text/plain", "Sensor 2 tare completed"); });

    // Tare both sensors
    server.on("/tare", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  tareSensor();
                  server.send(200, "text/plain", "Both sensors tare completed"); });

    // ── Time synchronization ──────────────────────────────────────────

    server.on("/sync", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  if (!server.hasArg("t")) {
                      server.send(400, "text/plain", "Missing time parameter");
                      return;
                  }
#if NTP_ENABLED
                  if (isTimeSynced()) {
                      Serial.println("[Web] /sync skipped — NTP already synced");
                      server.send(200, "text/plain", "NTP");
                      return;
                  }
#endif
                  time_t t = (time_t)server.arg("t").substring(0, 10).toInt();
                  struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
                  settimeofday(&tv, NULL);
#if NTP_ENABLED
                  setTimeSynced(true);
#endif
                  Serial.printf("[Web] Time synchronized via browser: %ld\n", (long)t);
                  server.send(200, "text/plain", "OK"); });

    server.on("/time", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  struct tm timeinfo;
                  if (!getLocalTime(&timeinfo)) {
                      server.send(200, "text/plain", "Not synced");
                      return;
                  }
                  char buf[20];
                  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
                  server.send(200, "text/plain", String(buf)); });

#if WIFI_CONFIG_ENABLED
    // ── Dynamic WiFi configuration ────────────────────────────────────

    server.on("/network", [&server]()
              {
                  server.sendHeader("Cache-Control", "max-age=300, must-revalidate");
                  server.send(200, "text/html", getNetworkPageHTML()); });

    server.on("/wifi-status", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  server.send(200, "application/json", getWifiStatusJson()); });

    server.on("/network/scan", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  String json = scanNetworksJson();
                  if (json.startsWith("{\"error\":\"busy\"")) {
                      server.send(409, "application/json", json);
                  } else if (json.startsWith("{\"error\":")) {
                      server.send(500, "application/json", json);
                  } else {
                      server.send(200, "application/json", json);
                  } });

    server.on("/network/save", HTTP_POST, [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  String ssid = server.arg("s");
                  String pass = server.arg("p");
                  String err;
                  if (!requestStaChange(ssid, pass, err)) {
                      if (err == "busy") {
                          String body = "{\"error\":\"busy\",\"current_ssid\":\""
                                        + getCurrentSsid() + "\"}";
                          server.send(409, "application/json", body);
                      } else {
                          server.send(400, "application/json",
                                      "{\"error\":\"" + err + "\"}");
                      }
                      return;
                  }
                  server.send(202, "application/json",
                              "{\"status\":\"connecting\"}"); });

    server.on("/network/clear", HTTP_POST, [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  clearStaCredentials();
                  server.send(200, "application/json",
                              "{\"status\":\"cleared\"}"); });
#endif

    // ── Record management ─────────────────────────────────────────────

    server.on("/get-records", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  server.send(200, "application/json", getRecordsJson()); });

    server.on("/add-record", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  String t  = server.arg("t");
                  String d  = server.arg("d");
                  String s1 = server.arg("s1");
                  String s2 = server.arg("s2");
                  long id = getNextRecordId();
                  addRecordToStorage(id, d, t, s1, s2);
                  server.send(200, "application/json", getRecordsJson()); });

    server.on("/del-record", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  int index = server.arg("i").toInt();
                  deleteRecordFromStorage(index);
                  server.send(200, "application/json", getRecordsJson()); });

    server.on("/clear-records", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  clearRecordsInStorage();
                  server.send(200, "application/json", "[]"); });

    // ── Absolute zero calibration ─────────────────────────────────────

    // Calibrate sensor 1
    server.on("/set-zero1", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  long newOffset = captureAbsoluteOffset1();
                  saveAbsoluteOffset(newOffset);
                  Serial.printf("[Web] Sensor 1 calibration done. Offset: %ld\n", newOffset);
                  server.send(200, "text/plain", "OK"); });

    // Calibrate sensor 2
    server.on("/set-zero2", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  long newOffset = captureAbsoluteOffset2();
                  saveAbsoluteOffset2(newOffset);
                  Serial.printf("[Web] Sensor 2 calibration done. Offset: %ld\n", newOffset);
                  server.send(200, "text/plain", "OK"); });

    // ── Scale factor calibration ──────────────────────────────────────

    server.on("/calibrate-scale1", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  if (!server.hasArg("w")) {
                      server.send(400, "application/json", "{\"error\":\"缺少重量參數\"}");
                      return;
                  }
                  float w = server.arg("w").toFloat();
                  String err;
                  float newFactor = calibrateScaleFactor1(w, err);
                  if (newFactor < 0.0f) {
                      String body = "{\"error\":\"" + err + "\"}";
                      server.send(400, "application/json", body);
                      return;
                  }
                  String body = "{\"factor\":" + String(newFactor, 2) + "}";
                  server.send(200, "application/json", body); });

    server.on("/calibrate-scale2", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  if (!server.hasArg("w")) {
                      server.send(400, "application/json", "{\"error\":\"缺少重量參數\"}");
                      return;
                  }
                  float w = server.arg("w").toFloat();
                  String err;
                  float newFactor = calibrateScaleFactor2(w, err);
                  if (newFactor < 0.0f) {
                      String body = "{\"error\":\"" + err + "\"}";
                      server.send(400, "application/json", body);
                      return;
                  }
                  String body = "{\"factor\":" + String(newFactor, 2) + "}";
                  server.send(200, "application/json", body); });

    // ── Wake-up schedule management ──────────────────────────────────────

#if SCHEDULE_ENABLED
    server.on("/get-schedule", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  server.send(200, "application/json", getScheduleJson()); });

    server.on("/add-schedule", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  int h = server.arg("h").toInt();
                  int m = server.arg("m").toInt();
                  if (addScheduleEntry(h, m)) {
                      server.send(200, "application/json", getScheduleJson());
                  } else {
                      server.send(400, "text/plain", "Invalid or duplicate time");
                  } });

    server.on("/del-schedule", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  int idx = server.arg("i").toInt();
                  removeScheduleEntry(idx);
                  server.send(200, "application/json", getScheduleJson()); });

    server.on("/clear-schedule", HTTP_POST, [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  clearAllScheduleEntries();
                  server.send(200, "application/json", getScheduleJson()); });
#endif

#if GOOGLE_SHEETS_ENABLED
    server.on("/sync-sheets", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  String result = triggerGoogleSheetsSync();
                  server.send(200, "text/plain", result); });
#endif

#if DEV_MODE_ENABLED
    server.on("/dev-status", [&server]()
              {
                  server.sendHeader("Cache-Control", "no-store");
                  String body = "{\"dev\":";
                  body += isDevMode() ? "true" : "false";
                  body += "}";
                  server.send(200, "application/json", body); });
#endif

    server.onNotFound([&server]()
              {
                  Serial.printf("[Web] 404: %s %s\n", server.method() == HTTP_GET ? "GET" : "POST", server.uri().c_str());
                  server.send(404, "text/plain", "Not Found"); });

    server.begin();
}

#endif // WEB_SERVER_ENABLED
