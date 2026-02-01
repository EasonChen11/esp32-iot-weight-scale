#include <Arduino.h>
#include "wifi_manager.h"
#include "sensor_manager.h"
#include "web_server_logic.h"
#include "storage_manager.h"

WebServer server(80);

void setup()
{
    Serial.begin(115200);
    initStorage();
    long offset = getAbsoluteOffset();

    initWiFi();
    initSensor(offset);
    initWebRoutes(server);
}

void loop()
{
    server.handleClient(); // 處理網路請求
    updateSensor();        // 處理數據採樣
}