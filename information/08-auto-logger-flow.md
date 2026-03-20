# 自動記錄模組流程 (auto_logger)

## 功能

自動在兩個時機記錄重量到 LittleFS：
1. **開機紀錄** — 開機後等待穩定 + 時間同步，記錄一次
2. **每小時紀錄** — 每個整點記錄一次

## 初始化

```
initAutoLogger()
    │
    ▼
bootTime = millis()          ← 記住開機時間
startupRecordDone = false    ← 標記：開機紀錄尚未完成
Serial: "Service initialized. Waiting for stability and time sync..."
```

## 主循環 (Core 0, 每 10ms 被呼叫)

```
handleAutoLogging()
    │
    ├─── Logic A: 開機紀錄
    │    │
    │    ├─ startupRecordDone == true ? → 跳過
    │    │
    │    ├─ millis() - bootTime < 10000ms ? → 跳過（等待穩定）
    │    │
    │    ├─ 時間已同步 (getLocalTime) ?
    │    │       │
    │    │  No:  每 5 秒印一次 "Waiting for time sync..."
    │    │       └─ return（不記錄）
    │    │       │
    │    │  Yes: getCachedWeight() → 讀取總重量
    │    │       getLogTimestamp() → "14:30:00"
    │    │       addRecordToStorage(time, weight) → RAM + Flash
    │    │       startupRecordDone = true
    │    │       Serial: "Initial record saved with synced time"
    │    │
    │    │
    └─── Logic B: 每小時紀錄
         │
         ├─ 時間未同步 → 跳過
         │
         ├─ timeinfo.tm_hour == lastRecordedHour ? → 跳過（同一小時）
         │
         ├─ lastRecordedHour == -1 ?
         │       └─ Yes: 首次校準 → lastRecordedHour = 現在的小時 → return
         │              （不記錄，只是設定起始點）
         │
         ▼ 小時變了
         lastRecordedHour = 新的小時
         getCachedWeight() → 讀取總重量
         getLogTimestamp() → "15:00:00"
         addRecordToStorage(time, weight) → RAM + Flash
         Serial: "Hourly record saved"
```

## 時間戳取得

```
getLogTimestamp()
    │
    ├─ getLocalTime(&timeinfo) 成功 ?
    │       │
    │  Yes: strftime → "14:30:00"
    │       │
    │  No:  "Boot-" + millis()/1000 + "s" → "Boot-45s"
    │
    ▼
  回傳時間字串
```

## 時間同步來源

ESP32 沒有 RTC 電池，開機時不知道現在幾點。時間來源：

```
瀏覽器開啟網頁
    │
    ▼
window.onload → fetch('/sync?t=' + Math.floor(Date.now()/1000))
    │
    ▼
ESP32 收到 /sync?t=1711000000
    │
    ▼
settimeofday({tv_sec: 1711000000})  ← 設定系統時鐘
    │
    ▼
之後 getLocalTime() 就會回傳正確時間
    │
    ▼
Auto-logger 偵測到時間已同步 → 開始記錄
```

## 完整時間線

```
開機 (t=0)
  │
  ▼
  initAutoLogger()
  bootTime = millis()
  │
  │  等待 10 秒（STARTUP_RECORD_DELAY_MS）
  │
  ▼ (t=10s)
  條件滿足：延遲已過
  但時間可能還沒同步 → 持續等待
  │
  │  使用者開啟網頁 → /sync 同步時間
  │
  ▼ (t=12s, 時間已同步)
  記錄開機紀錄 ← addRecordToStorage("14:30:12", "24.235")
  startupRecordDone = true
  │
  │  lastRecordedHour 初始化為 14（首次不記錄）
  │
  ▼ (t=15:00:00, 整點)
  timeinfo.tm_hour = 15 ≠ lastRecordedHour(14)
  記錄每小時紀錄 ← addRecordToStorage("15:00:00", "24.100")
  │
  ▼ (t=16:00:00)
  記錄每小時紀錄
  │
  ... 每小時重複
```

## 紀錄格式

儲存到 LittleFS `/records.json`：

```json
[
  {"time": "15:00:00", "weight": "24.100"},
  {"time": "14:30:12", "weight": "24.235"},
  ...最多 10 筆，最新在最前面
]
```

## 設定參數

| 參數 | 值 | 用途 |
|------|-----|------|
| `STARTUP_RECORD_DELAY_MS` | 10000 (10 秒) | 開機後等待感測器穩定 |
| `AUTO_RECORD_INTERVAL_MS` | 3600000 (1 小時) | 定義在 config.h 但目前用 tm_hour 判斷 |
| `MAX_RECORDS` | 10 | 最多保留幾筆紀錄 |
