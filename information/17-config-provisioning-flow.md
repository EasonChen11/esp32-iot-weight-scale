# 設定 Provisioning 流程（NVS，跨 OTA 保留）

## 概觀

操作者/服務祕密以 NVS 為單一真實來源（Single Source of Truth），跨重開機與 OTA 保留。
OTA release 韌體以 `config_secrets.h.example`（佔位符）建置，因此「只存在編譯時」的值在 OTA
後會變回 example；把值寫進 NVS 即可永久保留。

## 各設定的 provisioning 管道

| 設定 | NVS namespace | 序列指令 | 網頁 |
|------|---------------|----------|------|
| STA WiFi | `wifi_cfg` | （現有，經網頁/序列） | `/network` |
| AP SSID/密碼 | `ap_cfg` | `ap-set` / `ap-status` / `ap-clear` | `/network`（write-only） |
| MQTT broker IP/port | `mqtt_cfg` | `mqtt-set` / `mqtt-status` / `mqtt-clear` | 否（序列限定） |
| Google Sheets URL/token | `sheets_cfg` | `sheets-set` / `sheets-status` / `sheets-clear` | 否（序列限定） |

敏感的服務祕密（MQTT、Sheets）僅經 USB 序列設定，不暴露於網頁。AP 憑證可經 `/network` 頁的
write-only 欄位設定（永不回顯已存密碼）。

## 序列指令（115200 baud，需 DEV_MODE_ENABLED）

```
ap-set <ssid> <pass>      AP SSID + 密碼（ssid 1-32 字元；pass 8-63 碼、不可含空格）；重開生效
ap-status                 顯示來源（NVS / compile-time）與 SSID；密碼只印長度
ap-clear                  清除 ap_cfg，回退編譯時值

mqtt-set <ip> <port>      MQTT broker 位址（port 1-65535）；重開生效
mqtt-status               顯示來源、ip、port
mqtt-clear                清除 mqtt_cfg，回退編譯時值
```

## 解析規則（開機時）

softAP 與 MQTT broker 在啟動時解析：**NVS 有有效值就用 NVS，否則用編譯時值**，並在
log 標示來源（`source: NVS` / `source: compile-time`）。改設定後**需重開機生效**。

> MQTT 預設停用（`MQTT_ENABLED=false`）。broker provisioning 能力已就緒，待之後以 OTA 部署
> 一版 `MQTT_ENABLED=true` 即可啟用，broker 位址無需重新編譯。

## 出貨前建議流程

1. USB 連線、開序列監控。
2. 設定 STA WiFi（網頁或序列）、`ap-set`、（如需）`mqtt-set`、`sheets-set`。
3. 各 `*-status` 確認來源為 NVS。
4. 重開機，確認 log 顯示 `source: NVS`。
5. 部署。之後 OTA 更新不會洗掉這些設定。

## 跨 OTA 保留的原理

`nvs` 分區（offset `0x9000`、size `0x5000`）在 OTA 前後不變，只有 app 分區被更新，
所以所有 NVS namespace（含上述設定與感測器校正、排程）皆完整保留。詳見
`information/16-ota-flow.md`。
