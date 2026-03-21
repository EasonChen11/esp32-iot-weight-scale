# WiFi 模組流程 (wifi_manager)

## 連線模式：AP + STA 同時運行

ESP32 同時開啟兩個 WiFi 介面：

```
┌─────────────────────────────────────────────────────────────┐
│                   WiFi.mode(WIFI_AP_STA)                    │
├────────────────────────────┬────────────────────────────────┤
│    AP interface (Soft-AP)  │    STA interface (Station)     │
│                            │                                │
│  SSID: ESP32_Weight_Scale  │  Connects to: YOUR_AP_PASS        │
│  Pass: YOUR_AP_PASS         │  Pass: YOUR_AP_PASS              │
│  IP: 192.168.4.1 (fixed)  │  IP: DHCP assigned             │
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

## 初始化流程

```
initWiFi()
    │
    ▼
WiFi.mode(WIFI_AP_STA)       ← 設定雙模式
    │
    ▼
WiFi.softAP(SSID, PASS)      ← 立即啟動 AP
Serial: "[WiFi] AP started: SSID='ESP32_Weight_Scale' IP: 192.168.4.1"
    │
    ▼
WiFi.begin(STA_SSID, STA_PASS)  ← 開始連接家用 WiFi
    │
    ▼
等待連線（每 500ms 檢查一次，最多 30 次 = 15 秒）
    │
    ├─ 成功 → Serial 印出 STA IP + MQTT broker 資訊
    │
    └─ 失敗 → Serial 警告，MQTT 不可用，Web 仍可透過 AP 使用
    │
    ▼
initNTP()                      ← NTP 時間同步（見 10-ntp-flow.md）
    │
    ├─ STA 連線成功 → configTime() → 自動從 NTP 伺服器取得時間
    │
    └─ STA 連線失敗 → 跳過 NTP，依靠瀏覽器 /sync 同步
```

## 連線狀態對功能的影響

| 功能 | STA 連線成功 | STA 連線失敗 |
|------|-------------|-------------|
| Web 伺服器 (AP) | 可用 (192.168.4.1) | 可用 (192.168.4.1) |
| Web 伺服器 (STA) | 可用 (DHCP IP) | 不可用 |
| MQTT 發布 | 可用 | 不可用 |
| NTP 時間同步 | 自動同步 | 不可用 |
| OLED 顯示 | 正常 | 正常 |
| 感測器讀取 | 正常 | 正常 |
| 自動記錄 | 正常 | 正常 |
| 時間同步 | NTP 自動 + Web `/sync` 備用 | 僅 Web `/sync`（AP 模式） |
| 排程喚醒 | 正常（NTP 提供時間） | 需先開網頁 /sync 同步時間 |

## 使用情境

```
情境 A：有家用路由器（STA + AP）
  手機/電腦 ──WiFi──→ 路由器 ←──WiFi──→ ESP32 (STA)
                        │                   │
                        ▼                   ▼
                   MQTT broker (PC)    NTP 伺服器（自動同步）

情境 B：戶外/沒有路由器（僅 AP）
  手機 ──WiFi──→ ESP32 AP (192.168.4.1)
                     │
                     ▼
                  Web 介面正常使用
                  時間透過 /sync 從手機瀏覽器同步
                  （MQTT、NTP 不可用）

情境 C：深度睡眠模式 + 有路由器
  ESP32 喚醒 → WiFi STA 連線 → NTP 同步 → 記錄資料 → 10分鐘後入睡
  （全自動，無需人工介入）
```
