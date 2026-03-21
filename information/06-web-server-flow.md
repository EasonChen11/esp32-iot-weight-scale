# Web 伺服器模組流程 (web_server_logic + web_pages)

## 架構

```
┌──────────────────────────────────────────────────────────────────┐
│                  Web Server (Port 80, Core 0)                    │
├──────────────┬──────────────┬────────────────────────────────────┤
│  Static      │  Data API    │  Control API                       │
│  /           │  /data       │  /tare, /tare1, /tare2             │
│  /chartjs    │  /data1      │  /set-zero1, /set-zero2            │
│              │  /data2      │  /sync                             │
│              │  /time       │  /add-record, /del-record          │
│              │  /get-records│  /clear-records                    │
│              │              │  /get-schedule, /add-schedule      │
│              │              │  /del-schedule                     │
└──────────────┴──────────────┴────────────────────────────────────┘
```

## 路由註冊流程

```
initWebRoutes(server)
       │
       ├─ server.on("/")          → 回傳 getIndexHTML()  (整頁 HTML)
       ├─ server.on("/chartjs")   → LittleFS 讀取 chart.min.js.gz (gzip + 24h 快取)
       │
       ├─ server.on("/data")      → getCachedWeight()   (RAM)
       ├─ server.on("/data1")     → getCachedWeight1()  (RAM)
       ├─ server.on("/data2")     → getCachedWeight2()  (RAM)
       │
       ├─ server.on("/tare1")     → tareSensor1()
       ├─ server.on("/tare2")     → tareSensor2()
       ├─ server.on("/tare")      → tareSensor()
       │
       ├─ server.on("/sync")      → settimeofday()      (設定系統時鐘)
       ├─ server.on("/time")      → strftime()          (回傳目前時間)
       │
       ├─ server.on("/get-records")   → getRecordsJson()         (RAM 快取)
       ├─ server.on("/add-record")    → addRecordToStorage()     (RAM + Flash)
       ├─ server.on("/del-record")    → deleteRecordFromStorage() (RAM + Flash)
       ├─ server.on("/clear-records") → clearRecordsInStorage()  (RAM + Flash)
       │
       ├─ server.on("/set-zero1") → captureAbsoluteOffset1() + saveAbsoluteOffset()  (NVS)
       ├─ server.on("/set-zero2") → captureAbsoluteOffset2() + saveAbsoluteOffset2() (NVS)
       │
       ├─ server.on("/get-schedule")  → getScheduleJson()        (RAM)
       ├─ server.on("/add-schedule")  → addScheduleEntry(h, m)   (RAM + NVS)
       └─ server.on("/del-schedule")  → removeScheduleEntry(i)   (RAM + NVS)
       │
       ▼
  server.begin()
```

## 排程端點詳細

```
GET /get-schedule
  → 回傳 JSON: [{"h":7,"m":0},{"h":19,"m":0}]

GET /add-schedule?h=7&m=0
  → 驗證 h=0~23, m=0~59
  → 檢查重複
  → 成功: 200 + 更新後的 JSON
  → 失敗: 400 "Invalid or duplicate time"

GET /del-schedule?i=0
  → 刪除指定索引
  → 回傳更新後的 JSON
```

## 網頁前端運作流程

```
瀏覽器載入 http://<ESP32-IP>/
       │
       ▼
  收到 HTML (getIndexHTML)
       │
       ├─ <script src="/chartjs" defer>   ← 載入 Chart.js (gzip, 69KB)
       │                                     瀏覽器快取 24 小時
       │
       ▼
  window.onload 觸發
       │
       ├─ makeChart('chartTotal', '#27ae60')   ← 建立圖表
       ├─ fetchRecords()                        ← GET /get-records
       ├─ populateSelects()                     ← 填入 0~23 時 / 0~59 分選單
       ├─ fetchSchedule()                       ← GET /get-schedule
       ├─ fetch('/sync?t=' + timestamp)         ← 同步時間到 ESP32
       └─ setInterval(fetchRecords, 60000)      ← 每 60 秒更新紀錄表
       │
       ▼
  setInterval (每 500ms)
       │
       ├─ fetch('/data1') + fetch('/data2')     ← 並行請求兩個感測器
       ├─ 計算 total = v1 + v2
       ├─ 更新三個數字顯示
       └─ pushChart(chartTotal, total)          ← 推入圖表新資料點
```

## 資料流向

```
               每 500ms                        每 60s
瀏覽器 ──→ /data1, /data2 ──→ RAM 快取    瀏覽器 ──→ /get-records ──→ RAM 快取
               │                                         │
               ▼                                         ▼
          即時重量顯示                               紀錄表格更新
          + 圖表更新

        使用者操作                              使用者操作
瀏覽器 ──→ /add-record ──→ RAM + Flash    瀏覽器 ──→ /tare1 ──→ HX711 RAM
瀏覽器 ──→ /del-record ──→ RAM + Flash    瀏覽器 ──→ /set-zero1 ──→ NVS Flash

        排程操作
瀏覽器 ──→ /add-schedule ──→ RAM + NVS
瀏覽器 ──→ /del-schedule ──→ RAM + NVS
```

## Chart.js 載入優化

```
舊設計：
  <script src="https://cdn.jsdelivr.net/...">  ← 需要網路，AP 模式不可用

新設計：
  <script src="/chartjs" defer>
       │
       ▼
  ESP32 回應：
    Content-Encoding: gzip        ← 瀏覽器自動解壓
    Cache-Control: max-age=86400  ← 24 小時快取
    Body: chart.min.js.gz (69KB)  ← 從 LittleFS 讀取

  優點：
  - AP 模式也能顯示圖表
  - 首次載入 69KB（原本 204KB）
  - 後續載入 0KB（瀏覽器快取）
  - defer 確保不阻擋頁面渲染
```

## 四面板 UI 結構

```
┌───────────────────────────────────────────────────────────────┐
│                  System time: HH:MM:SS                        │
├───────────────┬───────────────┬───────────────┬───────────────┤
│  Sensor 1     │  Sensor 2     │  Total Weight │  Wake-up      │
│               │               │               │  Schedule     │
│  12.34 kg     │  11.89 kg     │  24.23 kg     │               │
│               │               │  ┌─────────┐  │  [HH]:[MM]   │
│  [Tare S1]    │  [Tare S2]    │  │  Chart  │  │  [Add]       │
│               │               │  └─────────┘  │               │
│  > Calibrate  │  > Calibrate  │  [Record]     │  07:00  [×]  │
│               │               │               │  19:00  [×]  │
│               │               │  # Time Weight │               │
│               │               │  1 14:30 24.2  │  Daily deep- │
│               │               │  2 13:00 23.9  │  sleep wake  │
│               │               │  [Clear All]  │  times (max  │
│               │               │               │  10)          │
└───────────────┴───────────────┴───────────────┴───────────────┘
```

排程面板（紫色主題 `#9b59b6`）功能：
- 時/分下拉選單（0~23 時，0~59 分）
- 新增按鈕 → POST `/add-schedule?h=&m=`
- 列表顯示所有排程，每項有刪除按鈕
- 自動從 `/get-schedule` 載入已儲存的排程
