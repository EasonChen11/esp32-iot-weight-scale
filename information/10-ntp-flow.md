# NTP 時間同步模組流程 (wifi_manager — initNTP)

## 功能

ESP32 沒有 RTC 電池，開機後不知道現在幾點。
NTP (Network Time Protocol) 讓 ESP32 自動從網路伺服器取得正確時間。

## 前置條件

- `NTP_ENABLED = true`（config.h）
- `WIFI_ENABLED = true`（config.h）
- STA WiFi 連線成功（有網路存取）

## 運作原理

```
ESP32                          NTP 伺服器
  │                          (216.239.35.0 / time.google.com / pool.ntp.org)
  │                                │
  ├─ configTime(28800, 0, ...)  ──→│  設定時區 UTC+8 + 伺服器地址
  │                                │
  │       ← NTP response ──────────┤  回傳 UTC 時間
  │                                │
  ├─ ntpSyncCallback() 觸發        │  ESP-IDF 收到回應後主動通知
  │  _timeSynced = true             │
  │                                │
  ▼
getLocalTime() 開始回傳正確時間
```

## 初始化流程

```
initNTP()
    │
    ├─ WiFi.status() != WL_CONNECTED ?
    │       └─ Yes: "[NTP] Skipped — no STA connection"
    │               return（跳過 NTP）
    │
    ▼ STA 已連線
getLocalTime(&timeinfo, 0)   ← timeout=0，不阻塞
    │  印出 RTC 舊時間（debug 用，觀察漂移量）
    │
    ▼
sntp_set_time_sync_notification_cb(ntpSyncCallback)  ← 註冊回呼函式
    │
    ▼
sntp_set_sync_status(SNTP_SYNC_STATUS_RESET)  ← 重置同步狀態
    │  （深度睡眠醒來可能繼承上次的 COMPLETED 狀態，必須先清除）
    │
    ▼
configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2, NTP_SERVER3)
    │
    │  NTP_GMT_OFFSET_SEC      = 28800  (UTC+8 台灣)
    │  NTP_DAYLIGHT_OFFSET_SEC = 0      (台灣無日光節約)
    │  NTP_SERVER1 = "216.239.35.0"     (time.google.com IP，避免 DNS)
    │  NTP_SERVER2 = "time.google.com"
    │  NTP_SERVER3 = "pool.ntp.org"
    │
    ▼
等待 callback（每 500ms 檢查 _timeSynced，最多 60 次 = 30 秒）
    │
    ├─ _timeSynced == true（callback 已觸發）
    │       → "[NTP] Time synced: 2026-03-30 22:11:02"
    │
    └─ 60 次都未觸發
            → "[NTP] Sync failed after 30s — RTC time preserved as fallback"
```

### 為什麼用 callback 而不是 polling sntp_get_sync_status()

實測發現 `sntp_get_sync_status()` 的 polling loop 在 ESP32 上會提早結束
（設定 60 次迴圈但只跑 3~10 次，原因不明）。
改用 `sntp_set_time_sync_notification_cb()` 後，ESP-IDF 收到 NTP 回應時
主動呼叫 callback 設定 `volatile bool _timeSynced = true`，完全可靠。

## 時區設定

```
┌──────────────────────────────────────────────────┐
│  configTime(GMT_offset, DST_offset, server...)   │
│                                                  │
│  GMT_offset = 28800 秒 = 8 小時 = UTC+8          │
│  DST_offset = 0 秒（台灣無夏令時間）               │
│                                                  │
│  NTP 回傳 UTC 時間 + offset = 台灣本地時間          │
│  例：UTC 06:30:00 + 8h = 14:30:00 (台灣)          │
└──────────────────────────────────────────────────┘
```

## 時間校時完整組合分析

### 所有校時來源

| 來源 | 觸發方式 | 精準度 | 需要條件 |
|------|---------|--------|---------|
| NTP | 開機自動 | ±10ms | STA WiFi + 網路 |
| 瀏覽器 `/sync` | 網頁 onload | ±1s | AP 或 STA 模式，需有人開網頁 |
| RTC 舊時間 | 深度睡眠保留 | 有漂移 | 之前曾成功校時 |

### 所有開機情境與時間狀態

```
開機
 ├─ WiFi STA 連線成功
 │   ├─ NTP 成功 ✅
 │   │   → _timeSynced = true, timeReliable = true
 │   │   → 瀏覽器 /sync → 跳過（"NTP already synced"）
 │   │
 │   └─ NTP 失敗（30秒超時）
 │       ├─ 深度睡眠醒來，RTC year > 2024
 │       │   → _timeSynced = false, timeReliable = true（接受 RTC 舊時間）
 │       │   → 瀏覽器 /sync → 接受，覆蓋 RTC，設 _timeSynced = true
 │       │
 │       └─ 首次開機，RTC 無時間
 │           → _timeSynced = false, timeReliable = false
 │           → 等待 /sync 或 60 秒超時後存 "no-sync" 記錄
 │           → 瀏覽器 /sync → 接受，設定時間
 │
 └─ WiFi STA 連線失敗（AP only）
     ├─ NTP 跳過
     │   ├─ 深度睡眠醒來，RTC year > 2024
     │   │   → timeReliable = true（用 RTC 舊時間）
     │   │
     │   └─ 首次開機，無時間
     │       → timeReliable = false → 等 /sync 或 60 秒超時
     │
     └─ 瀏覽器 /sync 是唯一校時方式
```

### `timeReliable` 判定邏輯（auto_logger.cpp）

```cpp
#if NTP_ENABLED
    bool timeReliable = (hasTime && isTimeSynced())
                     || (hasTime && timeinfo.tm_year > (2024 - 1900));
#else
    bool timeReliable = hasTime;
#endif
```

| 條件 | `hasTime` | `isTimeSynced()` | `tm_year > 2024` | `timeReliable` |
|------|:---------:|:----------------:|:-----------------:|:--------------:|
| NTP 成功 | true | true | true | **true** |
| NTP 失敗 + RTC 有舊時間 | true | false | true | **true** |
| NTP 失敗 + RTC 無時間 | false | false | false | **false** |
| 瀏覽器 /sync 成功 | true | true | true | **true** |
| NTP 關閉 + getLocalTime 有值 | true | N/A | N/A | **true** |

### `_timeSynced` 狀態變化

| 事件 | `_timeSynced` | 觸發點 |
|------|:---:|------|
| 開機初始值 | `false` | `static volatile bool` 宣告 |
| NTP callback 觸發 | → `true` | `ntpSyncCallback()` |
| `/sync` 且 NTP 未同步 | → `true` | `setTimeSynced(true)` |
| `/sync` 且 NTP 已同步 | 不變 `true` | 跳過，不覆蓋 NTP 時間 |
| 深度睡眠後重開機 | `false` | 變數重新初始化 |

## Auto-logger 時間超時保護

當 `timeReliable = false` 持續超過 `TIME_SYNC_TIMEOUT_MS`（60 秒）時：

```
開機
  │
  ├─ 10 秒後：開始嘗試存記錄
  │   └─ timeReliable = false → 等待校時...
  │
  ├─ 每 5 秒印一次等待訊息
  │
  └─ 60 秒後：強制存記錄
      → date = "no-sync", time = "no-sync"
      → 確保重量資料不遺失
      → Google Sheets sync 也能觸發（isStartupRecordDone = true）
```

## Deep sleep 與時間的交互

### RTC 時間漂移

ESP32 的內部 RTC 運行在 RTC 電源域，深度睡眠中不斷電：

```
開機 → NTP 同步 → RTC 設定正確時間
  │
  ▼
深度睡眠（RTC 繼續運行，使用 150kHz RC 振盪器，精度約 ±5%）
  │
  ▼
喚醒 → RTC 保留舊時間（但有漂移）
  │
  ▼
initNTP() 嘗試重新同步
  │
  ├─ NTP 成功 → 修正漂移，_timeSynced = true
  └─ NTP 失敗 → 保留 RTC 舊時間（timeReliable = true，因 year > 2024）
```

### RTC 漂移量估算（150kHz RC 振盪器，±5%）

| 睡眠時長 | 最大漂移 |
|----------|---------|
| 1 小時   | ±3 分鐘 |
| 6 小時   | ±18 分鐘 |
| 12 小時  | ±36 分鐘 |
| 24 小時  | ±72 分鐘 |

### Deep sleep 喚醒策略

```
handleDeepSleep()    ← 10 分鐘清醒期結束
    │
    ├─ getNextWakeupSeconds() > 0（排程有效 + 有時間）
    │   ├─ isTimeSynced() = true  → 按排程喚醒
    │   └─ isTimeSynced() = false → 提早 2 分鐘喚醒（補償 RTC 漂移）
    │
    └─ getNextWakeupSeconds() ≤ 0（無排程 或 無時間）
        └─ 使用 FALLBACK_WAKEUP_SEC（1 小時）定期喚醒
            → 避免裝置永遠沉睡
            → 下次開機重試 NTP
```

### 為什麼需要 fallback 喚醒

```
情境：ESP32 部署在戶外，NTP 失敗
  │
  ├─ 舊行為：getNextWakeupSeconds() = -1 → 只能按鈕喚醒 ❌
  │          （無人按鈕 = 永遠沉睡）
  │
  └─ 新行為：fallback 1 小時喚醒 → 重試 NTP → 恢復正常排程 ✅
```

## NTP vs 瀏覽器 /sync 比較

| 特性 | NTP 同步 | 瀏覽器 /sync |
|------|---------|-------------|
| 自動化 | 全自動 | 需有人開網頁 |
| 精準度 | ±10ms（典型） | ±1s（取決於網路延遲） |
| 網路需求 | STA WiFi + 網路 | AP 或 STA 模式皆可 |
| 深度睡眠支援 | 喚醒後自動重新同步 | 需有人開網頁 |
| 觸發時機 | 開機時（initNTP） | 網頁 onload 事件 |
| NTP 已同步時 | N/A | 跳過，不覆蓋 NTP 時間 |
| 在 config.h | `NTP_ENABLED` | 永遠可用（Web 功能） |

## 設定參數

| 參數 | 值 | 定義位置 | 用途 |
|------|-----|---------|------|
| `NTP_ENABLED` | `true` | config.h | 功能開關 |
| `NTP_SERVER1` | `"216.239.35.0"` | config.h | Google NTP（IP，避免 DNS） |
| `NTP_SERVER2` | `"time.google.com"` | config.h | Google NTP（hostname） |
| `NTP_SERVER3` | `"pool.ntp.org"` | config.h | NTP Pool 備用 |
| `NTP_GMT_OFFSET_SEC` | `28800` | config.h | UTC+8（台灣） |
| `NTP_DAYLIGHT_OFFSET_SEC` | `0` | config.h | 日光節約偏移（台灣=0） |
| `TIME_SYNC_TIMEOUT_MS` | `60000` | config.h | Auto-logger 最大等待校時時間 |
| `FALLBACK_WAKEUP_SEC` | `3600` | config.h | 無時間時的 fallback 喚醒間隔 |
