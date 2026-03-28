# OLED 顯示模組流程 (oled_manager)

## 硬體配置

```
ESP32                    SSD1306 OLED (128x32)
GPIO 19 (OUTPUT HIGH) ──→ VCC (~20mA, 在 40mA GPIO 限制內)
GPIO 21 ────────────────→ SDA (I2C 資料, ESP32 預設 I2C SDA)
GPIO 22 ────────────────→ SCL (I2C 時脈, 400kHz Fast-mode, ESP32 預設 I2C SCL)
GND     ────────────────→ GND
```

使用 ESP32 預設 I2C 腳位（GPIO 21/22），未來擴充其他 I2C 裝置更方便。

## 初始化

```
initOLED()
    │
    ▼
pinMode(GPIO 19, OUTPUT)
digitalWrite(GPIO 19, HIGH)    ← 供電給 OLED
delay(10)                      ← 等電壓穩定
    │
    ▼
Wire.begin(SDA=21, SCL=22)    ← ESP32 預設 I2C 腳位
Wire.setClock(400000)          ← Fast-mode: I2C 傳輸 90ms → 23ms
    │
    ▼
display.begin(SSD1306_SWITCHCAPVCC, 0x3C)  ← I2C 地址 0x3C
    │
    ├─ 失敗 → Serial 警告，return
    │
    ▼ 成功
display.clearDisplay()
display.display()
Serial: "[OLED] Initialized"
```

## 顯示循環 (Core 1, 每次 loop)

```
handleOLED()
    │
    ├─ 自動輪播檢查
    │   │
    │   ├─ currentMode == MODE_TOTAL ?
    │   │       → showDuration = 5000ms
    │   │
    │   └─ currentMode == MODE_SENSOR1/2 ?
    │           → showDuration = 2000ms
    │   │
    │   ├─ millis() - modeStartMs >= showDuration ?
    │   │       └─ Yes: currentMode = 下一個模式
    │   │                modeStartMs = millis()
    │   │
    │   └─ No: 維持目前模式
    │
    ├─ 刷新率限制
    │   │
    │   └─ millis() - lastRefreshMs < 200ms ?
    │           └─ Yes: return（5 Hz 刷新率）
    │
    ▼ 畫面刷新
  switch (currentMode)
    ├─ MODE_TOTAL   → drawWeight("Total",    getCachedWeight())
    ├─ MODE_SENSOR1 → drawWeight("Sensor 1", getCachedWeight1())
    └─ MODE_SENSOR2 → drawWeight("Sensor 2", getCachedWeight2())
```

## 自動輪播時序

```
 <------ 5 sec ------>|<-- 2 sec -->|<-- 2 sec -->|
┌─────────────────────┬─────────────┬─────────────┐
│       Total         │  Sensor 1   │  Sensor 2   │
│      24.23 kg       │  12.34 kg   │  11.89 kg   │
└─────────────────────┴─────────────┴─────────────┘
                                                 repeat

Full cycle = 5 + 2 + 2 = 9 sec
```

## 畫面佈局 (128x32 像素)

```
drawWeight(label, kg)

┌────────────────────────────────┐
│ Y=0  label (textSize 1)        │  例："Total"
│                                │
│ Y=14 weight (textSize 2)   kg  │  例："24.23"  "kg"
│                         Y=22   │    (textSize 1)
└────────────────────────────────┘
  X=0                      X=104

textSize 1 = 6x8 像素/字元
textSize 2 = 12x16 像素/字元
Total height: 14 + 16 = 30 px (fits 32 px)
```

## 資料來源

```
OLED 只從 RAM 快取讀取，不碰任何 Flash：

  getCachedWeight()  ← volatile float cached_weight1 + cached_weight2
  getCachedWeight1() ← volatile float cached_weight1
  getCachedWeight2() ← volatile float cached_weight2

  由 Core 1 的 updateSensor() 每 500ms 更新
  OLED 每 200ms 刷新一次畫面（同一個值可能畫 2-3 次）
```
