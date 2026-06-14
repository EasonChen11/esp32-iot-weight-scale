# OTA 韌體更新流程 (ota_manager)

## 概觀

OTA 模組（`ota_manager`）透過 HTTPS 自動從 GitHub Releases 拉取韌體更新。每次開機/喚醒、WiFi 連上之後，ESP32 會抓一個很小的 `manifest.json`，把裡面的版本和目前執行中的韌體比對；若有更新版，就串流下載韌體、驗證 SHA-256、寫入「非執行中」的 OTA 分區，再重開機切到新韌體。中途若出任何問題（網路錯誤、下載截斷、雜湊不符），更新會中止、目前韌體照常運行，不需要任何人介入。

## 架構

```
ESP32 (Core 0 — WebAndTasksCode)
        │
        │  HTTPS GET  (WiFiClientSecure + Mozilla CA bundle)
        ▼
GitHub Releases  ──  manifest.json  ──► fetchManifest()
   (latest)                                   │
        │                                     ▼
        │                            version compare
        │                           (semver 3-field)
        │                                     │
        │         not newer ◄────────────────┤
        │                                     │ newer
        │  HTTPS GET  firmware.bin            ▼
        │◄────────────────────────── downloadAndApply()
        │                                     │
        │                            SHA-256 verify
        │                                     │
        │                     mismatch ◄──────┤
        │                                     │ match
        │                                     ▼
        │                            Update.end(true)
        │                                     │
        │                                     ▼
        │                              ESP.restart()
        │                           (boots new image)
        │
        ▼
  bootloader (ESP-IDF OTA)
  PENDING_VERIFY → initOTA() marks valid
                   (rollback cancelled)
```

## checkOtaUpdate() 逐步流程

```
WebAndTasksCode (Core 0)
        │
        │  first iteration where WiFi.status() == WL_CONNECTED
        ▼
checkOtaUpdate()   [guarded by static bool otaChecked + WIFI_ENABLED]
        │
        ▼
fetchManifest()
  ├─ WiFiClientSecure + setCACertBundle(rootca_crt_bundle_start)
  ├─ HTTPS GET OTA_MANIFEST_URL (15 s timeout, follow redirects)
  ├─ deserializeJson → {version, url, sha256}
  └─ validate: version/url non-empty, sha256 length == 64
        │
        ▼  manifest OK
isNewerVersion(FIRMWARE_VERSION, manifest.version)?
        │
        ├─ No  → "[OTA] already up to date"  → return
        │
        └─ Yes → otaInProgress = true
                      │
                      ▼
             downloadAndApply()
               ├─ HTTPS GET manifest.url (firmware.bin, CA-verified, 15 s timeout)
               ├─ Update.begin(total)  → write to inactive OTA slot
               ├─ stream loop (1024-byte chunks):
               │       mbedtls_sha256_update()  +  Update.write()
               │       stall guard: no data for OTA_HTTP_TIMEOUT_MS → break
               ├─ if written != total → Update.abort()  → return false
               ├─ SHA-256 compare: computed hex vs manifest.sha256
               │       mismatch → Update.abort()       → return false
               └─ match → Update.end(true)             → return true
                      │
                      ▼
             otaInProgress = false
             delay(500) → ESP.restart()
```

## Rollback 防護 (initOTA)

`initOTA()` 在 `setup()` 結尾被呼叫（在 WiFi、儲存、感測器初始化之後）。若目前執行分區的狀態是 `ESP_OTA_IMG_PENDING_VERIFY`——代表裝置剛 OTA 燒入新韌體、是第一次開機——就呼叫 `esp_ota_mark_app_valid_cancel_rollback()`。能順利跑到 `setup()` 且所有子系統都初始化完成，就是自我檢查；通過即把這個韌體標記為「有效」。若韌體在跑到這行之前就 panic 或開不起來，ESP-IDF bootloader 會在下次重開機時自動回滾到上一個可用的韌體。

## 設定

| 設定項 | 位置 | 值 |
|--------|------|-----|
| 功能開關 | `config.h` | `#define OTA_ENABLED true` |
| Manifest URL | `config.h` | `OTA_MANIFEST_URL` = `https://github.com/EasonChen11/esp32-iot-weight-scale/releases/latest/download/manifest.json` |
| HTTP 逾時 | `config.h` | `OTA_HTTP_TIMEOUT_MS` = 15000 ms |
| 韌體版號 | `platformio.ini` | `-DFIRMWARE_VERSION=\"X.Y.Z\"`（build flag） |
| CA bundle | `cert/x509_crt_bundle.bin` | Mozilla 根憑證，透過 `board_build.embed_files` 嵌入韌體 |
| 分區配置 | `partitions.csv` | 自訂雙 OTA（`app0`/`app1` 各 0x190000） |

## 錯誤處理

| 情境 | 行為 |
|------|------|
| 無 WiFi / 離線 | `otaChecked` guard 不會觸發——跳過檢查；所有感測器與 Web 伺服器照常運作 |
| Manifest HTTP 錯誤 | 記錄（`[OTA] manifest HTTP <code>`）；中止更新；下次開機重試 |
| JSON 解析錯誤 / 欄位缺失 | 記錄；中止更新；目前韌體繼續執行 |
| Manifest 版本不比較新 | 記錄（"already up to date"）；不動作 |
| 韌體下載 HTTP 錯誤 | 記錄；`Update.abort()`；下次開機重試 |
| 下載停滯（15 秒無資料） | 迴圈中斷；截斷檢查觸發 `Update.abort()` |
| 下載截斷（written != total） | `Update.abort()`；flash 不動；下次開機重試 |
| SHA-256 不符 | `Update.abort()`；flash 不動；裝置不會變磚；下次開機重試 |
| 新韌體開不起來（跑到 `initOTA()` 前就 panic/hang） | ESP-IDF bootloader 下次重開機自動回滾到上一個有效韌體 |

## 分區配置

自訂的 `partitions.csv` 取代預設的單 app 配置，以啟用雙 OTA slot：

```
nvs       data  nvs      0x9000   0x5000
otadata   data  ota      0xe000   0x2000
app0      app   ota_0    0x10000  0x190000   (1.5625 MB — active slot)
app1      app   ota_1    0x1A0000 0x190000   (1.5625 MB — OTA target slot)
spiffs    data  spiffs   0x330000 0xC0000    (768 KB — LittleFS)
coredump  data  coredump 0x3F0000 0x10000
```

63 KB 的 Mozilla CA bundle 嵌入在 flash（計入 app slot 內）。目前韌體用掉約一個 app slot 的 73%。

### 一次性分區遷移警告

從預設分區配置切換到這個自訂配置，**需要一次完整的 USB erase + 重燒**：

```bash
pio run --target erase
pio run --target uploadfs   # 重燒 LittleFS
pio run --target upload     # 燒韌體
```

這會清掉 NVS（校正 offset 與排程）以及所有 LittleFS 紀錄。第一次燒入支援 OTA 的韌體後：

1. 透過 Web UI 重新輸入校正 offset。
2. 重新加入喚醒排程。
3. 紀錄：已同步到 Google Sheets；flash 上的副本會遺失，但試算表才是真實來源（source of truth）。

之後的更新都走 OTA——不再需要 USB 線。

## 發佈流程（維護者）

發版採 **tag 驅動、CI 自動化**（見 `.github/workflows/release.yml`）。版號的單一來源是 `platformio.ini` 的 `FIRMWARE_VERSION`，tag 必須與它一致。

```bash
# 1. 改 platformio.ini：-DFIRMWARE_VERSION=\"X.Y.Z\"
git commit -am "release: vX.Y.Z"
# 2. 打 tag 推上去，CI 全自動（tag 版號必須與 FIRMWARE_VERSION 一致）
git tag vX.Y.Z
git push origin vX.Y.Z
```

`release.yml` 在收到 `v*.*.*` tag 後會：跑完整 build matrix（5 種組態）+ cppcheck 驗證 → 驗證 tag 版號與 `FIRMWARE_VERSION` 一致 → **用 `scripts/update_ca_bundle.sh` 從 curl.se 抓最新 Mozilla 根憑證重新產生 `cert/x509_crt_bundle.bin`**（每次發版自動更新憑證，免手動維護；下載失敗會保留 committed bundle、不擋發版）→ `pio run` → 用 `scripts/make_manifest.py` 產生 `manifest.json` → 建立 GitHub Release 並附上 `firmware.bin` + `manifest.json`。最新的 release 會成為 **latest**，正是裝置那個永久 OTA URL 指向的位置，所以版本之間不需要改任何 URL。

> **手動 fallback**（無 CI）：`pio run` → `python scripts/make_manifest.py <version> .pio/build/esp32dev/firmware.bin` → `gh release create v<version> .pio/build/esp32dev/firmware.bin .pio/build/esp32dev/manifest.json --generate-notes`。release 必須標記為 "latest"。

## 測試／驗證 OTA

OTA 檢查**每次開機（或喚醒）後、WiFi 連上時只觸發一次**（`main.cpp` 的 `otaChecked` guard），不會定時輪詢。因此驗證一次 OTA 升級的步驟是：

1. 確認已有一個**版號比裝置目前韌體更新**的 Release 發佈為 latest（`manifest.json` 可由 OTA URL 取得）。
2. 開啟序列監控觀察過程。
3. **重開裝置**（reset 按鈕或重新上電）以觸發那一次性檢查。
4. 監控應依序出現：

   ```
   [OTA] Running firmware version: <舊版>
   [OTA] running <舊版>, latest <新版>
   [OTA] update available -> <新版>
   [OTA] verified + written <新版>, rebooting...
   ── 裝置自動重開 ──
   [OTA] Running firmware version: <新版>
   [OTA] running <新版>, latest <新版>
   [OTA] already up to date
   ```

第二次開機印出 `Running firmware version: <新版>` 且 `already up to date`，即代表升級成功。NVS（校正 offset、WiFi、排程等）跨 OTA 保留，因為 `nvs` 分區位置在更新前後不變。

> 監控若掛上 `esp32_exception_decoder` filter，OTA 重開機時可能出現 `addr2line ... firmware.elf: No such file` 的紅字——那是 decoder 想解析開機 ROM 的正常訊息但找不到對應 elf（OTA 來的韌體本機沒有 elf），**並非當機**，可忽略。

## 相關檔案

| 檔案 | 角色 |
|------|------|
| `include/ota_manager.h` | 對外 API（`initOTA`、`checkOtaUpdate`、`isOtaInProgress`） |
| `src/ota_manager.cpp` | HTTPS manifest 抓取、semver 比對、下載 + SHA256 + 寫入邏輯 |
| `partitions.csv` | 自訂雙 OTA 分區表 |
| `cert/x509_crt_bundle.bin` | 嵌入 flash 的 Mozilla CA bundle，供 HTTPS 驗證（發版時自動刷新） |
| `scripts/make_manifest.py` | 從建置出的 binary 產生 `manifest.json`（version、URL、sha256） |
| `scripts/update_ca_bundle.sh` | 從 curl.se 抓最新 Mozilla 根憑證重產 CA bundle（CI 發版時呼叫） |
| `scripts/gen_crt_bundle.py` | esp-idf 的憑證轉換工具（Apache-2.0，vendored），把 PEM 轉成 esp_crt_bundle 格式 |
| `include/config.h` | `OTA_ENABLED`、`OTA_MANIFEST_URL`、`OTA_HTTP_TIMEOUT_MS` |
| `platformio.ini` | `-DFIRMWARE_VERSION` build flag；`board_build.embed_files`；`board_build.partitions` |
| `.github/workflows/release.yml` | tag 驅動的發佈 workflow（matrix + lint 自我驗證後發 Release） |
