#include "web_server_logic.h"
#include "web_pages.h"
#include "sensor_manager.h"
#include "storage_manager.h"
#include <time.h>

/*
Initialize and register all HTTP routes for the web server.
This function sets up all REST API endpoints including:
  - Data endpoints: /, /data, /get-records
  - Control endpoints: /tare, /sync, /add-record, /del-record, /clear-records, /set-zero

Parameters:
  server (WebServer&): Reference to the ESP32 WebServer instance

Returns:
  void
*/
void initWebRoutes(WebServer &server)
{
    // Root route: serve the main dashboard
    server.on("/", [&server]()
              { server.send(200, "text/html", getIndexHTML()); });

    // Get current weight reading
    server.on("/data", [&server]()
              { server.send(200, "text/plain", String(getCachedWeight(), 2)); });

    // Perform temporary tare (zero adjustment)
    server.on("/tare", [&server]()
              {
                  tareSensor();
                  server.send(200, "text/plain", "Tare completed successfully"); });

    // Time synchronization endpoint
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

    // Get current system time
    server.on("/time", [&server]()
              {
                  struct tm timeinfo;
                  if(!getLocalTime(&timeinfo)){
                      server.send(200, "text/plain", "Not synced");
                      return;
                  }
                  char buf[20];
                  strftime(buf, sizeof(buf), "%H:%M:%S", &timeinfo);
                  server.send(200, "text/plain", String(buf)); });

    // Retrieve all stored records
    server.on("/get-records", [&server]()
              { server.send(200, "application/json", getRecordsJson()); });

    // Add new measurement record
    server.on("/add-record", [&server]()
              {
                  String t = server.arg("t");
                  String w = server.arg("w");
                  addRecordToStorage(t, w);
                  server.send(200, "application/json", getRecordsJson()); });

    // Delete specific record by index
    server.on("/del-record", [&server]()
              {
                  int index = server.arg("i").toInt();
                  deleteRecordFromStorage(index);
                  server.send(200, "application/json", getRecordsJson()); });

    // Clear all records
    server.on("/clear-records", [&server]()
              {
                  clearRecordsInStorage();
                  server.send(200, "application/json", "[]"); });

    // Perform absolute zero-point calibration
    server.on("/set-zero", [&server]()
              {    
                  long newOffset = captureAbsoluteOffset();
                  saveAbsoluteOffset(newOffset);
                  Serial.printf("[Web] Absolute zero calibration completed. New offset: %ld\n", newOffset);
                  server.send(200, "text/plain", "OK"); });

    server.begin();
}