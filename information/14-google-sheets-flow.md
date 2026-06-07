# Google Sheets Sync Flow

## Overview

The Google Sheets module (`google_sheets_manager`) syncs weight records from LittleFS flash storage to a Google Sheets spreadsheet via a Google Apps Script (GAS) web app endpoint. Each record is tracked with a `synced` flag to ensure exactly-once delivery.

## Architecture

```
ESP32 (LittleFS)  ──HTTPS POST──▶  Google Apps Script  ──▶  Google Sheets
                  ◀──JSON response──  (returns received_ids)
```

## Record Format (LittleFS JSON)

```json
{
  "id": 1,
  "date": "2026-03-30",
  "time": "14:30:00",
  "sensor1": "25.300",
  "sensor2": "22.100",
  "synced": false
}
```

- `id`: Monotonic counter stored in NVS (persists across power cycles)
- `synced`: Set to `true` only after GAS confirms receipt

## Sync Trigger

### Automatic (once per boot)
1. Auto-logger saves startup record after `STARTUP_RECORD_DELAY_MS` (10s)
2. `handleGoogleSheetsSync()` detects `isStartupRecordDone() == true`
3. Collects all records where `synced == false`
4. POSTs batch to GAS endpoint
5. Marks confirmed IDs as synced

### Manual (web UI button)
- "Sync Sheets" button calls `/sync-sheets` endpoint
- Triggers `triggerGoogleSheetsSync()` which returns a status message

## GAS Communication Protocol

Google Apps Script returns HTTP 302 redirect after processing POST data. The redirect URL must be followed with **GET** (not POST) to retrieve the JSON response.

### Two-Step Flow
1. **POST** `{"token": "...", "records": [...]}` to GAS exec URL → receives **302** with `Location` header
2. **GET** the redirect URL → receives `{"success": true, "received_ids": [1,2,3], "count": 3}`

### Why Two Steps?
GAS architecture processes the POST body and writes to the sheet, then redirects to a `googleusercontent.com` echo endpoint that only accepts GET requests. Using `HTTPC_FORCE_FOLLOW_REDIRECTS` on the initial POST results in HTTP 400/405 errors because the redirect URL receives a POST instead of GET.

## GAS Script Contract

**POST body:**
```json
{"token": "<shared secret>", "records": [{"id": 1, "date": "2026-03-30", "time": "14:30:00", "sensor1": "25.300", "sensor2": "22.100"}]}
```

**Response:**
```json
{"success": true, "received_ids": [1], "count": 1}
```

- GAS performs deduplication by ID (won't insert duplicate rows)
- Only IDs listed in `received_ids` are marked synced on ESP32
- Total weight is calculated in the spreadsheet (not sent by ESP32)

## Configuration

| Setting | Location | Value |
|---------|----------|-------|
| Feature switch | `config.h` | `#define GOOGLE_SHEETS_ENABLED true` |
| GAS URL | `config_secrets.h` | `const char *const GOOGLE_SHEETS_URL = "https://..."` |
| Shared token | `config_secrets.h` | `const char *const GOOGLE_SHEETS_TOKEN = "..."` (must match `SHEETS_TOKEN` in GAS) |
| Max records | `config.h` | `MAX_RECORDS = 50` |

## Error Handling

| Scenario | Behaviour |
|----------|-----------|
| No WiFi (AP mode only) | Sync skipped — records stay unsynced |
| GAS returns partial success | Only confirmed IDs marked synced |
| HTTP failure / timeout | Records remain unsynced, retried next boot |
| Old record format (no `id` field) | Auto-migration: flash cleared on detection |
| NTP failure | Records still saved using RTC fallback time |

## NTP Callback Sync

NTP uses `sntp_set_time_sync_notification_cb()` for reliable sync detection instead of polling `sntp_get_sync_status()`. This resolves an issue where the polling loop would exit prematurely on ESP32.

## Files

| File | Role |
|------|------|
| `include/google_sheets_manager.h` | Public API |
| `src/google_sheets_manager.cpp` | HTTPS POST logic, sync orchestration |
| `src/storage/littlefs_storage.cpp` | Record CRUD with `synced` flag |
| `src/storage/nvs_storage.cpp` | Monotonic ID counter |
| `src/auto_logger.cpp` | Expanded record format (id, date, s1, s2) |
