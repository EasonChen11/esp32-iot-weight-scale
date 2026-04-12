# 儲存邏輯總覽

## 三種儲存機制

```
┌───────────────┬──────────────────┬─────────────────────────────┐
│     RAM       │    LittleFS      │         NVS                 │
│   (memory)    │  (Flash FS)      │  (Non-Volatile Storage)     │
├───────────────┼──────────────────┼─────────────────────────────┤
│ lost on reset │ survives reset   │ survives reset              │
│ very fast R/W │ fast R, slow W   │ fast R, slow W              │
│ limited size  │ uses flash space │ uses NVS partition          │
├───────────────┼──────────────────┼─────────────────────────────┤
│ cached_       │ /records.json    │ scale_data/offset           │
│  weight1/2    │ /chart.min.js.gz │ scale_data/offset2          │
│ cachedRec-    │                  │ scale_data/scale1           │
│  ordsJson     │                  │ scale_data/scale2           │
│               │                  │ schedule/times              │
└───────────────┴──────────────────┴─────────────────────────────┘

RAM      = cached_weight1/2, cachedRecordsJson (lost on power off)
LittleFS = /records.json, /chart.min.js.gz   (survives power off)
NVS      = scale_data/offset, offset2        (survives power off)
           scale_data/scale1, scale2         (survives power off)
           schedule/times                    (survives power off + deep sleep)
```

## 1. RAM — 即時資料

| 變數 | 檔案 | 寫入者 | 讀取者 | 用途 |
|------|------|--------|--------|------|
| `cached_weight1` | sensor_manager.cpp | Core 1 每 500ms | Web, MQTT, OLED, Logger | Sensor 1 最新重量 |
| `cached_weight2` | sensor_manager.cpp | Core 1 每 500ms | Web, MQTT, OLED, Logger | Sensor 2 最新重量 |
| `cachedRecordsJson` | littlefs_storage.cpp | 開機載入 / 寫入時更新 | Web `/get-records` | 紀錄 JSON 快取 |
| `lastRecordedHour` | auto_logger.cpp | 每小時更新 | auto_logger 自用 | 防止重複記錄 |
| `bootTime` | auto_logger.cpp | 開機設定 | auto_logger 自用 | 開機時間戳 |
| `startupRecordDone` | auto_logger.cpp | 開機紀錄完成後 | auto_logger 自用 | 避免重複開機紀錄 |
| `entries[]` | schedule_manager.cpp | 開機載入 / Web 新增刪除 | deep_sleep_manager | 喚醒排程陣列 |
| `bootTimeMs` | deep_sleep_manager.cpp | 開機設定 | handleDeepSleep 自用 | 清醒計時起點 |

## 2. LittleFS — 紀錄持久化

### /records.json

格式：JSON 陣列，最新在最前面，最多 `MAX_RECORDS = 10` 筆

```json
[
  {"time": "14:30:00", "weight": "24.235"},
  {"time": "13:00:00", "weight": "23.890"},
  ...
]
```

### 讀寫流程

```
開機
  │
  ▼
LittleFS.begin(true)
  │
  ▼
loadRecordsCache()
  ├─ 檔案不存在或空 → cachedRecordsJson = "[]"
  └─ 正常 → cachedRecordsJson = file.readString()
                                     ↓
        ┌──────────────────────────────────────────────────┐
        │  All subsequent reads (zero Flash I/O):          │
        │                                                  │
        │  getRecordsJson()                                │
        │    -> return cachedRecordsJson  (from RAM)       │
        └──────────────────────────────────────────────────┘
        ┌───────────────────────────────────────────────────┐
        │  Writes (add / delete / clear):                   │
        │                                                   │
        │  1. deserialize cachedRecordsJson -> JsonDocument │
        │  2. modify JSON array                             │
        │  3. saveJsonToFile(doc)  -> write /records.json   │
        │  4. updateCache(doc)    -> update RAM cache       │
        └───────────────────────────────────────────────────┘
```

### 新增紀錄的細節 (addRecordToStorage)

```
收到 time + weight
       │
       ▼
解析目前的 cachedRecordsJson → 舊陣列
       │
       ▼
建立新紀錄 { time, weight }
       │
       ▼
組合新陣列：[新紀錄, ...舊紀錄]
       │
       ├─ 超過 MAX_RECORDS (10) → 截斷尾部
       │
       ▼
saveJsonToFile()  ← 寫 flash
updateCache()     ← 更新 RAM
```

### /chart.min.js.gz

- 唯讀檔案，69KB gzip 壓縮的 Chart.js library
- 透過 PlatformIO `uploadfs` 上傳到 LittleFS
- Web 端 `/chartjs` 路由提供，帶 `Content-Encoding: gzip` 和 24 小時快取
- 程式不會修改此檔案

## 3. NVS (Preferences) — 校準偏移量 + 排程

### 校準偏移量（零點）

```
寫入路徑（使用者校準時）：
  /set-zero1 → captureAbsoluteOffset1() → saveAbsoluteOffset(offset)
                                              │
                                              ▼
                                    Preferences.begin("scale_data")
                                    Preferences.putLong("offset", value)
                                    Preferences.end()

讀取路徑（僅開機時）：
  setup() → getAbsoluteOffset()
              │
              ▼
    Preferences.begin("scale_data", readonly=true)
    Preferences.getLong("offset", default=0)
    Preferences.end()
              │
              ▼
    initSensor(offset1, offset2) → scale.set_offset()
```

### 倍率校正（scale factor）

```
寫入路徑（使用者校正時）：
  /calibrate-scale1 → calibrateScaleFactor1(knownWeight, err)
                          │
                          ├─ 驗證 knownWeight > 0.01
                          ├─ 驗證 rawReading > MIN_RAW_THRESHOLD (50)
                          ├─ 計算 newFactor = rawReading / knownWeight
                          ├─ 驗證 newFactor 在 1000 ~ 500000 之間
                          │
                          ▼
                      saveScaleFactor1(newFactor)
                          │
                          ▼
                      Preferences.begin("scale_data")
                      Preferences.putFloat("scale1", factor)
                      Preferences.end()
                          │
                          ▼
                      scale1.set_scale(newFactor) ← 即時套用

讀取路徑（僅開機時）：
  setup() → initSensor()
              │
              ▼
    hasScaleFactor1() → getScaleFactor1()
              │
              ├─ NVS 有值 → 使用 NVS 值
              └─ NVS 無值 → 使用 config.h 編譯期預設值
              │
              ▼
    scale1.set_scale(factor)
```

### 喚醒排程

```
寫入路徑（Web 新增/刪除排程時）：
  /add-schedule → addScheduleEntry(h, m)
                    │
                    ▼
                  _saveToNVS()
                    │
                    ▼
                  Preferences.begin("schedule")
                  Preferences.putString("times", "07:00,19:00")
                  Preferences.end()

讀取路徑（僅開機時）：
  setup() → initSchedule() → _loadFromNVS()
              │
              ▼
    Preferences.begin("schedule", readonly=true)
    Preferences.getString("times", default="")
    Preferences.end()
              │
              ▼
    解析 CSV → 排序 → entries[] 陣列
```

| NVS 命名空間 | Key | 用途 | 寫入時機 | 讀取時機 |
|-------------|-----|------|----------|----------|
| `scale_data` | `offset` | Sensor 1 零點偏移 | `/set-zero1` 校準 | 開機 1 次 |
| `scale_data` | `offset2` | Sensor 2 零點偏移 | `/set-zero2` 校準 | 開機 1 次 |
| `scale_data` | `scale1` | Sensor 1 倍率 | `/calibrate-scale1` 校正 | 開機 1 次 |
| `scale_data` | `scale2` | Sensor 2 倍率 | `/calibrate-scale2` 校正 | 開機 1 次 |
| `schedule` | `times` | 喚醒時間 CSV | Web 新增/刪除排程 | 開機 1 次 |

## 完整資料流向圖

```
HX711 #1 ──→ scale1.get_units(3) ──→ cached_weight1 (RAM)
                                          │
HX711 #2 ──→ scale2.get_units(3) ──→ cached_weight2 (RAM)
                                          │
              ┌───────────────────────────┤
              │           │               │              │
              ▼           ▼               ▼              ▼
         Web /data    MQTT publish    OLED 顯示    Auto-logger
              │                                         │
              │                                         ▼
              │                              addRecordToStorage()
              │                                    │         │
              │                                    ▼         ▼
              │                               RAM 快取   LittleFS
              │                                    │    /records.json
              │                                    │
              ▼                                    ▼
         Web /get-records ← getRecordsJson() ← cachedRecordsJson (RAM)

NVS (schedule/times) ──→ entries[] (RAM) ──→ getNextWakeupSeconds()
                                                    │
                                                    ▼
                                          deep_sleep_manager
                                          esp_sleep_enable_timer_wakeup()
```
