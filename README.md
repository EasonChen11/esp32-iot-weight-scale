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
- **Web Dashboard** — built-in HTTP UI with live charts (Chart.js from LittleFS)
- **OLED Display** — SSD1306 auto-cycles Total / Sensor 1 / Sensor 2
- **Auto-Logging** — startup + hourly records, RAM-cached, persisted to LittleFS (hourly disabled in deep sleep mode)
- **MQTT + Node-RED** — optional live stream to a Docker-based dashboard
- **NTP Time Sync** — automatic via NTP; browser `/sync` as AP-mode fallback
- **Scheduled Deep Sleep** — wake at configured times or by button press
- **Simulation Mode** — random-fluctuation test mode without hardware

> For detailed architecture, data flows, and module internals see the [**Wiki**](https://github.com/EasonChen11/esp32-iot-weight-scale/wiki).

---

## Hardware

| Component | Signal | GPIO | Notes |
|-----------|--------|------|-------|
| HX711 #1 (left) | DT | 21 | |
| HX711 #1 (left) | SCK | 22 | |
| HX711 #2 (right) | DT | 18 | |
| HX711 #2 (right) | SCK | 19 | |
| SSD1306 OLED | SDA | 4 | Custom I2C (GPIO 21/22 taken by HX711) |
| SSD1306 OLED | SCL | 5 | |
| SSD1306 OLED | VCC | 15 | GPIO as power (~20 mA, within 40 mA limit) |
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
#define WIFI_ENABLED        true
#define WEB_SERVER_ENABLED  true
#define MQTT_ENABLED        false
#define AUTO_LOGGER_ENABLED true
#define OLED_ENABLED        true
#define NTP_ENABLED         true
#define SCHEDULE_ENABLED    true
#define DEEP_SLEEP_ENABLED  false
#define SIMULATE_SENSOR     false
```

Dependencies: `WEB_SERVER_ENABLED`, `MQTT_ENABLED`, and `NTP_ENABLED` all require `WIFI_ENABLED`.
`DEEP_SLEEP_ENABLED` benefits from `SCHEDULE_ENABLED` + `NTP_ENABLED` for timed wake-ups.

> For a full explanation of each switch and their interactions, see the [**Wiki — System Overview**](https://github.com/EasonChen11/esp32-iot-weight-scale/wiki).

---

## PC Dashboard (Docker)

Requires Docker, `MQTT_ENABLED true`, and PC at `YOUR_AP_PASS` on the same network.

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
