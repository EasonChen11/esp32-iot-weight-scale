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
  scale2.begin(DT=25, SCK=26)
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
              └─ Yes: scale1.get_units(1)  ← 單讀一筆（不忙等）
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
  r1/r2 推入各自的 5 筆中位數緩衝（-1.0 未就緒則略過）
  cached_weight1/2 = median1()/median2()    ← 中位數，存入 volatile RAM
  （last_read_time 已在區塊開頭設定）
  Serial 輸出
```

> **降溫重構**：原本每次 `get_units(3)`（連讀 3 筆）會在 HX711 的 10 SPS 下忙等 ~200ms/顆；
> 改為單讀一筆後幾乎不忙等。平滑改由**軟體移動中位數**負責：每筆讀數推入 `DISPLAY_MEDIAN_WINDOW`（5）
> 筆的環形緩衝，`cached_weight` 取其**中位數**——中位數會完全濾掉偶發尖刺，而非被平均拉動。
> 讀取週期由 `SENSOR_READ_INTERVAL_MS`（500ms）控制；Core-1 `loop()` 以 `SENSOR_LOOP_DELAY_MS`（50ms）
> 節流，避免空轉發熱。CPU 時脈在 `setup()` 由 `setCpuFrequencyMhz(CPU_FREQ_MHZ)` 降至 80 MHz。
> HX711 存取由一個 mutex 序列化（顯示讀取與記錄讀取不會同時碰感測器）。

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

## 記錄讀取（readLogWeight，截尾平均）

連續顯示用中位數（便宜、抗尖刺）；但寫入永久記錄的值走**獨立、高精度**的路徑，與顯示解耦：

    readLogWeight1() / readLogWeight2()   ← 由 auto-logger 在記錄當下呼叫
           │
           ├─ 模擬模式：回傳一筆模擬值
           │
           └─ 真實模式：取得 HX711 mutex，讀 LOG_SAMPLE_COUNT（11）筆
                   │  （每筆 wait_ready_timeout 保護）
                   ├─ 去掉最高 LOG_TRIM_COUNT（2）、最低 2 筆
                   ├─ 剩 7 筆取平均（截尾平均：抗尖刺又保留平均降噪）
                   └─ 樣本不足 / 拿不到 mutex → 回退用 cached 值（不卡死）

- 忙等只在記錄當下發生（每顆 ~1.1 秒、兩顆依序），深睡模式一次喚醒一次、常開模式每小時一次 → 發熱可忽略。
- 因此**記錄值準確且不受顯示中位數的反應延遲影響**；Google Sheets 同步的就是這個值。

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
- **Auto-logger 例外**：寫入記錄時不讀 `getCachedWeight*()`，而是呼叫 `readLogWeight1/2()`（截尾平均，見上節）。`getCachedWeight*()`（中位數）僅供 Web / MQTT / OLED 的即時顯示。

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

## 倍率校正 (#30)

每個 HX711 + load cell 個體有製造誤差，需要單獨校正倍率才能正確把 raw ADC 值轉成 kg。

### 校正數學

```
weight_kg = (raw_reading - offset) / scale_factor
```

校正時：

```
new_scale_factor = (raw_with_known_weight - offset) / known_weight_kg
```

### NVS Fallback Chain

開機時 `initSensor()` 會：

```
1. 試讀 NVS scale_data:scale1
   ├─ 有 → 用 NVS 值
   └─ 沒 → fallback 到 LOADCELL1_SCALE_FACTOR (compile-time)
2. scale1.set_scale(factor)
3. 印出 serial: "Sensor 1 scale factor: NNN.NN (NVS|default)"
```

Sensor 2 同樣處理。**升級韌體不會弄丟校正值** — NVS 跟韌體 binary 是分開的 flash 區段。

### NVS Keys

| Namespace | Key | Type | 用途 |
|---|---|---|---|
| `scale_data` | `offset` | long | Sensor 1 絕對零點 (既有) |
| `scale_data` | `offset2` | long | Sensor 2 絕對零點 (既有) |
| `scale_data` | `scale1` | float | Sensor 1 倍率 (新) |
| `scale_data` | `scale2` | float | Sensor 2 倍率 (新) |
| `scale_data` | `record_id` | long | 記錄 ID 計數器 (既有) |

### 校正流程（使用者操作）

1. **清空感測器** — 確保 load cell 上完全沒東西
2. **點「設定零點」**（既有的絕對歸零）— offset 寫入 NVS
3. **放上已知重量** — 建議 5~20 kg 標準砝碼
4. **在「已知重量 kg」欄輸入該重量** — 例如 `5`
5. **點「校正倍率」** — 後端讀 10 筆 raw 平均，算出新倍率
6. **觀察主頁讀數** — 應該 ≈ 已知重量（誤差 < 1%）

兩個 sensor 各自獨立校正，互不影響。

### 錯誤處理摘要

| 情境 | 後端回應 | 系統行為 |
|---|---|---|
| 重量為空 / 0 / 負數 | （前端攔截）| 紅字「請輸入有效的重量」 |
| 重量 > 200 kg | （HTML5 max 攔截）| 瀏覽器原生提示 |
| 沒放重物（delta < 1000 raw counts）| 400 `{"error":"未偵測到負載 — 確認重物已放上"}` | 紅字錯誤訊息；NVS 不變 |
| 計算倍率超範圍 (1000~500000 之外) | 400 `{"error":"計算倍率超出合理範圍 — 檢查接線"}` | 紅字錯誤訊息；NVS 不變 |
| Sensor 沒回應 | 400 `{"error":"感測器無回應"}` | 紅字錯誤訊息 |
| 連點按鈕 | 前端 button 立即 disable | 只送出一次請求 |

### Strategy

**只在校正成功時才寫 NVS**。任何錯誤路徑都不污染 NVS，使用者重試或重開機可以回到上一個有效狀態。與 #28 WiFi credentials 相同模式。

### 相關檔案

- `include/config.h` — `LOADCELL1_SCALE_FACTOR`、`LOADCELL2_SCALE_FACTOR` (compile-time fallback)
- `include/storage/nvs_storage.h` / `src/storage/nvs_storage.cpp` — `saveScaleFactor1/2`、`getScaleFactor1/2`、`hasScaleFactor1/2`
- `include/sensor_manager.h` / `src/sensor_manager.cpp` — `calibrateScaleFactor1/2(knownKg, errorOut)`
- `src/web_server_logic.cpp` — `/calibrate-scale1`、`/calibrate-scale2` endpoints
- `src/web_pages.cpp` — UI 「感測器 N 校正」摺疊面板內的倍率校正區塊

### 相關 issue

#30 — Runtime scale factor calibration via web UI for each load cell
