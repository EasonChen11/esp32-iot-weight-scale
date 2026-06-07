<div align="center">

# ESP32 IoT Weight Scale

A WiFi-enabled beehive weight monitoring system using ESP32 and dual HX711 load cell sensors.
Weight data is served on a built-in web dashboard and optionally streamed to an MQTT broker for Node-RED visualisation.

[![MIT License][license-shield]][license-url]
![Status](https://img.shields.io/badge/Status-Active-green?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C++-blue?style=for-the-badge)
![Hardware](https://img.shields.io/badge/Hardware-ESP32-FF7139?style=for-the-badge&logo=espressif&logoColor=white)

</div>

## Key Features

- **Dual HX711 Sensors** — two independent load cells with per-sensor calibration
- **Runtime Sensor Calibration** — adjust scale factor and absolute zero per-sensor from the web UI; persisted to NVS, no re-flash required. See `information/02-sensor-flow.md`
- **Web Dashboard** — built-in HTTP UI with live charts (Chart.js from LittleFS)
- **OLED Display** — SSD1306 auto-cycles Total / Sensor 1 / Sensor 2
- **Auto-Logging** — startup + hourly records, RAM-cached, persisted to LittleFS (hourly disabled in deep sleep mode)
- **Google Sheets Sync** — auto-upload records to Google Sheets via Apps Script with deduplication
- **MQTT + Node-RED** — optional live stream to a Docker-based dashboard
- **NTP Time Sync** — automatic via NTP; browser `/sync` as AP-mode fallback
- **Scheduled Deep Sleep** — wake at configured times or by button press
- **Simulation Mode** — random-fluctuation test mode without hardware

> For detailed architecture, data flows, and module internals see the [**Wiki**](https://github.com/EasonChen11/esp32-iot-weight-scale/wiki).

---

## Hardware

| Component | Signal | GPIO | Notes |
|-----------|--------|------|-------|
| HX711 #1 (left) | DT | 13 | |
| HX711 #1 (left) | SCK | 14 | |
| HX711 #2 (right) | DT | 25 | RTC GPIO — holds state in deep sleep |
| HX711 #2 (right) | SCK | 26 | RTC GPIO — holds state in deep sleep |
| SSD1306 OLED | SDA | 21 | ESP32 default I2C |
| SSD1306 OLED | SCL | 22 | |
| SSD1306 OLED | VCC | 19 | GPIO as power (~20 mA, within 40 mA limit) |
| Wake button | Signal | 32 | `DEEP_SLEEP_ENABLED` — ext0, active LOW |
| Wake button | GND | 33 | OUTPUT LOW as button GND |

**Required parts:** ESP32 dev board, 2x HX711 modules, 2x load cells.
**Optional:** SSD1306 0.96" OLED (I2C), wake-up button.

---

## Getting Started

### 1. Configure

Edit [`include/config.h`](include/config.h) — set your WiFi credentials and calibration values:

```cpp
const char *const STA_WIFI_SSID = "YOUR_SSID";
const char *const STA_WIFI_PASS = "YOUR_PASS";

const float LOADCELL1_SCALE_FACTOR = 85000.0;  // raw_reading / known_weight_kg
const float LOADCELL2_SCALE_FACTOR = 85000.0;
```

### 2. Upload LittleFS image (Chart.js)

```bash
platformio run --target uploadfs --upload-port /dev/ttyUSB0
```

### 3. Build and upload firmware

```bash
platformio run --target upload --upload-port /dev/ttyUSB0
```

### 4. Monitor

```bash
platformio device monitor --baud 115200
```

After boot, open `http://<ESP32-IP>` (STA) or `http://192.168.4.1` (AP mode) in a browser.

---

## Feature Switches

All modules can be toggled independently in `config.h`:

```cpp
#define WIFI_ENABLED            true
#define WEB_SERVER_ENABLED      true
#define WIFI_CONFIG_ENABLED     true
#define OTA_ENABLED             true
#define MQTT_ENABLED            false
#define AUTO_LOGGER_ENABLED     true
#define OLED_ENABLED            true
#define DEV_MODE_ENABLED        true
#define GOOGLE_SHEETS_ENABLED   true
#define NTP_ENABLED             true
#define SCHEDULE_ENABLED        true
#define DEEP_SLEEP_ENABLED      false
#define SIMULATE_SENSOR         false
```

Dependencies: `WEB_SERVER_ENABLED`, `MQTT_ENABLED`, `NTP_ENABLED`, and `GOOGLE_SHEETS_ENABLED` all require `WIFI_ENABLED`.
`WIFI_CONFIG_ENABLED` requires `WIFI_ENABLED` and `WEB_SERVER_ENABLED`, and adds a heartbeat status indicator to the main dashboard with a runtime WiFi configuration subpage at `/network`.
`DEEP_SLEEP_ENABLED` benefits from `SCHEDULE_ENABLED` + `NTP_ENABLED` for timed wake-ups.
`GOOGLE_SHEETS_ENABLED` requires a Google Apps Script URL **and** a matching shared token. The device reads them **NVS-first** (provisioned over serial — see below), falling back to the compile-time `GOOGLE_SHEETS_URL` + `GOOGLE_SHEETS_TOKEN` in `config_secrets.h`. Because public/OTA builds ship with placeholder secrets, provision the real values over serial so they persist in NVS across OTA updates. See [`google_sheets/README.md`](google_sheets/README.md) for the Apps Script deployment.
`OTA_ENABLED` requires `WIFI_ENABLED` and outbound internet. On boot/wake the device
fetches `manifest.json` from the GitHub `latest` release, and if its `version` is newer
than the compiled `FIRMWARE_VERSION` (set in `platformio.ini`), downloads `firmware.bin`,
verifies its sha256 over HTTPS, writes it to the inactive OTA slot, and reboots. A new
image is marked valid only after it boots through `setup()`; otherwise it rolls back.

**Releasing an update (tag-driven, automated):** the `release.yml` GitHub Actions workflow
turns a pushed version tag into a published release. The version lives in one place —
`FIRMWARE_VERSION` in `platformio.ini` — and the tag must match it.

```bash
# 1. Bump the version in platformio.ini:  -DFIRMWARE_VERSION=\"1.2.0\"
# 2. Commit the bump
git commit -am "release: v1.2.0"
# 3. Tag and push the tag — CI does the rest
git tag v1.2.0
git push github v1.2.0
```

On a `v*.*.*` tag, the workflow: verifies the tag matches `FIRMWARE_VERSION` (fails loudly
on mismatch), runs `pio run`, generates `manifest.json` via `scripts/make_manifest.py`, and
publishes a GitHub Release with `firmware.bin` + `manifest.json` as assets. The newest
release becomes "latest", which is exactly where the device's permanent OTA URL points.

> **Manual fallback** (no CI): `pio run`, then
> `python scripts/make_manifest.py <version> .pio/build/esp32dev/firmware.bin`, then
> `gh release create v<version> .pio/build/esp32dev/firmware.bin .pio/build/esp32dev/manifest.json --generate-notes`.
> The release must be marked "latest" because the device reads `releases/latest/...`.

> ⚠️ Switching to the OTA partition layout (`partitions.csv`) requires a one-time USB
> `erase + uploadfs + upload`. This wipes NVS (calibration offsets, schedules) and stored
> records, so re-calibrate and reset schedules once after the first OTA-capable flash.
> Records already sync to Google Sheets, so record loss is recoverable.

### Developer Mode

Some destructive endpoints (`POST /factory-reset`, `POST /network/clear`) are hidden from the user UI and rejected with HTTP 403 unless the device is in **Developer Mode**. This mode lives in RAM only and resets to USER on every reboot — so a device cannot accidentally ship to a customer with developer privileges left on.

**Switching via serial monitor (115200 baud):**

| Command | Effect |
|---|---|
| `dev-on` | Enable developer mode |
| `dev-off` | Disable developer mode |
| `dev-status` | Print current mode |

**Google Sheets provisioning (serial, 115200 baud):** the Sheets URL/token are operator/service configuration, **not** exposed on any web page. Set them once over USB serial; they are stored in NVS and survive reboots and OTA updates.

| Command | Effect |
|---|---|
| `sheets-set <url> <token>` | Save the Apps Script URL + shared token to NVS |
| `sheets-status` | Show whether NVS config exists (token shown masked) |
| `sheets-clear` | Erase the NVS config; revert to the compile-time fallback |

> Requires `DEV_MODE_ENABLED` (default on) so the serial dispatcher is active. The token is never echoed in plaintext. For initial provisioning, build with `DEV_MODE_ENABLED true`.

When dev mode flips, any open dashboard tab auto-reloads within ~1 s (via `/tick` polling) so the developer buttons appear or disappear automatically.

**Developer-only endpoints:**

- `POST /factory-reset` — Clears records and resets the ID counter; preserves calibration, WiFi, schedule.
- `POST /factory-reset?full=1` — Above + clears WiFi NVS + schedule. Calibration still preserved.
- `POST /network/clear` — Clears WiFi NVS so the device falls back to the compile-time SSID at next boot.

**⚠️ Google Sheets warning:** After a `/factory-reset`, manually clear the matching rows in your Google Sheet. The receiver script dedups by ID and silently acks duplicates, which would cause silent data loss on the next sync if the new ID-1 record collides with the old one.

> For a full explanation of each switch and their interactions, see the [**Wiki — System Overview**](https://github.com/EasonChen11/esp32-iot-weight-scale/wiki).

---

## PC Dashboard (Docker)

Requires Docker, `MQTT_ENABLED true`, and PC at `192.168.x.x` on the same network.

```bash
docker compose -f docker/docker-compose.mqtt.yml up -d
```

| Container | Purpose | Port |
|-----------|---------|------|
| `esp32-mosquitto` | MQTT broker | 1884 |
| `esp32-nodered` | Node-RED dashboard | 1880 |

Open **http://localhost:1880/ui** for live gauges and history charts.

```bash
docker compose -f docker/docker-compose.mqtt.yml down   # stop
```

---

## Project Structure

```
├── include/
│   ├── config.h                 # Pins, WiFi, MQTT, feature switches
│   ├── sensor_manager.h
│   ├── wifi_manager.h
│   ├── mqtt_manager.h
│   ├── oled_manager.h
│   ├── auto_logger.h
│   ├── google_sheets_manager.h
│   ├── deep_sleep_manager.h
│   ├── schedule_manager.h
│   ├── web_server_logic.h
│   ├── web_pages.h
│   └── storage/
│       ├── storage_init.h
│       ├── littlefs_storage.h
│       └── nvs_storage.h
├── src/                         # Implementation files (mirrors include/)
├── data/
│   └── chart.min.js.gz          # Chart.js (uploaded to LittleFS)
├── docker/
│   ├── docker-compose.mqtt.yml
│   └── mosquitto/config/
├── nodered_data/                # Node-RED flows + dashboard
├── information/                 # Architecture docs (auto-synced to Wiki)
└── platformio.ini
```

---

## Troubleshooting

| Symptom | Fix |
|---------|-----|
| No weight reading | Check `config.h` pin assignments and scale factor |
| WiFi not connecting | Update `STA_WIFI_SSID` / `STA_WIFI_PASS` |
| MQTT failed (rc=-2) | Start broker: `docker compose -f docker/docker-compose.mqtt.yml up -d` |
| Chart not loading | Upload LittleFS: `platformio run --target uploadfs` |
| Time not synced | Connect STA WiFi for NTP, or open web UI for browser `/sync` |

---

## Development

```bash
platformio run                                              # build
platformio run --target upload --upload-port /dev/ttyUSB0   # flash
platformio run --target uploadfs --upload-port /dev/ttyUSB0 # upload LittleFS
platformio run --target clean                               # clean
platformio device monitor --baud 115200                     # serial monitor
```

---

<div align="center">

**[Back to top](#esp32-iot-weight-scale)**

</div>

[license-shield]: https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge
[license-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/blob/main/LICENSE
