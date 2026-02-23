#include <Arduino.h>
#include "wifi_manager.h"
#include "sensor_manager.h"
#include "web_server_logic.h"
#include "storage_manager.h"
#include "auto_logger.h"

WebServer server(80);

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
    // Handle incoming HTTP requests
    server.handleClient();

    // Perform hourly and startup logging (may perform file I/O)
    handleAutoLogging();

    // Small delay to avoid watchdog triggers on core 0
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  Serial.begin(115200);

  // Initialize storage and sensor
  initStorage();
  long offset = getAbsoluteOffset();

  initWiFi();
  initSensor(offset);
  initWebRoutes(server);

  // Initialize auto logger service
  initAutoLogger();

  // 2. Create task pinned to core 0
  xTaskCreatePinnedToCore(
      WebAndTasksCode, /* 任務函式 */
      "WebAndTasks",   /* 任務名稱 */
      8192,            /* 堆疊大小 (Stack size) */
      NULL,            /* 傳入參數 */
      1,               /* 優先級 */
      &WebTaskHandle,  /* 任務句柄 */
      0                /* 固定在核心 0 */
  );

  Serial.println("[System] System initialization complete, dual-core mode active");
}

void loop()
{
  // Core 1 (default core) focuses on sensor updates
  // There are no delays or file writes here to keep readings responsive
  updateSensor();
}