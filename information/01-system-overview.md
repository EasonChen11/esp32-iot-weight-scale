# 系統總覽與雙核心架構

## 雙核心分工

ESP32 有兩個核心，各自負責不同的工作：

```
┌───────────────────────────────────────────────────────────┐
│                    ESP32 dual-core layout                 │
├───────────────────────────┬───────────────────────────────┤
│      Core 1 (loop)        │    Core 0 (FreeRTOS Task)     │
│                           │                               │
│  updateSensor()           │  server.handleClient()  [Web] │
│    500ms read HX711       │  handleAutoLogging()  [Log]   │
│    store to RAM cache     │  handleMQTT()         [MQTT]  │
│                           │                               │
│  handleOLED()             │  vTaskDelay(10ms)             │
│    200ms refresh          │                               │
│                           │                               │
│  delay(1) -- yield CPU    │                               │
├───────────────────────────┴───────────────────────────────┤
│  shared: volatile float cached_weight1 / cached_weight2   │
└───────────────────────────────────────────────────────────┘

Core 1: sensor polling + OLED (every 500ms read, 200ms refresh)
Core 0: web server + MQTT + auto-logger (10ms tick)
shared variable: cached_weight1/2 (volatile, cross-core safe)
```

## 開機流程 (setup)

```
Serial.begin(115200)
       │
       ▼
initStorage()
  ├─ LittleFS.begin()        ← 掛載 flash 檔案系統
  └─ loadRecordsCache()      ← /records.json → RAM 快取
       │
       ▼
getAbsoluteOffset()          ← NVS 讀取 Sensor 1 校準值
getAbsoluteOffset2()         ← NVS 讀取 Sensor 2 校準值
       │
       ▼
initWiFi()                   ← AP+STA 雙模式
       │
       ▼
initSensor(offset1, offset2) ← HX711 初始化 + 套用校準
       │
       ▼
initWebRoutes(server)        ← 註冊所有 HTTP 路由
initAutoLogger()             ← 記錄開機時間
initMQTT()                   ← 設定 MQTT broker
initOLED()                   ← GPIO 供電 + I2C 初始化
       │
       ▼
xTaskCreatePinnedToCore()    ← 建立 Core 0 任務
       │
       ▼
loop() 開始在 Core 1 執行
```

## 模組一覽

| 模組 | 檔案 | 核心 | 功能 |
|------|------|------|------|
| 感測器 | `sensor_manager.cpp` | Core 1 | 雙 HX711 讀取 + RAM 快取 |
| OLED | `oled_manager.cpp` | Core 1 | SSD1306 顯示自動輪播 |
| Web 伺服器 | `web_server_logic.cpp` + `web_pages.cpp` | Core 0 | HTTP 路由 + 網頁 UI |
| MQTT | `mqtt_manager.cpp` | Core 0 | PubSubClient 發布重量 |
| 自動記錄 | `auto_logger.cpp` | Core 0 | 開機 + 每小時記錄 |
| 儲存 (紀錄) | `storage/littlefs_storage.cpp` | Core 0 | LittleFS + RAM 快取 |
| 儲存 (校準) | `storage/nvs_storage.cpp` | setup | NVS Preferences |
| 儲存 (初始化) | `storage/storage_init.cpp` | setup | 掛載 + 載入快取 |
| WiFi | `wifi_manager.cpp` | setup | AP+STA 連線 |

## 功能開關

所有模組都可透過 `config.h` 的 `#define` 獨立開關：

```
WIFI_ENABLED ──┬── WEB_SERVER_ENABLED
               └── MQTT_ENABLED
OLED_ENABLED
AUTO_LOGGER_ENABLED
SIMULATE_SENSOR
DEEP_SLEEP_ENABLED (未來)
```

相依性：`WEB_SERVER_ENABLED` 和 `MQTT_ENABLED` 需要 `WIFI_ENABLED = true`。
