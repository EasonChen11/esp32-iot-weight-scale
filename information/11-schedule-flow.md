# 排程管理模組流程 (schedule_manager)

## 功能

管理每日喚醒時間排程，儲存在 NVS flash 中，支援透過 Web UI 新增/刪除。
主要用途是搭配深度睡眠模組，讓 ESP32 在指定時間自動喚醒。

## 資料結構

```
struct ScheduleEntry {
    uint8_t hour;    // 0-23
    uint8_t minute;  // 0-59
};

static ScheduleEntry entries[MAX_SCHEDULE_ENTRIES];  // 最多 10 筆
static int entryCount = 0;
```

## NVS 儲存格式

```
命名空間: "schedule"
Key:      "times"
格式:     逗號分隔的 HH:MM 字串
範例:     "07:00,12:30,19:00"
```

## 初始化流程

```
initSchedule()
    │
    ▼
_loadFromNVS()
    │
    ├─ Preferences.begin("schedule", readonly)
    ├─ 讀取 "times" key → "07:00,19:00"
    ├─ Preferences.end()
    │
    ▼
解析 CSV 字串
    │
    ├─ "07:00" → { hour: 7, minute: 0 }
    ├─ "19:00" → { hour: 19, minute: 0 }
    │
    ▼
_sortEntries()  ← 按時間排序
    │
    ▼
Serial: "[Schedule] Loaded 2 wake-up entries"
```

## 新增排程

```
addScheduleEntry(hour, minute)
    │
    ├─ hour > 23 || minute > 59 ? → return false（無效時間）
    │
    ├─ entryCount >= MAX_SCHEDULE_ENTRIES ? → return false（已滿）
    │
    ├─ 已有相同時間 ? → return false（重複）
    │
    ▼
entries[entryCount] = { hour, minute }
entryCount++
    │
    ▼
_sortEntries()  ← 重新排序
    │
    ▼
_saveToNVS()
    │
    ├─ 組合 CSV: "07:00,12:30,19:00"
    ├─ Preferences.begin("schedule")
    ├─ Preferences.putString("times", csv)
    ├─ Preferences.end()
    │
    ▼
Serial: "[Schedule] Saved 3 entries: 07:00,12:30,19:00"
return true
```

## 刪除排程

```
removeScheduleEntry(index)
    │
    ├─ index < 0 || index >= entryCount ? → return false
    │
    ▼
陣列左移（覆蓋被刪除的元素）
entryCount--
    │
    ▼
_saveToNVS()  ← 儲存更新後的排程
return true
```

## 取得下一次喚醒秒數

```
getNextWakeupSeconds()
    │
    ├─ entryCount == 0 ? → return -1（沒有排程）
    │
    ├─ getLocalTime() 失敗 ? → return -1（時間未同步）
    │
    ▼
nowSec = hour * 3600 + min * 60 + sec    ← 現在是一天中的第幾秒
    │
    ▼
遍歷所有排程:
    for each entry:
        targetSec = entry.hour * 3600 + entry.minute * 60
        delta = targetSec - nowSec
        if delta <= 0: delta += 86400    ← 已過今天，算到明天
        取最小的 delta
    │
    ▼
return bestDelta（秒數）
```

### 計算範例

```
現在時間: 20:30:00 (73800 秒)
排程: 07:00 (25200), 19:00 (68400)

07:00: delta = 25200 - 73800 = -48600 → +86400 = 37800 (10.5 小時)
19:00: delta = 68400 - 73800 = -5400  → +86400 = 81000 (22.5 小時)

最小 delta = 37800 → 下一次喚醒在 10.5 小時後（明天 07:00）
```

## JSON 輸出

```
getScheduleJson()
    │
    ▼
"[{"h":7,"m":0},{"h":19,"m":0}]"
```

## Web 整合

```
前端 (web_pages.cpp)                  後端 (web_server_logic.cpp)
┌─────────────────────┐          ┌──────────────────────────────┐
│ 時/分 下拉選單       │          │                              │
│ [HH] : [MM] [Add]   │──GET──→ │ /add-schedule?h=7&m=0        │
│                      │         │   → addScheduleEntry(7, 0)   │
│ 07:00  [×]           │──GET──→ │ /del-schedule?i=0            │
│ 19:00  [×]           │         │   → removeScheduleEntry(0)   │
│                      │         │                              │
│ fetchSchedule()      │──GET──→ │ /get-schedule                │
│   ← renderSchedule() │←─JSON──│   → getScheduleJson()        │
└─────────────────────┘          └──────────────────────────────┘
```

## 驗證規則

| 檢查項目 | 條件 | 結果 |
|---------|------|------|
| 時間範圍 | hour 0~23, minute 0~59 | 超出 → 400 錯誤 |
| 重複檢查 | 已存在相同 hour + minute | 重複 → 400 錯誤 |
| 數量限制 | entryCount < MAX_SCHEDULE_ENTRIES (10) | 已滿 → 400 錯誤 |

## 設定參數

| 參數 | 值 | 定義位置 | 用途 |
|------|-----|---------|------|
| `SCHEDULE_ENABLED` | `true` | config.h | 功能開關 |
| `MAX_SCHEDULE_ENTRIES` | `10` | config.h | 最多排程數量 |
