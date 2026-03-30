#include <Arduino.h>
#include "config.h"
#include "sensor_manager.h"
#include "storage/storage_init.h"
#include "storage/nvs_storage.h"
#if WIFI_ENABLED
#include "wifi_manager.h"
#endif
#if WEB_SERVER_ENABLED
#include "web_server_logic.h"
#endif
#if AUTO_LOGGER_ENABLED
#include "auto_logger.h"
#endif
#if MQTT_ENABLED
#include "mqtt_manager.h"
#endif
#if GOOGLE_SHEETS_ENABLED
#include "google_sheets_manager.h"
#endif
#if OLED_ENABLED
#include "oled_manager.h"
#endif
#if SCHEDULE_ENABLED
#include "schedule_manager.h"
#endif
#if DEEP_SLEEP_ENABLED
#include "deep_sleep_manager.h"
#endif

#if WEB_SERVER_ENABLED
WebServer server(80);
#endif

// 1. Define task handle for FreeRTOS task
TaskHandle_t WebTaskHandle = NULL;

/*
WebAndTasksCode

Task function pinned to core 0 that handles the web server and automatic
logging tasks. These operations may perform network and filesystem I/O and
should run on the dedicated core.

Parameters:
  pvParameters (void*): FreeRTOS task parameter (unused)

Returns:
  void
*/
void WebAndTasksCode(void *pvParameters)
{
  Serial.printf("[System] Web & Tasks running on core: %d\n", xPortGetCoreID());

  for (;;)
  {
#if WEB_SERVER_ENABLED
    // Handle incoming HTTP requests
    server.handleClient();
#endif

#if AUTO_LOGGER_ENABLED
    // Perform hourly and startup logging (may perform file I/O)
    handleAutoLogging();
#endif

#if MQTT_ENABLED
    // Publish weight data to MQTT broker
    handleMQTT();
#endif

#if GOOGLE_SHEETS_ENABLED
    handleGoogleSheetsSync();
#endif

#if DEEP_SLEEP_ENABLED
    handleDeepSleep();
#endif

    // Small delay to avoid watchdog triggers on core 0
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  Serial.begin(115200);
  delay(300); // Wait for serial monitor connection

  // Initialize storage and sensor
  initStorage();
  long offset1 = getAbsoluteOffset();
  long offset2 = getAbsoluteOffset2();

#if WIFI_ENABLED
  initWiFi();
#if NTP_ENABLED
  initNTP();
#endif
#endif
  initSensor(offset1, offset2);
#if WEB_SERVER_ENABLED
  initWebRoutes(server);
#endif

#if AUTO_LOGGER_ENABLED
  initAutoLogger();
#endif

#if MQTT_ENABLED
  initMQTT();
#endif

#if GOOGLE_SHEETS_ENABLED
  initGoogleSheets();
#endif

#if OLED_ENABLED
  initOLED();
#endif

#if SCHEDULE_ENABLED
  initSchedule();
#endif

#if DEEP_SLEEP_ENABLED
  initDeepSleep();
#endif

  // 2. Create task pinned to core 0
  xTaskCreatePinnedToCore(
      WebAndTasksCode, /* 任務函式 */
      "WebAndTasks",   /* 任務名稱 */
      24576,            /* 堆疊大小 (Stack size) — increased for WiFiClientSecure TLS */
      NULL,            /* 傳入參數 */
      1,               /* 優先級 */
      &WebTaskHandle,  /* 任務句柄 */
      0                /* 固定在核心 0 */
  );

  Serial.println("[System] System initialization complete, dual-core mode active");
}

void loop()
{
  // Core 1: sensor reads + OLED (neither needs network)
  // Keeping OLED here frees Core 0 entirely for web + MQTT
  updateSensor();
#if OLED_ENABLED
  handleOLED();
#endif
  delay(1); // yield CPU time so WiFi stack on Core 0 gets more bandwidth
}