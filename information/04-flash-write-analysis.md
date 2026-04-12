# Flash 寫入頻率分析

## 會寫入 Flash 的操作

整個系統有三個地方會寫入 flash：

### 1. LittleFS 寫入 — `/records.json`

| 觸發來源 | 呼叫的函式 | 頻率 | 類型 |
|----------|-----------|------|------|
| Auto-logger 開機紀錄 | `addRecordToStorage()` | 每次開機 1 次 | 自動 |
| Auto-logger 每小時紀錄 | `addRecordToStorage()` | 每小時 1 次 | 自動 |
| Web `/add-record` | `addRecordToStorage()` | 使用者手動 | 手動 |
| Web `/del-record` | `deleteRecordFromStorage()` | 使用者手動 | 手動 |
| Web `/clear-records` | `clearRecordsInStorage()` | 使用者手動 | 手動 |

### 2. NVS 寫入 — scale_data 命名空間

| 觸發來源 | 呼叫的函式 | 頻率 | 類型 |
|----------|-----------|------|------|
| Web `/set-zero1` | `saveAbsoluteOffset()` | 校準時才觸發 | 手動 |
| Web `/set-zero2` | `saveAbsoluteOffset2()` | 校準時才觸發 | 手動 |
| Web `/calibrate-scale1` | `saveScaleFactor1()` | 校正倍率時才觸發 | 手動 |
| Web `/calibrate-scale2` | `saveScaleFactor2()` | 校正倍率時才觸發 | 手動 |

### 3. NVS 寫入 — schedule 命名空間

| 觸發來源 | 呼叫的函式 | 頻率 | 類型 |
|----------|-----------|------|------|
| Web `/add-schedule` | `_saveToNVS()` | 新增排程時 | 手動 |
| Web `/del-schedule` | `_saveToNVS()` | 刪除排程時 | 手動 |

## 自動寫入量計算

### 持續運行模式（DEEP_SLEEP_ENABLED = false）

只有 auto-logger 會自動寫入 flash，手動操作忽略不計：

```
每天自動寫入次數 = 1（開機紀錄）+ 24（每小時紀錄）= 25 次/天
```

### 深度睡眠模式（DEEP_SLEEP_ENABLED = true）

假設設定 2 個喚醒時間（07:00 和 19:00）：

```
每天自動寫入次數 = 2（每次喚醒的開機紀錄）= 2 次/天
```

注：深度睡眠模式下，每次喚醒僅清醒 10 分鐘，不太可能跨整點，
因此每小時紀錄通常不會觸發。

## Flash 壽命評估

ESP32 使用的 SPI Flash 壽命規格：

| 參數 | 值 |
|------|-----|
| Flash 擦寫壽命 | ~100,000 次/sector |
| LittleFS 磨損均衡 | 有（分散寫入到不同 sector） |
| 每天寫入次數（持續模式） | ~25 次 |
| 每天寫入次數（睡眠模式） | ~2 次 |
| 每筆寫入大小 | < 1KB（JSON 陣列，最多 10 筆紀錄）|

```
持續模式壽命 = 100,000 ÷ 25 = 4,000 天 ≈ 11 年
睡眠模式壽命 = 100,000 ÷ 2 = 50,000 天 ≈ 137 年
```

加上 LittleFS 的 wear leveling，實際壽命會更長。
排程和校正 NVS 寫入均為手動操作（每月可能幾次），對壽命無影響。

## 讀取頻率（不影響 Flash 壽命）

讀取 flash 不會造成磨損，但舊設計每次讀取都碰 flash：

### 舊設計 vs 新設計對比

```
舊設計：
  Web 輪詢 /get-records（每 60 秒）──→ 讀 flash /records.json
  網頁載入                           ──→ 讀 flash /records.json
  結果：每分鐘至少 1 次 flash 讀取，多人同時開網頁會加倍

新設計（RAM 快取）：
  Web 輪詢 /get-records（每 60 秒）──→ return cachedRecordsJson（RAM）
  網頁載入                           ──→ return cachedRecordsJson（RAM）
  Flash 讀取：僅開機 1 次
```

### 讀取頻率比較

| 操作 | 舊設計 Flash 讀取 | 新設計 Flash 讀取 |
|------|------------------|------------------|
| 每次 `/get-records` | 1 次 | 0 次 |
| 網頁輪詢（每 60s） | 每分鐘 1 次 | 0 次 |
| 開機 | 1 次 | 1 次 |
| **每天總計（假設持續運行）** | **~1,440 次** | **1 次** |

## 結論

```
                 寫入頻率（持續模式）  寫入頻率（睡眠模式）  讀取頻率（Flash）
  LittleFS       ~25 次/天            ~2 次/天              1 次/天（僅開機）   ✅ 安全
  NVS (校準)     接近 0 次/天          接近 0 次/天           1 次/天（僅開機）   ✅ 安全
  NVS (倍率)     接近 0 次/天          接近 0 次/天           1 次/天（僅開機）   ✅ 安全
  NVS (排程)     接近 0 次/天          接近 0 次/天           1 次/天（僅開機）   ✅ 安全
```

目前方案對 Flash 壽命沒有影響。主要改善來自 RAM 快取消除了所有常規的 Flash 讀取。
