#include <Arduino.h>
#include "wifi_manager.h"
#include "sensor_manager.h"
#include "web_server_logic.h"
#include "storage_manager.h"

WebServer server(80);

/*
System initialization routine.
Sets up serial communication, initializes storage, loads calibration data,
configures WiFi, initializes the weight sensor, and sets up all HTTP routes.

Returns:
  void
*/
void setup()
{
    Serial.begin(115200);
    initStorage();
    long offset = getAbsoluteOffset();

    initWiFi();
    initSensor(offset);
    initWebRoutes(server);
}

/*
Main application loop.
Handles incoming HTTP client requests and updates the weight sensor readings
at regular intervals.

Returns:
  void
*/
void loop()
{
    server.handleClient();
    updateSensor();
}