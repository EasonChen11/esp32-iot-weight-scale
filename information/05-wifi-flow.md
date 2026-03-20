# WiFi 模組流程 (wifi_manager)

## 連線模式：AP + STA 同時運行

ESP32 同時開啟兩個 WiFi 介面：

```
┌─────────────────────────────────────────────────────────────┐
│                   WiFi.mode(WIFI_AP_STA)                    │
├────────────────────────────┬────────────────────────────────┤
│    AP interface (Soft-AP)  │    STA interface (Station)     │
│                            │                                │
│  SSID: ESP32_Weight_Scale  │  Connects to: ***REMOVED***         │
│  Pass: ***REMOVED***         │  Pass: ***REMOVED***              │
│  IP: 192.168.4.1 (fixed)   │  IP: DHCP assigned             │
│                            │                                │
│  Purpose:                  │  Purpose:                      │
│  - Phone direct connect    │  - Reach MQTT broker           │
│  - Works without router    │  - Access via home network     │
│  - Always on               │  - Falls back gracefully       │
└────────────────────────────┴────────────────────────────────┘

AP (Soft-AP): phone/device direct connect, always on, IP 192.168.4.1
STA (Station): home WiFi for MQTT, DHCP IP, fallback if unavailable
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
```

## 連線狀態對功能的影響

| 功能 | STA 連線成功 | STA 連線失敗 |
|------|-------------|-------------|
| Web 伺服器 (AP) | 可用 (192.168.4.1) | 可用 (192.168.4.1) |
| Web 伺服器 (STA) | 可用 (DHCP IP) | 不可用 |
| MQTT 發布 | 可用 | 不可用 |
| OLED 顯示 | 正常 | 正常 |
| 感測器讀取 | 正常 | 正常 |
| 自動記錄 | 正常 | 正常 |
| 時間同步 | 透過 Web `/sync` | 透過 AP Web `/sync` |

## 使用情境

```
情境 A：有家用路由器
  手機/電腦 ──WiFi──→ 路由器 ←──WiFi──→ ESP32 (STA)
                        │
                        ▼
                   MQTT broker (PC)

情境 B：戶外/沒有路由器
  手機 ──WiFi──→ ESP32 AP (192.168.4.1)
                     │
                     ▼
                  Web 介面正常使用
                  （MQTT 不可用）
```
