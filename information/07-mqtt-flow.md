# MQTT 模組流程 (mqtt_manager)

## 架構

```
ESP32                              PC (Docker)
┌──────────────┐            ┌────────────────────────┐
│ mqtt_manager │──publish──→│  Mosquitto (port 1884) │
│              │            │         │              │
│ PubSubClient │            │         ▼              │
│ + WiFiClient │            │  Node-RED (port 1880)  │
└──────────────┘            │         │              │
                            │         ▼              │
                            │  Dashboard UI          │
                            │  http://localhost:1880 │
                            └────────────────────────┘
```

## 初始化

```
initMQTT()
    │
    ▼
mqttClient.setServer("YOUR_AP_PASS", 1884)
Serial: 印出 broker 地址和 topic 列表
```

## 主循環 (Core 0, 每 10ms)

```
handleMQTT()
    │
    ├─ WiFi.status() != WL_CONNECTED ?
    │       └─ Yes: return（WiFi 未連線，跳過）
    │
    ├─ mqttClient.connected() ?
    │       │
    │       └─ No: tryReconnectMQTT()
    │              │
    │              ├─ 距離上次嘗試 < 5 秒 → return（back-off）
    │              │
    │              └─ mqttClient.connect("esp32-weight-scale")
    │                     │
    │                     ├─ 成功 → "connected."
    │                     └─ 失敗 → "failed (rc=X). Next attempt in 5s"
    │                     │
    │                     └─ return（本輪不發布）
    │
    ▼ 已連線
mqttClient.loop()            ← 維持心跳
    │
    ├─ 距離上次發布 < 5000ms ?
    │       └─ Yes: return（未到發布時間）
    │
    ▼ 發布
snprintf → mqttClient.publish("weight-scale/sensor1", "12.345")
snprintf → mqttClient.publish("weight-scale/sensor2", "11.890")
snprintf → mqttClient.publish("weight-scale/total",   "24.235")
    │
    ▼
Serial: "[MQTT] Published -> S1=12.345 kg  S2=11.890 kg  Total=24.235 kg"
```

## MQTT Topics

```
weight-scale/
    ├── sensor1    → getCachedWeight1()  "12.345"
    ├── sensor2    → getCachedWeight2()  "11.890"
    └── total      → getCachedWeight()   "24.235"

發布間隔：每 5 秒
資料來源：RAM 快取（不碰 Flash）
格式：純文字，3 位小數
```

## 重連機制

```
                    ┌───────────────────┐
                    │  WiFi connected?  │
                    └───────┬───────────┘
                       No   │   Yes
                    return  │
                            ▼
                    ┌───────────────────┐
                    │  MQTT connected?  │
                    └───────┬───────────┘
                       No   │   Yes
                            │      ↓
                            │   normal publish
                            ▼
                    ┌───────────────────┐
                    │  last attempt     │
                    │  >= 5 sec ago?    │
                    └───────┬───────────┘
                       No   │   Yes
                    return  │
                            ▼
                    mqttClient.connect()
                            │
                       ┌────┴────┐
                    success   fail
                       │         │
                  publish    retry in 5s
                  next loop
```

## Docker 端

```bash
# 啟動
docker compose -f docker/docker-compose.mqtt.yml up -d

# 容器
esp32-mosquitto  → MQTT broker (port 1884)
esp32-nodered    → Dashboard   (port 1880)

# Node-RED Dashboard
http://localhost:1880/ui
  ├── Sensor 1 (Left) gauge
  ├── Sensor 2 (Right) gauge
  ├── Total (Beehive) gauge + 數字
  └── History Chart (6 小時滾動折線圖)

# 停止
docker compose -f docker/docker-compose.mqtt.yml down
```
