# 深度睡眠模組流程 (deep_sleep_manager)

## 功能

讓 ESP32 在清醒一段時間後自動進入深度睡眠，並在指定時間或按下按鈕時喚醒。
適合戶外電池供電場景，大幅降低功耗。

## 喚醒方式

```
┌─────────────────────────────────────────────────────────┐
│                    喚醒來源                              │
├────────────────────────────┬────────────────────────────┤
│    Timer（定時喚醒）         │    Button（按鈕喚醒）        │
│                            │                            │
│  esp_sleep_enable_timer_   │  esp_sleep_enable_ext0_    │
│  wakeup(sleepUs)           │  wakeup(GPIO 32, LOW)      │
│                            │                            │
│  需要 SCHEDULE_ENABLED      │  永遠可用                   │
│  + 時間已同步（NTP/sync）    │  按下按鈕即喚醒               │
│  + 有排程項目                │                            │
│                            │                            │
│  自動計算到下個排程的秒數      │  使用者手動觸發               │
└────────────────────────────┴────────────────────────────┘
```

## 按鈕硬體接線

```
ESP32 GPIO 32 (INPUT_PULLUP) ──→ 按鈕一端
ESP32 GPIO 33 (OUTPUT LOW)   ──→ 按鈕另一端（當作 GND）

按鈕開路 → GPIO 32 = HIGH（內部上拉）
按鈕按下 → GPIO 32 = LOW → 觸發 ext0 喚醒

GPIO 33 用作按鈕 GND，因為：
- 按鈕消耗電流極小（µA 等級）
- GPIO 可安全提供此電流
- 不佔用實體 GND 腳位
```

## 初始化流程

```
initDeepSleep()
    │
    ▼
pinMode(WAKE_BTN_GND=33, OUTPUT)
digitalWrite(33, LOW)            ← 按鈕 GND
    │
    ▼
pinMode(WAKE_BTN_PIN=32, INPUT_PULLUP)
esp_sleep_enable_ext0_wakeup(GPIO 32, LOW)  ← 設定按鈕喚醒
    │
    ▼
bootTimeMs = millis()            ← 開始計時
sleepTriggered = false
    │
    ▼
偵測喚醒原因：
esp_sleep_get_wakeup_cause()
    │
    ├─ ESP_SLEEP_WAKEUP_TIMER → "[SLEEP] Woke up by timer (scheduled)"
    ├─ ESP_SLEEP_WAKEUP_EXT0  → "[SLEEP] Woke up by button press"
    └─ 其他                    → "[SLEEP] Woke up by power-on / reset"
    │
    ▼
Serial: "[SLEEP] Will stay awake for 600000 ms"
```

## 入睡流程 (Core 0, 每 10ms 被呼叫)

```
handleDeepSleep()
    │
    ├─ sleepTriggered == true ? → return（已觸發，避免重複）
    │
    ├─ millis() - bootTimeMs < AWAKE_DURATION_MS (600000) ?
    │       └─ Yes: return（清醒時間未到）
    │
    ▼ 清醒時間已到（10 分鐘）
sleepTriggered = true
    │
    ├─── SCHEDULE_ENABLED ?
    │         │
    │    Yes: getNextWakeupSeconds()
    │         │
    │         ├─ > 0: 有排程
    │         │       sleepUs = seconds * 1,000,000
    │         │       esp_sleep_enable_timer_wakeup(sleepUs)
    │         │       "[SLEEP] Timer wake-up set: X seconds"
    │         │
    │         └─ -1: 沒排程或時間未同步
    │                "[SLEEP] No schedule entries or time not synced"
    │                （僅按鈕喚醒）
    │
    │    No:  "[SLEEP] No schedule module — button wake-up only"
    │
    ▼
powerDownSensors()
  → scale1.power_down()  ← SCK=HIGH, HX711 省電模式 (~1µA)
  → scale2.power_down()
gpio_hold_en(LOADCELL1_SCK_PIN)  ← 鎖住 SCK HIGH
gpio_hold_en(LOADCELL2_SCK_PIN)
    │
    ▼
Serial: "[SLEEP] Entering deep sleep now..."
Serial.flush()
delay(100)          ← 確保序列輸出完成
esp_deep_sleep_start()
```

## 完整運作週期

```
┌─────────────────────────────────────────────────────────────┐
│                     一個完整的睡眠/喚醒週期                    │
└─────────────────────────────────────────────────────────────┘

喚醒（Timer 或 Button）
  │
  ▼
setup()
  ├─ initStorage()        ← 掛載 LittleFS + 載入紀錄快取
  ├─ getAbsoluteOffset()  ← NVS 讀取校準值
  ├─ initWiFi()           ← AP+STA 連線
  ├─ initNTP()            ← NTP 同步時間
  ├─ initSensor()         ← HX711 初始化
  ├─ initWebRoutes()      ← 註冊 HTTP 路由
  ├─ initAutoLogger()     ← 開始記錄服務
  ├─ initMQTT()           ← MQTT 連線
  ├─ initOLED()           ← OLED 初始化
  ├─ initSchedule()       ← 載入排程
  └─ initDeepSleep()      ← 設定喚醒源 + 開始計時
  │
  ▼
Core 0 任務開始
  ├─ Web server 處理請求
  ├─ Auto-logger 10 秒後記錄開機數據
  ├─ MQTT 每 5 秒發布
  └─ handleDeepSleep() 監控清醒時間
  │
  │  ← 10 分鐘 →
  │
  ▼
handleDeepSleep() 觸發入睡
  ├─ 計算下次排程: getNextWakeupSeconds()
  ├─ 設定 timer 喚醒
  └─ esp_deep_sleep_start()
  │
  │  ← 深度睡眠（功耗 ~10µA）→
  │
  ▼
Timer 到期 → 重新開機（回到頂部）
```

## 邊界情境處理

```
情境 1：有排程 + 時間已同步
  → 計算到最近排程的秒數 → 設定 timer → 入睡
  → 時間到 → 自動喚醒

情境 2：有排程但時間未同步（NTP 失敗 + 沒人開網頁）
  → getNextWakeupSeconds() return -1
  → 不設定 timer → 僅按鈕喚醒
  → 按按鈕才能喚醒

情境 3：沒有排程項目
  → getNextWakeupSeconds() return -1
  → 僅按鈕喚醒

情境 4：SCHEDULE_ENABLED = false
  → 不呼叫 getNextWakeupSeconds()
  → 僅按鈕喚醒

情境 5：所有排程時間都已過（今天）
  → delta 為負 → 加 86400（24小時）→ 算到明天
  → 例：現在 20:00，排程 07:00 → 明天 07:00（11 小時後）
```

## 深度睡眠期間的狀態

```
┌──────────────────────────────────────────────────┐
│              深度睡眠期間                          │
├──────────────┬───────────────────────────────────┤
│  保持運行     │  關閉                              │
│              │                                   │
│  RTC 時鐘    │  CPU（兩個核心）                     │
│  RTC 記憶體  │  WiFi                              │
│  ext0 喚醒   │  Bluetooth                        │
│  timer 喚醒  │  所有 GPIO（除 RTC GPIO）           │
│  SCK hold    │  OLED                             │
│  (HX711省電) │  HX711 感測器（已 power_down）      │
│              │  所有 RAM（volatile 變數全部消失）    │
├──────────────┴───────────────────────────────────┤
│  功耗：~10 µA + HX711 ~2 µA（vs 正常運行 ~80 mA）   │
│  喚醒後：完整重開機（setup() 重新執行）               │
└──────────────────────────────────────────────────┘
```

## 設定參數

| 參數 | 值 | 定義位置 | 用途 |
|------|-----|---------|------|
| `DEEP_SLEEP_ENABLED` | `false` (預設) | config.h | 功能開關 |
| `WAKE_BTN_PIN` | `32` | config.h | 喚醒按鈕 GPIO（ext0） |
| `WAKE_BTN_GND` | `33` | config.h | 按鈕 GND GPIO |
| `AWAKE_DURATION_MS` | `600000` (10 分鐘) | config.h | 每次喚醒的清醒時間 |
