# OLED 顯示模組流程 (oled_manager)

## 硬體配置

```
ESP32                    SSD1306 OLED (128x64)
GPIO 15 (OUTPUT HIGH) ──→ VCC (~20mA, 在 40mA GPIO 限制內)
GPIO 4  ────────────────→ SDA (I2C 資料)
GPIO 5  ────────────────→ SCL (I2C 時脈, 400kHz Fast-mode)
GND     ────────────────→ GND
```

為什麼不用 3.3V/5V：兩個 HX711 已經佔用了電源腳位。
為什麼不用 GPIO 21/22 (預設 I2C)：被 HX711 #1 佔用。

## 初始化

```
initOLED()
    │
    ▼
pinMode(GPIO 15, OUTPUT)
digitalWrite(GPIO 15, HIGH)    ← 供電給 OLED
delay(10)                      ← 等電壓穩定
    │
    ▼
Wire.begin(SDA=4, SCL=5)      ← 自定義 I2C 腳位
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

## 畫面佈局 (128x64 像素)

```
drawWeight(label, kg)

┌────────────────────────────────┐
│ Y=0  label (textSize 2)       │  例："Total"
│                                │
│                                │
│ Y=22 weight (textSize 3)   kg │  例："24.23"  "kg"
│                          Y=30 │
│                                │
│                                │
└────────────────────────────────┘
  X=0                      X=96

textSize 2 = 12x16 像素/字元
textSize 3 = 18x24 像素/字元
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
