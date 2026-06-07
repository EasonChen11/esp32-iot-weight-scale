# Google Sheets Receiver

Google Apps Script (GAS) that receives batched weight records from the ESP32 and appends them to a spreadsheet.

## Files

- `google_sheet_script.js` — the GAS source. Paste into a `Code.gs` file at https://script.google.com/.

## Deploy

1. Open the target Google Sheet → Extensions → Apps Script.
2. Replace the default `Code.gs` contents with `google_sheet_script.js`.
3. Set `SHEETS_TOKEN` in the script to a secret of your choice — it **must match** `GOOGLE_SHEETS_TOKEN` in `include/config_secrets.h`. Requests with a wrong/missing token are rejected with `{"success":false,"error":"unauthorized"}`.
4. Deploy → New deployment → Type **Web app** → Execute as **Me** → Who has access **Anyone**.
5. Copy the deployment URL (`https://script.google.com/macros/s/.../exec`).
6. Paste it into `include/config_secrets.h` as `GOOGLE_SHEETS_URL`.

> Updating an existing deployment later: **Deploy → Manage deployments → edit (pencil) → Version: New version**. This keeps the same `/exec` URL, so you don't need to change `GOOGLE_SHEETS_URL`.

## Behaviour

- POST body: `{"token":"...","records":[{"id":..,"date":..,"time":..,"sensor1":..,"sensor2":..}, ...]}`
- Response: `{"success":true,"received_ids":[...],"count":N}`
- Dedup by ID — duplicate IDs are silently acked and skipped (see warning below).
- Spreadsheet column F (`Total`) is computed by formula `=D+E`.

## ⚠️ Silent data loss after `/factory-reset`

The dedup logic acks duplicates as `received_ids`, so the ESP32 marks them `synced=true` even though they were not written. After triggering the developer-only `/factory-reset` (which resets the ID counter back to 1), **manually delete the matching rows in the sheet first**, otherwise the next sync silently loses the new records.

See `information/14-google-sheets-flow.md` for the full sync flow and `information/15-dev-mode-flow.md` for the dev-mode background.
