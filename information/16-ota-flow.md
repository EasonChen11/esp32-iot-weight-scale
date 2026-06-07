# OTA (Over-the-Air) Firmware Update Flow

## Overview

The OTA module (`ota_manager`) pulls firmware updates automatically from GitHub Releases over HTTPS. Each boot/wake cycle, after WiFi connects, the ESP32 fetches a small `manifest.json`, compares the version against the running firmware, and — if a newer version exists — streams the binary, verifies its SHA-256, writes it to the inactive OTA partition, and reboots into the new image. If anything goes wrong (network error, truncated download, hash mismatch) the update is aborted and the current firmware continues running without any intervention needed.

## Architecture

```
ESP32 (Core 0 — WebAndTasksCode)
        │
        │  HTTPS GET  (WiFiClientSecure + Mozilla CA bundle)
        ▼
GitHub Releases  ──  manifest.json  ──► fetchManifest()
   (latest)                                   │
        │                                     ▼
        │                            version compare
        │                           (semver 3-field)
        │                                     │
        │         not newer ◄────────────────┤
        │                                     │ newer
        │  HTTPS GET  firmware.bin            ▼
        │◄────────────────────────── downloadAndApply()
        │                                     │
        │                            SHA-256 verify
        │                                     │
        │                     mismatch ◄──────┤
        │                                     │ match
        │                                     ▼
        │                            Update.end(true)
        │                                     │
        │                                     ▼
        │                              ESP.restart()
        │                           (boots new image)
        │
        ▼
  bootloader (ESP-IDF OTA)
  PENDING_VERIFY → initOTA() marks valid
                   (rollback cancelled)
```

## checkOtaUpdate() Step-by-Step Flow

```
WebAndTasksCode (Core 0)
        │
        │  first iteration where WiFi.status() == WL_CONNECTED
        ▼
checkOtaUpdate()   [guarded by static bool otaChecked + WIFI_ENABLED]
        │
        ▼
fetchManifest()
  ├─ WiFiClientSecure + setCACertBundle(rootca_crt_bundle_start)
  ├─ HTTPS GET OTA_MANIFEST_URL (15 s timeout, follow redirects)
  ├─ deserializeJson → {version, url, sha256}
  └─ validate: version/url non-empty, sha256 length == 64
        │
        ▼  manifest OK
isNewerVersion(FIRMWARE_VERSION, manifest.version)?
        │
        ├─ No  → "[OTA] already up to date"  → return
        │
        └─ Yes → otaInProgress = true
                      │
                      ▼
             downloadAndApply()
               ├─ HTTPS GET manifest.url (firmware.bin, CA-verified, 15 s timeout)
               ├─ Update.begin(total)  → write to inactive OTA slot
               ├─ stream loop (1024-byte chunks):
               │       mbedtls_sha256_update()  +  Update.write()
               │       stall guard: no data for OTA_HTTP_TIMEOUT_MS → break
               ├─ if written != total → Update.abort()  → return false
               ├─ SHA-256 compare: computed hex vs manifest.sha256
               │       mismatch → Update.abort()       → return false
               └─ match → Update.end(true)             → return true
                      │
                      ▼
             otaInProgress = false
             delay(500) → ESP.restart()
```

## Rollback Safety (initOTA)

`initOTA()` is called at the end of `setup()` (after WiFi, storage, and sensor init). If the running partition state is `ESP_OTA_IMG_PENDING_VERIFY` — meaning the device just booted into a freshly OTA-flashed image for the first time — it calls `esp_ota_mark_app_valid_cancel_rollback()`. Reaching `setup()` with all subsystems initialized is the self-test; a pass marks the image valid. If the firmware had panicked or failed to boot before reaching that call, the ESP-IDF bootloader would automatically roll back to the previous working image on the next reboot.

## Configuration

| Setting | Location | Value |
|---------|----------|-------|
| Feature switch | `config.h` | `#define OTA_ENABLED true` |
| Manifest URL | `config.h` | `OTA_MANIFEST_URL` = `https://github.com/EasonChen11/esp32-iot-weight-scale/releases/latest/download/manifest.json` |
| HTTP timeout | `config.h` | `OTA_HTTP_TIMEOUT_MS` = 15000 ms |
| Firmware version | `platformio.ini` | `-DFIRMWARE_VERSION=\"1.1.0\"` (build flag) |
| CA bundle | `cert/x509_crt_bundle.bin` | Mozilla root CAs, embedded via `board_build.embed_files` |
| Partition layout | `partitions.csv` | Custom dual-OTA (`app0`/`app1` = 0x190000 each) |

## Error Handling

| Scenario | Behaviour |
|----------|-----------|
| No WiFi / offline | `otaChecked` guard never fires — check skipped; all sensors and web server continue normally |
| HTTP error on manifest | Logged (`[OTA] manifest HTTP <code>`); update aborted; retried next boot |
| JSON parse error / missing fields | Logged; update aborted; current firmware continues |
| Manifest version not newer | Logged ("already up to date"); no-op |
| HTTP error on firmware download | Logged; `Update.abort()`; retried next boot |
| Download stalled (no data for 15 s) | Loop breaks; truncation check triggers `Update.abort()` |
| Truncated download (written != total) | `Update.abort()`; flash untouched; retried next boot |
| SHA-256 mismatch | `Update.abort()`; flash untouched; device not bricked; retried next boot |
| New image fails to boot (panic/hang before `initOTA()`) | ESP-IDF bootloader auto-rolls back to previous valid image on next reboot |

## Partition Layout

Custom `partitions.csv` replaces the default single-app layout to enable dual OTA slots:

```
nvs       data  nvs      0x9000   0x5000
otadata   data  ota      0xe000   0x2000
app0      app   ota_0    0x10000  0x190000   (1.5625 MB — active slot)
app1      app   ota_1    0x1A0000 0x190000   (1.5625 MB — OTA target slot)
spiffs    data  spiffs   0x330000 0xC0000    (768 KB — LittleFS)
coredump  data  coredump 0x3F0000 0x10000
```

The 63 KB Mozilla CA bundle is embedded in flash (counted inside the app slot). Firmware currently uses ~73% of one app slot.

### One-Time Partition Migration Warning

Switching from the default partition layout to this custom layout **requires a full USB erase + re-flash**:

```bash
pio run --target erase
pio run --target uploadfs   # re-flash LittleFS
pio run --target upload     # flash firmware
```

This wipes NVS (calibration offsets and schedules) and all LittleFS records. After the first OTA-capable USB flash:

1. Re-enter calibration offsets via the web UI.
2. Re-add wake schedules.
3. Records: already synced to Google Sheets; the flash copy is lost but the sheet is the source of truth.

Subsequent updates are OTA — no USB cable needed.

## Release Process (Maintainer)

1. Bump `-DFIRMWARE_VERSION` in `platformio.ini`.
2. Build: `pio run`
3. Generate manifest: `python scripts/make_manifest.py <version> .pio/build/esp32dev/firmware.bin`
4. Create a GitHub Release marked **"latest"** and attach `firmware.bin` and `manifest.json` as assets.

The `OTA_MANIFEST_URL` always resolves to the **latest** release assets via GitHub's redirect, so no URL change is needed between releases.

## Files

| File | Role |
|------|------|
| `include/ota_manager.h` | Public API (`initOTA`, `checkOtaUpdate`, `isOtaInProgress`) |
| `src/ota_manager.cpp` | HTTPS manifest fetch, semver compare, download+SHA256+write logic |
| `partitions.csv` | Custom dual-OTA partition table |
| `cert/x509_crt_bundle.bin` | Mozilla CA bundle embedded in flash for HTTPS verification |
| `scripts/make_manifest.py` | Generates `manifest.json` (version, URL, sha256) from a built binary |
| `include/config.h` | `OTA_ENABLED`, `OTA_MANIFEST_URL`, `OTA_HTTP_TIMEOUT_MS` |
| `platformio.ini` | `-DFIRMWARE_VERSION` build flag; `board_build.embed_files`; `board_build.partitions` |
