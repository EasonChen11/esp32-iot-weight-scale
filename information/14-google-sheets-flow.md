# Google Sheets 同步流程 (google_sheets_manager)

## 概觀

Google Sheets 模組（`google_sheets_manager`）透過 Google Apps Script (GAS) web app 端點，把 LittleFS flash 儲存裡的重量紀錄同步到 Google 試算表。每筆紀錄都用 `synced` 旗標追蹤，以確保 exactly-once 投遞（不重複、不漏）。

## 架構

```
ESP32 (LittleFS)  ──HTTPS POST──▶  Google Apps Script  ──▶  Google Sheets
                  ◀──JSON response──  (returns received_ids)
```

## 紀錄格式 (LittleFS JSON)

```json
{
  "id": 1,
  "date": "2026-03-30",
  "time": "14:30:00",
  "sensor1": "25.300",
  "sensor2": "22.100",
  "synced": false
}
```

- `id`：存在 NVS 的單調遞增計數器（跨電源週期保留）
- `synced`：只有在 GAS 確認收到後才設為 `true`

## 同步觸發

### 自動（每次開機一次）

1. Auto-logger 在 `STARTUP_RECORD_DELAY_MS`（10 秒）後存一筆開機紀錄
2. `handleGoogleSheetsSync()` 偵測到 `isStartupRecordDone() == true`
3. 收集所有 `synced == false` 的紀錄
4. 批次 POST 到 GAS 端點
5. 把已確認的 ID 標記為 synced

### 手動（Web UI 按鈕）

- 「Sync Sheets」按鈕呼叫 `/sync-sheets` 端點
- 觸發 `triggerGoogleSheetsSync()`，回傳一段狀態訊息

## NVS 設定與序列 provisioning

URL 與 token 屬於**維運方/服務端**設定（不是使用者環境），因此**不放在任何網頁上**，改用 USB 序列 console 設定一次、存進 NVS。

讀取優先序（比照 WiFi）：**NVS 優先，compile-time `GOOGLE_SHEETS_URL/TOKEN` 當 fallback**。因為公開/OTA build 用的是佔位符 secrets，序列設定的值存在 NVS、**跨 OTA 更新都保留**，所以 OTA 後同步照常運作。

序列指令（115200 baud，需 `DEV_MODE_ENABLED`）：

| 指令 | 效果 |
|------|------|
| `sheets-set <url> <token>` | 把 Apps Script URL + token 存進 NVS |
| `sheets-status` | 顯示是否已有 NVS 設定（token 遮蔽，只顯示長度） |
| `sheets-clear` | 清除 NVS 設定，回到 compile-time fallback |

> token 絕不以明文回顯（含序列）。設定一次即可，之後 OTA 更新不用重設。

## GAS 通訊協定

Google Apps Script 處理完 POST 資料後會回傳 HTTP 302 重導。這個重導 URL 必須用 **GET**（不是 POST）去取，才能拿到 JSON 回應。

### 兩段式流程

1. **POST** `{"token": "...", "records": [...]}` 到 GAS exec URL → 收到 **302** 含 `Location` header
2. **GET** 該重導 URL → 收到 `{"success": true, "received_ids": [1,2,3], "count": 3}`

### 為什麼要兩段？

GAS 架構會先處理 POST body 並寫入試算表，然後重導到一個只接受 GET 的 `googleusercontent.com` echo 端點。若對最初的 POST 直接用 `HTTPC_FORCE_FOLLOW_REDIRECTS`，重導 URL 會收到 POST 而非 GET，導致 HTTP 400/405 錯誤。

## GAS 腳本契約

**POST body：**
```json
{"token": "<shared secret>", "records": [{"id": 1, "date": "2026-03-30", "time": "14:30:00", "sensor1": "25.300", "sensor2": "22.100"}]}
```

**回應：**
```json
{"success": true, "received_ids": [1], "count": 1}
```

- GAS 以 ID 去重（不會插入重複的列）
- 只有列在 `received_ids` 裡的 ID 才會在 ESP32 上被標記為 synced
- 總重量在試算表內計算（不由 ESP32 傳送）

## 設定

| 設定項 | 位置 | 值 |
|--------|------|-----|
| 功能開關 | `config.h` | `#define GOOGLE_SHEETS_ENABLED true` |
| GAS URL（fallback） | `config_secrets.h` | `const char *const GOOGLE_SHEETS_URL = "https://..."` |
| 共享 token（fallback） | `config_secrets.h` | `const char *const GOOGLE_SHEETS_TOKEN = "..."`（必須與 GAS 內的 `SHEETS_TOKEN` 一致） |
| URL/token（執行期優先） | NVS `sheets_cfg` | 由 `sheets-set` 序列指令寫入，優先於 compile-time |
| 最大紀錄數 | `config.h` | `MAX_RECORDS = 200` |

## 錯誤處理

| 情境 | 行為 |
|------|------|
| 無 WiFi（僅 AP 模式） | 跳過同步——紀錄維持未同步 |
| GAS 回傳部分成功 | 只有已確認的 ID 被標記 synced |
| HTTP 失敗 / 逾時 | 紀錄維持未同步，下次開機重試 |
| 舊紀錄格式（無 `id` 欄位） | 自動遷移：偵測到即清空 flash |
| NTP 失敗 | 紀錄仍會用 RTC fallback 時間存下 |

## NTP Callback 同步

NTP 用 `sntp_set_time_sync_notification_cb()` 來可靠地偵測同步，而非輪詢 `sntp_get_sync_status()`。這解決了 ESP32 上輪詢迴圈會過早結束的問題。

## 相關檔案

| 檔案 | 角色 |
|------|------|
| `include/google_sheets_manager.h` | 對外 API |
| `src/google_sheets_manager.cpp` | HTTPS POST 邏輯、同步協調、URL/token NVS-first 解析 |
| `src/storage/nvs_storage.cpp` | `synced` ID 計數器 + Sheets 設定（`saveSheetsConfig` 家族） |
| `src/storage/littlefs_storage.cpp` | 帶 `synced` 旗標的紀錄 CRUD |
| `src/dev_mode.cpp` | `sheets-set` / `sheets-status` / `sheets-clear` 序列指令 |
| `src/auto_logger.cpp` | 擴充的紀錄格式（id, date, s1, s2） |
