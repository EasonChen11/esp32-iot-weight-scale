#include <Arduino.h>
#include "wifi_manager.h"
#include "sensor_manager.h"
#include "web_server_logic.h"
#include "storage_manager.h"
#include "auto_logger.h"

WebServer server(80);

// 1. 定義任務控制句柄 (Task Handle)
TaskHandle_t WebTaskHandle = NULL;

/**
 * 核心 0 的任務函式：處理 WebServer 與 自動紀錄
 * 這些任務涉及 WiFi 封包處理與 LittleFS 檔案寫入，較為耗時
 */
void WebAndTasksCode(void *pvParameters)
{
  Serial.printf("[System] Web & Tasks 運行於核心: %d\n", xPortGetCoreID());

  for (;;)
  {
    // 處理網頁請求
    server.handleClient();

    // 處理每小時/啟動 10 秒的紀錄 (涉及檔案 I/O)
    handleAutoLogging();

    // 關鍵：給予微小的延遲，防止核心 0 的看門狗 (Watchdog) 報錯
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

void setup()
{
  Serial.begin(115200);

  // 初始化儲存與感測器
  initStorage();
  long offset = getAbsoluteOffset();

  initWiFi();
  initSensor(offset);
  initWebRoutes(server);

  // 初始化自動紀錄服務
  initAutoLogger();

  // 2. 建立任務並固定在核心 0 (Core 0)
  xTaskCreatePinnedToCore(
      WebAndTasksCode, /* 任務函式 */
      "WebAndTasks",   /* 任務名稱 */
      8192,            /* 堆疊大小 (Stack size) */
      NULL,            /* 傳入參數 */
      1,               /* 優先級 */
      &WebTaskHandle,  /* 任務句柄 */
      0                /* 固定在核心 0 */
  );

  Serial.println("[System] 系統初始化完成，已進入雙核心模式");
}

void loop()
{
  // 核心 1 (預設核心) 專心執行感測器更新
  // 這裡沒有任何 delay 或 檔案寫入，讀數反應會變得極快
  updateSensor();
}