<div align="center">

# ESP32 IoT Weight Scale

A WiFi-enabled beehive weight monitoring system using ESP32 and dual HX711 load cell sensors.
Weight data is served on a built-in web dashboard and optionally streamed to an MQTT broker for Node-RED visualisation.

[![MIT License][license-shield]][license-url]
![Status](https://img.shields.io/badge/Status-Active-green?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C++-blue?style=for-the-badge)
![Hardware](https://img.shields.io/badge/Hardware-ESP32-FF7139?style=for-the-badge&logo=espressif&logoColor=white)

</div>

## Overview

The ESP32 reads two HX711 load cell sensors (left + right), displays live weights on an SSD1306 OLED,
and serves a built-in web dashboard over WiFi. Optionally it publishes to an MQTT broker for
a Docker-based Node-RED dashboard on your PC.

### Use cases
- Smart beehive weight tracking
- Dual-sensor IoT scales
- Agricultural monitoring
- Laboratory weighing

---

## Key Features

- **Dual-Sensor Support** — Two independent HX711 load cells with per-sensor calibration
- **Web Dashboard** — Built-in HTTP dashboard with live charts (Chart.js served from LittleFS, gzipped)
- **OLED Display** — SSD1306 auto-cycles Total (5 s) → Sensor 1 (2 s) → Sensor 2 (2 s)
- **Auto-Logging** — Startup + hourly weight records, RAM-cached for fast reads, persisted to LittleFS
- **MQTT Publishing** — Optional live weight stream to a broker at configurable intervals
- **Node-RED Dashboard** — Docker stack with Mosquitto + Node-RED for real-time gauges and charts
- **Sensor Calibration** — Per-sensor tare and absolute zero-point via web endpoints
- **NTP Time Sync** — Automatic time via NTP on WiFi connect; browser `/sync` as fallback
- **Wake-up Schedule** — Set daily wake-up times via web UI, persisted in NVS flash
- **Scheduled Deep Sleep** — Auto-sleep after 10 min, timer wake at scheduled times + button override
- **Simulation Mode** — Random-fluctuation test mode without physical hardware
- **Modular Storage** — Split into LittleFS records, NVS calibration, and init modules

---

## Hardware Requirements

- ESP32 Development Board
- 2 × HX711 24-bit ADC modules
- 2 × load cells (5 kg – 50 kg)
- *(Optional)* SSD1306 0.96" OLED display (I2C)

### Pin Configuration

| Component | Signal | GPIO | Notes |
|-----------|--------|------|-------|
| HX711 #1 (left) | DT | 21 | |
| HX711 #1 (left) | SCK | 22 | |
| HX711 #2 (right) | DT | 18 | |
| HX711 #2 (right) | SCK | 19 | |
| SSD1306 OLED | SDA | **4** | Custom I2C — GPIO 21 is taken by HX711 |
| SSD1306 OLED | SCL | **5** | Custom I2C — GPIO 22 is taken by HX711 |
| SSD1306 OLED | VCC | **15** | GPIO used as power supply — no free 3.3V/5V pin needed |
| SSD1306 OLED | GND | GND | |
| *(future)* Wake button | Signal | **32** | Reserved for deep-sleep wake-up (`DEEP_SLEEP_ENABLED`) |
| *(future)* Wake button | GND | **33** | Reserved — will be set OUTPUT LOW as button GND |

> HX711 only needs **DT** and **SCK** — there is no ACC/CS pin.
>
> **OLED power:** The OLED VCC is powered from **GPIO 15**, set HIGH in firmware.
> The SSD1306 draws ~20 mA, which is within the ESP32 GPIO 40 mA limit.
> This avoids using the 3.3 V and 5 V rails, which are already occupied by the two HX711 modules.
>
> **OLED display** auto-cycles through views automatically — no button required:
> Total (5 s) → Sensor 1 (2 s) → Sensor 2 (2 s) → repeat.
> Timing is adjustable via `OLED_TOTAL_SHOW_MS` / `OLED_SENSOR_SHOW_MS` in `config.h`.

---

## Software Stack

| Layer | Technology |
|-------|-----------|
| Firmware | PlatformIO + Arduino (ESP32) |
| Sensor library | bogde/HX711 |
| MQTT client | knolleary/PubSubClient |
| JSON | bblanchon/ArduinoJson 7 |
| Web server | ESP32 WebServer (port 80) |
| Charts | Chart.js (served gzipped from LittleFS) |
| MQTT broker | eclipse-mosquitto 2 (Docker) |
| Dashboard | Node-RED + node-red-dashboard (Docker) |

---

## Getting Started

### 1. Configure the firmware

Edit [`include/config.h`](include/config.h):

#### Feature Switches

Enable or disable each subsystem at the top of `config.h`:

```cpp
#define WIFI_ENABLED        true   // WiFi STA + AP dual mode
#define WEB_SERVER_ENABLED  true   // HTTP web UI on port 80
#define MQTT_ENABLED        false  // Publish weight to MQTT broker
#define AUTO_LOGGER_ENABLED true   // Startup + hourly LittleFS logging
#define OLED_ENABLED        true   // SSD1306 OLED display (auto-cycles Total/S1/S2)
#define NTP_ENABLED         true   // NTP time sync on WiFi connect (UTC+8)
#define SCHEDULE_ENABLED    true   // Wake-up schedule management (NVS + web UI)
#define DEEP_SLEEP_ENABLED  false  // Scheduled deep sleep + timer/button wake-up
#define SIMULATE_SENSOR     false  // Random-fluctuation fake sensor data
```

> **Note:** `WEB_SERVER_ENABLED` and `MQTT_ENABLED` both require `WIFI_ENABLED true`.
> `NTP_ENABLED` requires `WIFI_ENABLED true` (needs STA connection for NTP servers).
> `DEEP_SLEEP_ENABLED` uses `SCHEDULE_ENABLED` for timer wake-up and `NTP_ENABLED` for time awareness.
> `OLED_ENABLED` requires an SSD1306 OLED wired to GPIO 4 (SDA) and GPIO 5 (SCL). See [Pin Configuration](#pin-configuration).

#### Credentials & addresses

```cpp
// WiFi – ESP32 will join this network
const char *const STA_WIFI_SSID = "YOUR_AP_PASS";
const char *const STA_WIFI_PASS = "YOUR_AP_PASS";

// MQTT broker (only needed if MQTT_ENABLED true)
const char *const MQTT_BROKER_IP = "YOUR_AP_PASS";
const int         MQTT_BROKER_PORT = 1884;

// Per-sensor calibration
const float LOADCELL1_SCALE_FACTOR = 85000.0;
const float LOADCELL2_SCALE_FACTOR = 85000.0;
```

### 2. Upload the LittleFS filesystem image

Chart.js is served from LittleFS as a gzipped file. Upload it before first use:

```bash
platformio run --target uploadfs --upload-port /dev/ttyUSB0
```

### 3. Build and upload firmware

```bash
platformio run --target upload --upload-port /dev/ttyUSB0
```

### 4. Monitor serial output

```bash
platformio device monitor --baud 115200
```

After boot you will see lines like:

```
[WiFi] Connected! IP: 192.168.1.xxx
[WiFi] Web interface: http://192.168.1.xxx
[Storage] Records cache loaded (128 bytes)
[System] System initialization complete, dual-core mode active
```

---

## ESP32 Web Interface

After boot, open `http://<ESP32-IP>` in a browser.

| Panel | Content |
|-------|---------|
| Sensor 1 | Live reading + tare/calibration controls |
| Sensor 2 | Live reading + tare/calibration controls |
| Total | Combined weight + chart, stored records, record management |
| Wake-up Schedule | Add/remove daily wake-up times for deep sleep cycle |

### REST API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/` | Main dashboard (HTML) |
| GET | `/chartjs` | Chart.js library (gzipped, cached 24 h) |
| GET | `/data` | Total weight (S1 + S2) in kg |
| GET | `/data1` | Sensor 1 weight in kg |
| GET | `/data2` | Sensor 2 weight in kg |
| GET | `/get-records` | All stored records (JSON) |
| GET | `/add-record?t=TIME&w=WEIGHT` | Save a record |
| GET | `/del-record?i=INDEX` | Delete record by index |
| GET | `/clear-records` | Clear all records |
| GET | `/tare` | Tare both sensors |
| GET | `/tare1` | Tare Sensor 1 |
| GET | `/tare2` | Tare Sensor 2 |
| GET | `/set-zero1` | Absolute zero calibration — Sensor 1 |
| GET | `/set-zero2` | Absolute zero calibration — Sensor 2 |
| GET | `/sync?t=EPOCH` | Sync ESP32 clock from browser |
| GET | `/time` | Current ESP32 time (HH:MM:SS or "Not synced") |
| GET | `/get-schedule` | Wake-up schedule (JSON array) |
| GET | `/add-schedule?h=HOUR&m=MIN` | Add a wake-up time |
| GET | `/del-schedule?i=INDEX` | Remove a wake-up time |

---

## PC Dashboard (Docker)

### What you need

- Docker and Docker Compose installed on your PC
- PC fixed IP: `YOUR_AP_PASS` on the same network as the ESP32
- `MQTT_ENABLED true` in `config.h`

### Start the stack

```bash
docker compose -f docker/docker-compose.mqtt.yml up -d
```

This starts two containers:

| Container | Purpose | Port |
|-----------|---------|------|
| `esp32-mosquitto` | MQTT broker | `1884` |
| `esp32-nodered` | Node-RED dashboard | `1880` |

### Open the dashboard

Navigate to **http://localhost:1880/ui** in your browser.

You will see:
- **Sensor 1 (Left)** gauge — live weight from the left load cell
- **Sensor 2 (Right)** gauge — live weight from the right load cell
- **Total (Beehive)** gauge + numeric readout — combined weight
- **History Chart** — scrolling 6-hour line graph of total weight

### MQTT topics published by the ESP32

| Topic | Value | Example |
|-------|-------|---------|
| `weight-scale/sensor1` | Left sensor (kg) | `12.345` |
| `weight-scale/sensor2` | Right sensor (kg) | `11.890` |
| `weight-scale/total` | S1 + S2 (kg) | `24.235` |

### Stop the stack

```bash
docker compose -f docker/docker-compose.mqtt.yml down
```

---

## Project Structure

```
├── include/
│   ├── config.h              # Pins, WiFi, MQTT, calibration constants
│   ├── sensor_manager.h      # Dual HX711 abstraction
│   ├── wifi_manager.h        # STA/AP WiFi setup
│   ├── mqtt_manager.h        # MQTT publish manager
│   ├── oled_manager.h        # SSD1306 OLED display driver
│   ├── auto_logger.h         # Hourly weight logger
│   ├── deep_sleep_manager.h  # Scheduled deep sleep + wake-up
│   ├── schedule_manager.h    # Wake-up schedule CRUD (NVS-backed)
│   ├── web_server_logic.h    # HTTP REST endpoints
│   ├── web_pages.h           # Embedded HTML/CSS/JS
│   └── storage/
│       ├── storage_init.h        # LittleFS mount + cache init
│       ├── littlefs_storage.h    # Records CRUD (RAM-cached)
│       └── nvs_storage.h         # NVS calibration offsets
├── src/
│   ├── main.cpp              # Entry point, dual-core task setup
│   ├── sensor_manager.cpp    # HX711 driver (dual sensor)
│   ├── wifi_manager.cpp      # STA connect + AP fallback
│   ├── mqtt_manager.cpp      # PubSubClient publish loop
│   ├── oled_manager.cpp      # OLED auto-cycle display
│   ├── auto_logger.cpp       # Startup + hourly auto-record
│   ├── deep_sleep_manager.cpp # Scheduled deep sleep + timer/button wake
│   ├── schedule_manager.cpp  # NVS-backed schedule storage
│   ├── web_server_logic.cpp  # REST API routes
│   ├── web_pages.cpp         # Web UI assets
│   └── storage/
│       ├── storage_init.cpp      # Mount LittleFS, load cache
│       ├── littlefs_storage.cpp  # Records: RAM cache + LittleFS persist
│       └── nvs_storage.cpp       # Preferences API offsets
├── data/
│   └── chart.min.js.gz       # Chart.js (gzipped, uploaded to LittleFS)
├── docker/
│   ├── docker-compose.mqtt.yml   # Mosquitto + Node-RED stack
│   └── mosquitto/config/
│       └── mosquitto.conf        # Broker configuration
├── nodered_data/             # Node-RED project (flows + dashboard)
├── .github/workflows/        # CI checks
└── platformio.ini            # Build configuration
```

---

## Architecture

### Dual-Core Design

| Core | Task | Details |
|------|------|---------|
| Core 1 (`loop()`) | Sensor polling + OLED | `updateSensor()` + `handleOLED()`, 1 ms yield |
| Core 0 (FreeRTOS task) | Web server + MQTT + auto-logger | 10 ms tick via `vTaskDelay` |

### Storage Architecture

| Module | Backend | Purpose |
|--------|---------|---------|
| `littlefs_storage` | LittleFS + RAM cache | Weight records — reads from RAM, writes persist to flash |
| `nvs_storage` | Preferences (NVS) | Absolute zero calibration offsets |
| `storage_init` | — | Mounts LittleFS, primes the RAM cache at boot |

### WiFi + MQTT Connection Flow

1. `initWiFi()` attempts STA connection (15 s timeout)
2. On success: STA IP printed to serial; web server and MQTT become available
3. On failure: falls back to AP mode (`ESP32_Weight_Scale`) — web UI works, MQTT unavailable
4. `handleMQTT()` auto-reconnects to the broker with 5 s back-off
5. Weight values are published every `MQTT_PUBLISH_INTERVAL_MS` (default 5 s)

---

## Configuration Reference

### WiFi (STA + AP fallback)

```cpp
const char *const STA_WIFI_SSID = "YOUR_AP_PASS";           // Primary network
const char *const STA_WIFI_PASS = "YOUR_AP_PASS";
const char *const AP_WIFI_SSID  = "ESP32_Weight_Scale";  // Fallback AP
const char *const AP_WIFI_PASS  = "YOUR_AP_PASS";
```

### MQTT

```cpp
const char *const MQTT_BROKER_IP             = "YOUR_AP_PASS";
const int         MQTT_BROKER_PORT           = 1884;
const char *const MQTT_CLIENT_ID             = "esp32-weight-scale";
const char *const MQTT_TOPIC_SENSOR1         = "weight-scale/sensor1";
const char *const MQTT_TOPIC_SENSOR2         = "weight-scale/sensor2";
const char *const MQTT_TOPIC_TOTAL           = "weight-scale/total";
const unsigned long MQTT_PUBLISH_INTERVAL_MS = 5000; // 5 seconds
```

### Sensor Calibration

```cpp
const float LOADCELL1_SCALE_FACTOR = 85000.0; // Left load cell
const float LOADCELL2_SCALE_FACTOR = 85000.0; // Right load cell
```

Calibration procedure:
1. Place a known weight on the load cell
2. Read the raw ADC value from serial output
3. `scale_factor = raw_reading / known_weight_in_kg`

### Auto-Logger

```cpp
const unsigned long STARTUP_RECORD_DELAY_MS = 10000;   // Wait 10 s after boot
const unsigned long AUTO_RECORD_INTERVAL_MS = 3600000; // Hourly
const int           MAX_RECORDS             = 10;      // Rolling buffer
```

### NTP Time Sync

```cpp
#define NTP_ENABLED              true
const char *const NTP_SERVER1            = "pool.ntp.org";
const char *const NTP_SERVER2            = "time.nist.gov";
const long        NTP_GMT_OFFSET_SEC     = 28800;   // UTC+8 (Taiwan)
const int         NTP_DAYLIGHT_OFFSET_SEC = 0;
```

### Wake-up Schedule

```cpp
#define SCHEDULE_ENABLED    true
const int MAX_SCHEDULE_ENTRIES = 10;   // Max daily wake-up times
```

Schedule entries are stored in NVS (namespace `schedule`, key `times`) as comma-separated `HH:MM` strings.
Managed via the web UI's "Wake-up Schedule" panel or REST endpoints.

### Deep Sleep

```cpp
#define DEEP_SLEEP_ENABLED  false                     // Enable scheduled sleep/wake
const unsigned long AWAKE_DURATION_MS = 600000;       // Stay awake 10 min after boot
const int WAKE_BTN_PIN = 32;                          // Button wake-up (ext0, active LOW)
const int WAKE_BTN_GND = 33;                          // GPIO used as button GND
```

**Sleep/wake cycle:** Boot → WiFi → NTP sync → log data → serve web UI for 10 min → calculate next scheduled wake time → deep sleep → timer fires → reboot.

If no schedule is set or time is not synced, only button wake-up (GPIO 32) is active.

### Simulation Mode

```cpp
#define SIMULATE_SENSOR true   // Set false for production
```

Simulated sensors fluctuate around realistic base weights (S1 ~25 kg, S2 ~22 kg) with ±0.5 kg random noise.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No weight reading | Wrong pin or scale factor | Check `config.h` pin/calibration |
| WiFi not connecting | Wrong SSID/password | Update `STA_WIFI_SSID` / `STA_WIFI_PASS` |
| MQTT failed (rc=-2) | Broker not running | `docker compose -f docker/docker-compose.mqtt.yml up -d` |
| Dashboard empty | Node-RED not connected to broker | Check that `mosquitto` container is running |
| Chart not loading | LittleFS not uploaded | Run `platformio run --target uploadfs` |
| Storage full | Too many records | Clear via `/clear-records` endpoint |
| Sensor drift | Temperature change | Recalibrate scale factor |
| Slow page load | Chart.js missing from LittleFS | Upload filesystem image (see step 2) |

---

## Development

```bash
# Build
platformio run

# Upload firmware
platformio run --target upload --upload-port /dev/ttyUSB0

# Upload LittleFS filesystem (Chart.js)
platformio run --target uploadfs --upload-port /dev/ttyUSB0

# Clean build
platformio run --target clean

# Serial monitor
platformio device monitor --baud 115200
```

---

<div align="center">

**[Back to top](#esp32-iot-weight-scale)**

</div>

[license-shield]: https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge
[license-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/blob/main/LICENSE
