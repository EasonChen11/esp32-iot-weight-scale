# 感測器模組流程 (sensor_manager)

## 初始化流程

```
initSensor(offset1, offset2)
       │
       ├─── SIMULATE_SENSOR = true ?
       │         │
       │    Yes: sim_weight1 = 10.0, sim_weight2 = 15.0
       │         └─ return
       │
       No
       │
       ▼
  scale1.begin(DT=21, SCK=22)
  scale1.wait_ready_timeout(2000ms)
       │
       ├─ 失敗 → Serial 錯誤訊息 → delay(1000) → 遞迴重試
       │
       ▼ 成功
  scale1.set_scale(85000.0)
       │
       ├─ offset1 != 0 → scale1.set_offset(offset1)  ← 套用 NVS 校準
       └─ offset1 == 0 → scale1.tare()                ← 自動歸零
       │
       ▼
  scale2.begin(DT=18, SCK=19)
  scale2.wait_ready_timeout(2000ms)
       │
       ├─ 失敗 → 警告但不阻擋啟動（Sensor 2 為非致命）
       │
       ▼ 成功
  scale2.set_scale(85000.0)
  套用 offset2 或 tare（同 Sensor 1 邏輯）
```

## 讀取循環（Core 1, 每 500ms）

```
updateSensor()
       │
       ├─ millis() - last_read_time < 500ms ?
       │         └─ Yes: return（未到時間）
       │
       ▼ No
  _doRead1()
       │
       ├─ 模擬模式：sim_weight1 += 0.03, 超過 50 歸回 10
       │
       └─ 真實模式：scale1.is_ready() ?
              ├─ No: return -1.0（感測器未準備好）
              └─ Yes: scale1.get_units(3)  ← 讀 3 次平均
                      │
                      └─ |raw| < 0.01 → 歸零（消除雜訊）
       │
       ▼
  cached_weight1 = 結果    ← 存入 volatile RAM
  cached_weight2 = 結果    ← 同上
  last_read_time = millis()
  Serial 輸出
```

## 資料存取方式

```
              Core 1                             Core 0
        ┌───────────────────┐          ┌───────────────────────┐
        │  updateSensor()   │          │                       │
        │       |           │          │  getCachedWeight()    │ <- Web /data
        │       v           │          │  getCachedWeight1()   │ <- Web /data1
        │  cached_weight1   ├─volatile─│  getCachedWeight2()   │ <- Web /data2
        │  cached_weight2   │          │                       │ <- MQTT
        │                   │          │                       │ <- OLED
        └───────────────────┘          │                       │ <- Auto-logger
                                       └───────────────────────┘
```

- `volatile float` 確保跨核心讀取不會被編譯器優化掉
- `getCachedWeight()` = S1 + S2，負值視為 0（感測器未就緒）

## Tare 與校準

```
/tare1 (Web) → tareSensor1() → scale1.tare()        ← 暫時歸零（RAM，斷電消失）
/tare2 (Web) → tareSensor2() → scale2.tare()
/tare  (Web) → tareSensor()  → 兩者都 tare

/set-zero1 (Web) → captureAbsoluteOffset1()
                      → scale1.tare()
                      → scale1.get_offset()    ← 取得原始偏移量
                      → saveAbsoluteOffset()   ← 寫入 NVS（永久保存）

/set-zero2 (Web) → captureAbsoluteOffset2()
                      → saveAbsoluteOffset2()  ← 寫入 NVS
```

**差異：**
- `tare` = 暫時歸零，存在 HX711 library 的 RAM 變數，重開機就消失
- `set-zero` = 永久校準，offset 寫入 NVS flash，開機自動套用
