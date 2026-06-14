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
│                           │  handleDeepSleep()    [Sleep] │
│  handleOLED()             │                               │
│    200ms refresh          │  vTaskDelay(10ms)             │
│                           │                               │
│  delay(1) -- yield CPU    │                               │
├───────────────────────────┴───────────────────────────────┤
│  shared: volatile float cached_weight1 / cached_weight2   │
└───────────────────────────────────────────────────────────┘

Core 1: sensor polling + OLED (every 500ms read, 200ms refresh)
Core 0: web server + MQTT + auto-logger + deep-sleep (10ms tick)
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
initNTP()                    ← NTP 時間同步（需 STA 連線）
       │
       ▼
initSensor(offset1, offset2) ← HX711 初始化 + 套用校準
       │
       ▼
initWebRoutes(server)        ← 註冊所有 HTTP 路由（含排程端點）
initAutoLogger()             ← 記錄開機時間
initMQTT()                   ← 設定 MQTT broker
initOLED()                   ← GPIO 供電 + I2C 初始化
initSchedule()               ← 從 NVS 載入喚醒排程
initDeepSleep()              ← 設定按鈕喚醒 + 記錄喚醒原因
initOTA()                    ← 驗證執行中 image（OTA 首次開機 rollback 保護）
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
| WiFi | `wifi_manager.cpp` | setup | AP+STA 連線 + NTP 同步 |
| NTP | `wifi_manager.cpp` | setup | 網路時間同步（UTC+8） |
| 排程 | `schedule_manager.cpp` | setup/Web | NVS 喚醒排程管理 |
| 深度睡眠 | `deep_sleep_manager.cpp` | Core 0 | 定時/按鈕喚醒 + 自動入睡 |
| OTA | `ota_manager.cpp` | Core 0 | GitHub Releases HTTPS 韌體更新 |

## 功能開關

所有模組都可透過 `config.h` 的 `#define` 獨立開關：

| Switch | 預設 | 開啟效果 | 關閉效果 |
|--------|------|----------|----------|
| `WIFI_ENABLED` | `true` | 啟動 AP 熱點 + STA 連線 | **全部網路功能關閉**（Web、MQTT、NTP 都不可用） |
| `WEB_SERVER_ENABLED` | `true` | 啟動 port 80 Web UI（秤重面板、校正、排程管理、`/sync` 時間同步） | 無 Web 介面，只能靠 OLED 看數值 |
| `MQTT_ENABLED` | `false` | 每 5 秒發佈 sensor1/sensor2/total 到 MQTT broker | 不發佈，省去需要架 broker 的麻煩 |
| `AUTO_LOGGER_ENABLED` | `true` | 開機 10 秒後記錄一筆、之後每小時記錄一筆到 LittleFS | 不自動記錄，手動從 Web 按才會記 |
| `OLED_ENABLED` | `true` | SSD1306 螢幕顯示重量，自動輪播 Total → S1 → S2 | 不驅動 OLED，省電 |
| `SIMULATE_SENSOR` | `false` | 不讀 HX711，用亂數模擬重量（S1≈25kg, S2≈22kg ±0.5kg） | 讀真實 HX711 感測器 |
| `NTP_ENABLED` | `true` | STA 連線成功時自動從 NTP server 同步時間 | 不同步，時間靠 `/sync` 或從 0 開始 |
| `SCHEDULE_ENABLED` | `true` | 可設定每日喚醒時間（NVS 儲存），deep sleep 時用來計算下次喚醒 | 無排程功能，deep sleep 只能靠按鈕喚醒 |
| `DEEP_SLEEP_ENABLED` | `false` | 開機 10 分鐘後自動進入深度睡眠（~10µA），入睡前自動 power down HX711 省電，靠排程/按鈕喚醒 | 永遠保持運行不休眠 |
| `OTA_ENABLED` | `true` | 開機/喚醒時連網後自動檢查 GitHub latest release，有新版就下載驗證 sha256 後寫入備用分區並重開機 | 不檢查更新，只能 USB 燒錄 |

### 依賴關係圖

```
WIFI_ENABLED ─┬─► WEB_SERVER_ENABLED
              ├─► MQTT_ENABLED
              ├─► NTP_ENABLED
              └─► OTA_ENABLED（需 STA 連到網際網路）

SCHEDULE_ENABLED ──► DEEP_SLEEP_ENABLED（排程才有意義）

NTP_ENABLED ──► SCHEDULE_ENABLED（時間準確排程才準）
```

### 需要一起開才有意義的組合

| 組合 | 原因 |
|------|------|
| `WIFI_ENABLED` + `WEB_SERVER_ENABLED` | Web 需要 WiFi，關 WiFi 開 Web 沒用 |
| `WIFI_ENABLED` + `MQTT_ENABLED` | MQTT 需要 STA 連到 broker，關 WiFi 開 MQTT 沒用 |
| `WIFI_ENABLED` + `NTP_ENABLED` | NTP 需要 STA 連網取得時間 |
| `WIFI_ENABLED` + `OTA_ENABLED` | OTA 需要 STA 連到網際網路存取 GitHub Releases；關 WiFi 開 OTA 不會觸發任何更新檢查 |
| `SCHEDULE_ENABLED` + `DEEP_SLEEP_ENABLED` | 排程是給 deep sleep 計算下次喚醒用的；不開 deep sleep 排程不會觸發動作 |
| `NTP_ENABLED` + `SCHEDULE_ENABLED` | 排程靠 `getLocalTime()` 計算秒數；沒 NTP 也沒 `/sync` 時間會錯 |

### 單獨開也 OK 的

| Switch | 說明 |
|--------|------|
| `OLED_ENABLED` | 獨立運作，不依賴任何其他功能 |
| `AUTO_LOGGER_ENABLED` | 獨立寫 LittleFS，但沒有 Web 就看不到紀錄 |
| `SIMULATE_SENSOR` | 純軟體模擬，不需要接 HX711 硬體即可測試 |
| `DEEP_SLEEP_ENABLED` 不開 `SCHEDULE` | 可以運作，但只能靠**按鈕喚醒**，沒有定時喚醒 |

## 深度睡眠模式下的運作週期

當 `DEEP_SLEEP_ENABLED = true` 時，系統自動在清醒一段時間後進入深度睡眠：

```
開機 → WiFi → NTP 同步 → 感測器初始化 → Web 伺服器
  → Auto-logger 記錄資料
  → 清醒 10 分鐘（AWAKE_DURATION_MS）
  → 計算下一個排程時間 → 深度睡眠
  → 計時器到期 → 開機（重複）
  或
  → 按鈕按下 → 開機（重複）
```
