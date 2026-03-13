#include "config.h"
#if WEB_SERVER_ENABLED
#include "web_server_logic.h"
#include "web_pages.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include <time.h>
#include <LittleFS.h>

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
    // Root: serve the main dashboard
    server.on("/", [&server]()
              { server.send(200, "text/html", getIndexHTML()); });

    // Chart.js served locally from LittleFS (no CDN — works in AP mode)
    // Browser caches for 24 h so subsequent loads are instant
    server.on("/chartjs", [&server]()
              {
                  File f = LittleFS.open("/chart.min.js", "r");
                  if (!f) {
                      server.send(404, "text/plain", "chart.min.js not found — run Upload Filesystem Image");
                      return;
                  }
                  server.sendHeader("Cache-Control", "max-age=86400");
                  server.streamFile(f, "application/javascript");
                  f.close();
              });

    // ── Weight data endpoints ──────────────────────────────────────────

    // Total weight (sum of both sensors) — also used by legacy clients
    server.on("/data", [&server]()
              { server.send(200, "text/plain", String(getCachedWeight(), 2)); });

    // Sensor 1 weight
    server.on("/data1", [&server]()
              { server.send(200, "text/plain", String(getCachedWeight1(), 2)); });

    // Sensor 2 weight
    server.on("/data2", [&server]()
              { server.send(200, "text/plain", String(getCachedWeight2(), 2)); });

    // ── Tare endpoints ────────────────────────────────────────────────

    server.on("/tare1", [&server]()
              {
                  tareSensor1();
                  server.send(200, "text/plain", "Sensor 1 tare completed"); });

    server.on("/tare2", [&server]()
              {
                  tareSensor2();
                  server.send(200, "text/plain", "Sensor 2 tare completed"); });

    // Tare both sensors
    server.on("/tare", [&server]()
              {
                  tareSensor();
                  server.send(200, "text/plain", "Both sensors tare completed"); });

    // ── Time synchronization ──────────────────────────────────────────

    server.on("/sync", [&server]()
              {
                  if (server.hasArg("t")) {
                      time_t t = (time_t)server.arg("t").substring(0, 10).toInt();
                      struct timeval tv = { .tv_sec = t, .tv_usec = 0 };
                      settimeofday(&tv, NULL);
                      Serial.print("[Web] Time synchronized: ");
                      Serial.println(t);
                      server.send(200, "text/plain", "OK");
                  } else {
                      server.send(400, "text/plain", "Missing time parameter");
                  } });

    server.on("/time", [&server]()
              {
                  struct tm timeinfo;
                  if (!getLocalTime(&timeinfo)) {
                      server.send(200, "text/plain", "Not synced");
                      return;
                  }
                  char buf[20];
                  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
                  server.send(200, "text/plain", String(buf)); });

    // ── Record management ─────────────────────────────────────────────

    server.on("/get-records", [&server]()
              { server.send(200, "application/json", getRecordsJson()); });

    server.on("/add-record", [&server]()
              {
                  String t = server.arg("t");
                  String w = server.arg("w");
                  addRecordToStorage(t, w);
                  server.send(200, "application/json", getRecordsJson()); });

    server.on("/del-record", [&server]()
              {
                  int index = server.arg("i").toInt();
                  deleteRecordFromStorage(index);
                  server.send(200, "application/json", getRecordsJson()); });

    server.on("/clear-records", [&server]()
              {
                  clearRecordsInStorage();
                  server.send(200, "application/json", "[]"); });

    // ── Absolute zero calibration ─────────────────────────────────────

    // Calibrate sensor 1
    server.on("/set-zero1", [&server]()
              {
                  long newOffset = captureAbsoluteOffset1();
                  saveAbsoluteOffset(newOffset);
                  Serial.printf("[Web] Sensor 1 calibration done. Offset: %ld\n", newOffset);
                  server.send(200, "text/plain", "OK"); });

    // Calibrate sensor 2
    server.on("/set-zero2", [&server]()
              {
                  long newOffset = captureAbsoluteOffset2();
                  saveAbsoluteOffset2(newOffset);
                  Serial.printf("[Web] Sensor 2 calibration done. Offset: %ld\n", newOffset);
                  server.send(200, "text/plain", "OK"); });

    server.begin();
}

#endif // WEB_SERVER_ENABLED
