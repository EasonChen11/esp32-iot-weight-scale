# Dev Mode 模組流程 (dev_mode + serial console + SSR + tick 偵測)

## 設計動機

原始的 `/clear-records` 端點在 `#if SIMULATE_SENSOR` 條件下會同時重置 NVS 的 record ID 計數器。這個行為若在正式出貨的裝置上意外觸發，會導致新記錄的 ID 從 1 重新開始；由於 Google Apps Script (GAS) 端依 ID 做 dedup，舊的 ID-1 row 仍在 Sheet 上，新上傳的 ID-1 資料會被 GAS 靜默 ack 為「已收到」但實際上沒有寫入新 row，造成 **silent data loss**。

解決方向是將破壞性動作（ID 重置、清 WiFi 憑證）隔離到只有連著 USB 線的開發者才能觸發的 **Developer Mode**，並確保裝置重啟後一定回到 USER 狀態，防止意外出貨。

---

## 架構總覽

```
Serial input (USB 115200)
       │
       ▼
handleSerialModeCommand()
  ├─ "dev-on"     → setDevMode(true)
  ├─ "dev-off"    → setDevMode(false)
  └─ "dev-status" → Serial.printf(...)
              │
              ▼
       RAM bool g_devMode
              │
       ┌──────┴──────────────────────────────────────────┐
       │                                                  │
       ▼                                                  ▼
requireDevMode(server)                          SSR 渲染 (getIndexHTML / getNetworkPageHTML)
  ├─ false → HTTP 403                             ├─ dev=true  → 插入 {{DEV_BANNER}}、按鈕佔位符
  └─ true  → 繼續執行                              └─ dev=false → 佔位符替換為空字串
              │
              ▼
       /tick 回應中加入 "dev":true/false
              │
              ▼
       瀏覽器 JS 偵測 data.dev !== PAGE_RENDERED_DEV
              │
              ▼
       location.reload() → 重取 SSR HTML，dev 按鈕出現或消失
```

---

## 狀態與生命週期

| 屬性 | 值 |
|------|----|
| 儲存位置 | RAM 的 `static bool g_devMode`（`src/dev_mode.cpp`） |
| 預設值 | `false`（USER）——每次 boot 後固定如此 |
| 持久化 | **不寫 NVS**，重啟永遠回到 USER |
| 設計意圖 | 防止開著 DEV 出貨；只有連著 USB 的開發者才能切換 |

---

## Serial 介面

### 指令一覽

| 指令 | 效果 | 回應範例 |
|------|------|---------|
| `dev-on` | 切換至 DEVELOPER 模式 | `[Mode] DEVELOPER ON` |
| `dev-off` | 切換回 USER 模式 | `[Mode] USER` |
| `dev-status` | 印出目前模式（不切換） | `[Mode] Current: USER` |

未知指令會被靜默忽略（不輸出錯誤）。

### Boot Banner

裝置啟動時 `printBootModeBanner()` 會印出：

```
[Mode] Current: USER  (type "dev-on" to enable developer mode)
```

提醒開發者在 serial monitor 可以切換模式。

### 為什麼選 Serial？

Serial 介面需要 USB 線實體連接，確保只有持有裝置的開發者才能切換。透過 web UI 或 MQTT 遠端切換故意不支援，避免遠端攻擊或誤操作將出貨裝置切入 DEV 模式。

---

## 後端 Guard

```cpp
// src/web_server_logic.cpp — file-scope function
static bool requireDevMode(WebServer &server)
{
    if (!isDevMode()) {
        server.sendHeader("Cache-Control", "no-store");
        server.send(403, "text/plain", "Developer mode required");
        return false;
    }
    return true;
}
```

**為什麼是 file-scope 函式而非 lambda？**

`initWebRoutes()` 內定義的 lambda 是本地物件，函式回傳後即銷毀；若在其他地方透過 `server.on()` 回呼呼叫這些 lambda，會是 dangling reference（未定義行為）。改成 file-scope 的 `static` 函式，生命週期與程式相同，永遠有效。

### 受 guard 保護的 Endpoints

| Endpoint | HTTP Method | 被封鎖時的回應 |
|----------|-------------|---------------|
| `/factory-reset` | POST | HTTP 403 |
| `/factory-reset?full=1` | POST | HTTP 403 |
| `/network/clear` | POST | HTTP 403 |

---

## 前端 SSR + Auto-reload

### SSR 佔位符

`getIndexHTML()` 與 `getNetworkPageHTML()` 在 HTML 字串中埋入四個佔位符，由 `web_pages.cpp` 在伺服器端渲染時替換：

| 佔位符 | DEV 時替換為 | USER 時替換為 |
|--------|------------|-------------|
| `{{DEV_BANNER}}` | 橘色「DEVELOPER MODE」橫幅 HTML | 空字串 |
| `{{DEV_FACTORY_BUTTONS}}` | Factory Reset 與 Factory Reset Full 按鈕 HTML | 空字串 |
| `{{DEV_NETWORK_BUTTONS}}` | Clear WiFi Credentials 按鈕 HTML（於 /network 頁） | 空字串 |
| `{{PAGE_RENDERED_DEV}}` | `true` | `false` |

`{{PAGE_RENDERED_DEV}}` 會直接嵌入 JS 變數：

```js
const PAGE_RENDERED_DEV = {{PAGE_RENDERED_DEV}};
```

### ETag 的角色

`/` 端點根據目前模式設定不同的 ETag：

```cpp
server.sendHeader("ETag", isDevMode() ? "\"v1-dev\"" : "\"v1-user\"");
```

當模式切換後，ETag 改變，瀏覽器不會使用快取的舊頁面，確保 `location.reload()` 取回正確的 SSR HTML。

### Tick Polling 偵測模式翻轉

前端每 500ms 透過 `/tick` 輪詢感測器資料，`/tick` 回應中包含 `"dev": true|false`。

```js
fetch('/tick').then(r => r.json()).then(data => {
    // ...
    // Mode flip detection — reload page so SSR re-renders dev DOM
    if (typeof PAGE_RENDERED_DEV !== 'undefined'
        && typeof data.dev !== 'undefined'
        && data.dev !== PAGE_RENDERED_DEV) {
        console.log('[Mode] flip detected (' + PAGE_RENDERED_DEV + ' -> ' + data.dev + '), reloading');
        location.reload();
    }
});
```

當 `data.dev`（目前模式）與 `PAGE_RENDERED_DEV`（頁面渲染時的模式）不一致，即觸發 `location.reload()`，約 ≤1 s 內自動刷新，dev 按鈕出現或消失。

---

## Endpoint 對照表

| Endpoint | 可用 Mode | HTTP Method | 動作 |
|----------|----------|-------------|------|
| `/clear-records` | user + dev | GET | 清 records.json，**不**重置 record ID |
| `/clear-schedule` | user + dev | POST | 清空所有 schedule entries（RAM + NVS） |
| `/dev-status` | user + dev | GET | 回傳 `{"dev":true}` 或 `{"dev":false}` |
| `/factory-reset` | **dev only** | POST | 清 records.json + 重置 NVS record ID（保留校正/WiFi/排程） |
| `/factory-reset?full=1` | **dev only** | POST | 上面 + 清 WiFi NVS + 清全部 schedule entries |
| `/network/clear` | **dev only** | POST | 清 WiFi NVS（下次重啟 fallback 到 compile-time SSID） |

---

## 校正 vs 破壞性動作的確認強度

| 動作 | 確認方式 |
|------|---------|
| Clear All Records | 瀏覽器原生 `confirm()` |
| Clear All Schedule | 瀏覽器原生 `confirm()` |
| 絕對歸零（Set Zero） | Modal 輸入 `CONFIRM` |
| 倍率校正（Calibrate Scale） | Modal 輸入 `CONFIRM` |
| Factory Reset | Modal 輸入 `CONFIRM` |
| Clear WiFi Credentials | Modal 輸入 `CONFIRM` |

---

## ⚠️ Google Sheets Dedup 風險

GAS 端以 record ID 做去重（deduplication）。

**問題場景：**
1. 裝置正常運作，已上傳 ID 1~10 到 Google Sheet
2. 開發者執行 `/factory-reset`，`records.json` 清空，NVS record ID 重置為 0
3. 下次 auto-logger 建立記錄時 ID 從 1 重新開始
4. GAS 收到 ID-1 的新資料，發現 Sheet 上已有 ID-1，視為重複，靜默 ack 為 `received_ids` 但**不寫入新 row**
5. ESP32 收到 ack 後將 record 標記為 `synced=true`，實際上資料沒進 Sheet

**解決方式：** Factory Reset 完成後，必須手動至 Google Sheet 刪除所有現有資料列（或清空整個 Sheet），讓 ID 空間歸零，避免 ID 碰撞。

---

## 檔案

| 檔案 | 角色 |
|------|------|
| `include/dev_mode.h` | 公開 API：`isDevMode()`、`setDevMode()`、`printBootModeBanner()`、`handleSerialModeCommand()` |
| `src/dev_mode.cpp` | RAM bool `g_devMode`、serial parser、boot banner 輸出 |
| `src/web_server_logic.cpp` | `requireDevMode()` file-scope guard、受保護 endpoints 實作 |
| `src/web_pages.cpp` | SSR 佔位符替換、ETag 設定、`/tick` auto-reload JS |
