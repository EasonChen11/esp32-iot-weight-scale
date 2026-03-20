# 儲存邏輯總覽

## 三種儲存機制

```
┌────────────────────────────────────────────────────────────────┐
│                        儲存架構                                │
├─────────────┬──────────────────┬───────────────────────────────┤
│    RAM      │    LittleFS      │         NVS                   │
│  (記憶體)    │  (Flash 檔案系統) │   (Non-Volatile Storage)      │
├─────────────┼──────────────────┼───────────────────────────────┤
│ 斷電消失     │ 斷電保留          │ 斷電保留                      │
│ 讀寫極快     │ 讀快 寫慢         │ 讀快 寫慢                     │
│ 容量有限     │ 使用 flash 空間   │ 使用 NVS 分區                 │
├─────────────┼──────────────────┼───────────────────────────────┤
│ cached_     │ /records.json    │ scale_data/offset             │
│  weight1/2  │ /chart.min.js.gz │ scale_data/offset2            │
│ cachedRec-  │                  │                               │
│  ordsJson   │                  │                               │
└─────────────┴──────────────────┴───────────────────────────────┘
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
        ┌────────────────────────────────────────────────┐
        │            之後所有的讀取                        │
        │                                                │
        │  getRecordsJson()                              │
        │    → return cachedRecordsJson   ← 零 Flash I/O  │
        └────────────────────────────────────────────────┘
        ┌────────────────────────────────────────────────┐
        │            寫入（新增/刪除/清空）                 │
        │                                                │
        │  1. 解析 cachedRecordsJson → JsonDocument       │
        │  2. 修改 JSON 陣列                              │
        │  3. saveJsonToFile(doc) → 寫入 /records.json    │
        │  4. updateCache(doc)   → 更新 cachedRecordsJson │
        └────────────────────────────────────────────────┘
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

## 3. NVS (Preferences) — 校準偏移量

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

| NVS Key | 用途 | 寫入時機 | 讀取時機 |
|---------|------|----------|----------|
| `scale_data/offset` | Sensor 1 零點偏移 | `/set-zero1` 校準 | 開機 1 次 |
| `scale_data/offset2` | Sensor 2 零點偏移 | `/set-zero2` 校準 | 開機 1 次 |

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
```
