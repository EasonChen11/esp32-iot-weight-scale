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
  │                          (pool.ntp.org / time.nist.gov)
  │                                │
  ├─ configTime(28800, 0, ...)  ──→│  設定時區 UTC+8 + 伺服器地址
  │                                │
  │       ← NTP response ──────────┤  回傳 UTC 時間
  │                                │
  ├─ ESP32 內部 RTC 設定為           │
  │  UTC+8 的當地時間                │
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
configTime(NTP_GMT_OFFSET_SEC, NTP_DAYLIGHT_OFFSET_SEC, NTP_SERVER1, NTP_SERVER2)
    │
    │  NTP_GMT_OFFSET_SEC     = 28800  (UTC+8 台灣)
    │  NTP_DAYLIGHT_OFFSET_SEC = 0     (台灣無日光節約)
    │  NTP_SERVER1 = "pool.ntp.org"
    │  NTP_SERVER2 = "time.nist.gov"
    │
    ▼
等待同步（每 500ms 檢查一次，最多 10 次 = 5 秒）
    │
    ├─ getLocalTime() 成功
    │       → "[NTP] Time synced: 2026-03-21 14:30:00"
    │
    └─ getLocalTime() 失敗（10 次都未成功）
            → "[NTP] Sync failed — will rely on browser /sync fallback"
```

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

## 深度睡眠模式下的時間持久性

ESP32 的內部 RTC 運行在 RTC 電源域，即使進入深度睡眠也不會斷電：

```
開機 → NTP 同步 → RTC 設定正確時間
  │
  ▼
深度睡眠（RTC 繼續運行，有微小漂移）
  │
  ▼
喚醒 → getLocalTime() 仍可取得近似正確的時間
  │     （每小時漂移約 ±1 秒，通常可接受）
  │
  ▼
initNTP() 再次同步 → 修正漂移
```

注意：RTC 時間在深度睡眠中不需要重新設定 `configTime()`，
因為 `configTime` 主要設定 NTP 自動同步的參數，
而 RTC 本身已經有上次同步的時間。

## NTP vs 瀏覽器 /sync 比較

| 特性 | NTP 同步 | 瀏覽器 /sync |
|------|---------|-------------|
| 自動化 | 全自動 | 需有人開網頁 |
| 精準度 | ±10ms（典型） | ±1s（取決於網路延遲） |
| 網路需求 | STA WiFi + 網路 | AP 或 STA 模式皆可 |
| 深度睡眠支援 | 喚醒後自動重新同步 | 需有人開網頁 |
| 觸發時機 | 開機時（initNTP） | 網頁 onload 事件 |
| 在 config.h | `NTP_ENABLED` | 永遠可用（Web 功能） |

## 無法使用 NTP 的情境

1. **STA WiFi 連不上**（SSID/密碼錯誤、路由器不在範圍內）
2. **僅 AP 模式**（ESP32 在戶外，無路由器）
3. **路由器無網路**（區網環境，無法連到 NTP 伺服器）

以上情境都會自動回退到瀏覽器 `/sync` 機制。

## 設定參數

| 參數 | 值 | 定義位置 | 用途 |
|------|-----|---------|------|
| `NTP_ENABLED` | `true` | config.h | 功能開關 |
| `NTP_SERVER1` | `"pool.ntp.org"` | config.h | 主要 NTP 伺服器 |
| `NTP_SERVER2` | `"time.nist.gov"` | config.h | 備用 NTP 伺服器 |
| `NTP_GMT_OFFSET_SEC` | `28800` | config.h | UTC+8（台灣） |
| `NTP_DAYLIGHT_OFFSET_SEC` | `0` | config.h | 日光節約偏移（台灣=0） |
