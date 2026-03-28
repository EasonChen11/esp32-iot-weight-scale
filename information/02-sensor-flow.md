# 感測器模組流程 (sensor_manager)

## 初始化流程

```
initSensor(offset1, offset2)
       │
       ├─── SIMULATE_SENSOR = true ?
       │         │
       │    Yes: randomSeed(analogRead(0))
       │         sim_weight1 = 25.0
       │         sim_weight2 = 22.0
       │         └─ return
       │
       No
       │
       ▼
  scale1.begin(DT=13, SCK=14)
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
       ├─ 模擬模式：base 25.0 kg + random(-0.5, +0.5) kg
       │             每次讀取都產生新的隨機波動
       │
       └─ 真實模式：scale1.is_ready() ?
              ├─ No: return -1.0（感測器未準備好）
              └─ Yes: scale1.get_units(3)  ← 讀 3 次平均
                      │
                      └─ |raw| < 0.01 → 歸零（消除雜訊）
       │
       ▼
  _doRead2()
       │
       ├─ 模擬模式：base 22.0 kg + random(-0.5, +0.5) kg
       │
       └─ 真實模式：同 _doRead1 邏輯
       │
       ▼
  cached_weight1 = 結果    ← 存入 volatile RAM
  cached_weight2 = 結果    ← 同上
  last_read_time = millis()
  Serial 輸出
```

## 模擬模式（SIMULATE_SENSOR）

使用隨機波動模擬真實感測器讀數：

```
_doRead1():
  noise = random(-500, 501) / 1000.0    // -0.500 ~ +0.500 kg
  sim_weight1 = 25.0 + noise            // 基礎值 25 kg ± 0.5 kg
  return sim_weight1

_doRead2():
  noise = random(-500, 501) / 1000.0    // -0.500 ~ +0.500 kg
  sim_weight2 = 22.0 + noise            // 基礎值 22 kg ± 0.5 kg
  return sim_weight2

初始化時呼叫 randomSeed(analogRead(0)) 確保每次啟動產生不同序列。
模擬值範圍：S1 = 24.5~25.5 kg, S2 = 21.5~22.5 kg, Total ≈ 46~48 kg
```

## 資料存取方式

```
              Core 1                             Core 0
        ┌───────────────────┐          ┌───────────────────────┐
        │  updateSensor()   │          │                       │
        │       |           │          │  getCachedWeight()    │ ← Web /data
        │       v           │          │  getCachedWeight1()   │ ← Web /data1
        │  cached_weight1   ├─volatile─│  getCachedWeight2()   │ ← Web /data2
        │  cached_weight2   │          │                       │ ← MQTT
        │                   │          │                       │ ← OLED
        └───────────────────┘          │                       │ ← Auto-logger
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

## HX711 省電（Deep Sleep）

進入深度睡眠前，`powerDownSensors()` 會呼叫兩顆 HX711 的 `power_down()`（SCK 拉 HIGH），
搭配 `gpio_hold_en()` 確保 SCK 在 deep sleep 期間維持 HIGH，避免 HX711 意外喚醒。

喚醒後 `initSensor()` 先呼叫 `gpio_hold_dis()` 釋放 SCK，再由 `scale.begin()` 拉低 SCK 自動喚醒 HX711。

```
入睡前：
  powerDownSensors()
    → scale1.power_down()   (SCK=HIGH → HX711 進入省電模式, ~1µA)
    → scale2.power_down()
    → gpio_hold_en(SCK1)    (鎖住 GPIO 狀態)
    → gpio_hold_en(SCK2)

喚醒後：
  initSensor()
    → gpio_hold_dis(SCK1)   (釋放 GPIO hold)
    → gpio_hold_dis(SCK2)
    → scale1.begin()         (SCK=LOW → HX711 自動喚醒)
    → scale2.begin()
```
