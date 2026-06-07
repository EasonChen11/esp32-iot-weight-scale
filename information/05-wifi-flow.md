# WiFi 模組流程 (wifi_manager)

## 連線模式：AP + STA 同時運行

ESP32 同時開啟兩個 WiFi 介面：

```
┌─────────────────────────────────────────────────────────────┐
│                   WiFi.mode(WIFI_AP_STA)                    │
├────────────────────────────┬────────────────────────────────┤
│    AP interface (Soft-AP)  │    STA interface (Station)     │
│                            │                                │
│  SSID: ESP32_Weight_Scale  │  Connects to: YOUR_WIFI_SSID    │
│  Pass: YOUR_AP_PASS         │  Pass: YOUR_WIFI_PASS           │
│  IP: 192.168.4.1 (fixed)   │  IP: DHCP assigned             │
│                            │                                │
│  Purpose:                  │  Purpose:                      │
│  - Phone direct connect    │  - Reach MQTT broker           │
│  - Works without router    │  - Access via home network     │
│  - Always on               │  - NTP time sync               │
│                            │  - Falls back gracefully       │
└────────────────────────────┴────────────────────────────────┘

AP (Soft-AP): phone/device direct connect, always on, IP 192.168.4.1
STA (Station): home WiFi for MQTT + NTP, DHCP IP, fallback if unavailable
```

## 開機 Fallback Chain (#28)

開機時 `initWiFi()` 依序嘗試以下兩組設定，每組 8 秒 timeout：

```
setup()
  ├─ WiFi.softAP(...)  ← AP 立即開啟，永遠不關
  ├─ if NVS 有 SSID:
  │     try connect (8s)
  │     ✓ → 完成
  ├─ try compile-time SSID (8s)
  │     ✓ → 完成
  └─ ✗ → DISCONNECTED (AP 仍在)
```

最壞情況約 16 秒；之後 web server 才啟動（同步阻塞，避免初始化階段的 race condition）。

## Runtime 狀態機 (#28)

```
DISCONNECTED ──/network/save──→ CONNECTING ──成功──→ CONNECTED
                                    │                    │
                                    │                    │ router 斷電
                                    │                    ▼
                                    │              DISCONNECTED
                                    │ 8s timeout
                                    ▼
                                  FAILED
```

唯一寫入點：`processWifiTasks()`，跑在 Core 0 的 `WebAndTasks` 迴圈，每次 loop 迭代呼叫一次。

## NVS 寫入時機 (Strategy 2)

「**只在連線成功後**」才把新的 SSID/密碼寫入 NVS namespace `wifi_cfg`。失敗時 NVS 完全不變，所以使用者放棄離開後，下次重開機仍然會嘗試上次成功的設定（自我修復）。

## /network 子頁面流程

1. 使用者進入 `/network` → 立刻 fetch `/wifi-status` 顯示目前狀態
2. 自動 fetch `/network/scan` → 顯示周圍 SSID 清單（含 RSSI、加密狀態）
3. 使用者點選 SSID → 輸入密碼 → 按 Connect
4. 後端 `requestStaChange()` → 立刻回 `202 connecting` → 前端進入 1Hz polling
5. polling 看到 `connected` → 顯示綠色成功訊息；看到 `failed` 或 10 秒超時 → 顯示紅色錯誤訊息

額外功能：「Clear Stored Network」按鈕清空 NVS，但不影響當下連線；下次 reboot 才走 compile-time fallback。

## Spam-click 防護 (4 層)

| 層 | 機制 |
|---|---|
| 1 | 前端：`btn.disabled = true` 同步生效，同 tab 物理上無法重複觸發 |
| 2 | 後端：`g_wifiOpBusy` 旗標，並發 `/network/save` 或 `/network/scan` 回 409 |
| 3 | Reload 期間：`window.onload` 先 fetch `/wifi-status`，看到 `connecting` 就預先 disable button + 接手 polling |
| 4 | 409 → 前端轉成 resume polling，不顯示錯誤，使用者不會看到「卡住」狀態 |

## Heartbeat 狀態指示

主頁右上角小圓點，每 5 秒 polling `/wifi-status`：

| 顏色 | 動畫 | 意義 |
|---|---|---|
| 綠色 | 1.5s 慢閃 (heartbeat) | CONNECTED |
| 黃色 | 0.5s 快閃 | CONNECTING |
| 灰色 | 靜止 | DISCONNECTED |
| 紅色 | 靜止 | FAILED |

## 錯誤處理摘要

| 情境 | 系統行為 |
|---|---|
| NVS 為空 | 跳過 NVS 嘗試，直接試 compile-time |
| NVS SSID 連不上 (8s timeout) | 自動 fallback 到 compile-time |
| 兩個都連不上 | 進入 AP-only 狀態，網頁仍可用，使用者可進 /network 重新設定 |
| 使用者打錯密碼 | 顯示 inline 紅色錯誤；NVS 不變；password 欄清空可立即重試 |
| 連線中重複按 Connect | 前端 button 立即 disable；後端 409 |
| 兩個 tab 同時送出 | 第一個 tab 收 202；第二個 tab 收 409 + `current_ssid` 並接手 polling |
| router 中途斷電 | heartbeat 變灰；ESP32 自動重連，恢復後 heartbeat 變回綠 |
| 切 SSID 後 NTP | 自動觸發 non-blocking re-sync (`triggerNtpResync` via `g_pendingNtpSync` flag) |
| Clear Stored Network | 清 NVS；當下連線完全不變；下次 reboot 走 compile-time fallback |

## 相關檔案

- `include/config.h` — `WIFI_CONFIG_ENABLED`、`WIFI_CONNECT_TIMEOUT_MS`
- `include/wifi_manager.h` / `src/wifi_manager.cpp` — 狀態機、NVS chain、scan/status 助手
- `include/storage/nvs_storage.h` / `src/storage/nvs_storage.cpp` — `wifi_cfg` namespace 的 4 個函式
- `src/web_server_logic.cpp` — `/network`, `/wifi-status`, `/network/scan`, `/network/save`, `/network/clear`
- `src/web_pages.cpp` — `getNetworkPageHTML()` + 主頁 heartbeat indicator
- `src/main.cpp` — `WebAndTasks` 迴圈呼叫 `processWifiTasks()`

## 相關 issue

#28 — Add runtime WiFi configuration via web UI
