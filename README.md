<div align="center">

# ESP32 IoT Weight Scale

A WiFi-enabled beehive weight monitoring system using ESP32 and dual HX711 load cell sensors.
Weight data is streamed live to an MQTT broker and visualised in a Node-RED dashboard on your PC.

[![MIT License][license-shield]][license-url]
![Status](https://img.shields.io/badge/Status-Active-green?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C++-blue?style=for-the-badge)
![Hardware](https://img.shields.io/badge/Hardware-ESP32-FF7139?style=for-the-badge&logo=espressif&logoColor=white)

</div>

## Overview

The ESP32 connects to your home/lab WiFi network, reads two HX711 load cell sensors (left + right),
and publishes the individual and combined weight values to an MQTT broker every 5 seconds.
A Docker-based stack (Mosquitto + Node-RED) runs on your PC to receive and visualise the data in real time.

### Use cases
- Smart beehive weight tracking
- Dual-sensor IoT scales
- Agricultural monitoring
- Laboratory weighing

---

## Key Features

- **Dual-Sensor Support**: Two independent HX711 load cells with per-sensor calibration
- **MQTT Publishing**: Live weight streamed to a broker at 5-second intervals
- **Node-RED Dashboard**: Real-time gauges and history chart at `http://localhost:1880/ui`
- **Web Interface**: Built-in HTTP dashboard at the ESP32's IP address
- **Auto-Logging**: Hourly weight records stored on LittleFS
- **Sensor Calibration**: Per-sensor tare and absolute zero-point management
- **Simulation Mode**: Full test coverage without physical hardware

---

## Hardware Requirements

- ESP32 Development Board
- 2 × HX711 24-bit ADC modules
- 2 × load cells (5 kg – 50 kg)

### Pin Configuration

| Component | Signal | GPIO |
|-----------|--------|------|
| HX711 #1 (left) | DT | 21 |
| HX711 #1 (left) | SCK | 22 |
| HX711 #2 (right) | DT | 18 |
| HX711 #2 (right) | SCK | 19 |

> HX711 only needs **DT** and **SCK** — there is no ACC/CS pin.
> Power each module from an external 3.3 V / 5 V rail if the ESP32 3V3 pin cannot supply enough current for both.

---

## Software Stack

| Layer | Technology |
|-------|-----------|
| Firmware | PlatformIO + Arduino (ESP32) |
| Sensor library | bogde/HX711 |
| MQTT client | knolleary/PubSubClient |
| JSON | bblanchon/ArduinoJson 7 |
| Web server | ESP32 WebServer (port 80) |
| MQTT broker | eclipse-mosquitto 2 (Docker) |
| Dashboard | Node-RED + node-red-dashboard (Docker) |

---

## Getting Started

### 1. Configure the firmware

Edit [`include/config.h`](include/config.h):

#### Feature Switches

Enable or disable each subsystem at the top of `config.h`:

```cpp
#define WIFI_ENABLED        true   // WiFi STA + AP fallback
#define WEB_SERVER_ENABLED  true   // HTTP web UI on port 80
#define MQTT_ENABLED        true   // Publish weight to MQTT broker
#define AUTO_LOGGER_ENABLED true   // Hourly LittleFS logging
#define SIMULATE_SENSOR     false  // Use fake sensor data (no hardware needed)
```

> **Note:** `WEB_SERVER_ENABLED` and `MQTT_ENABLED` both require `WIFI_ENABLED true`.

#### Credentials & addresses

```cpp
// WiFi – ESP32 will join this network to reach the MQTT broker
const char *const STA_WIFI_SSID = "NOL_WIFI";
const char *const STA_WIFI_PASS = "***REMOVED***";

// MQTT broker – fixed IP of your PC on the same network
const char *const MQTT_BROKER_IP = "***REMOVED***";

// Per-sensor calibration
const float LOADCELL1_SCALE_FACTOR = 85000.0;
const float LOADCELL2_SCALE_FACTOR = 85000.0;
```

### 2. Build and upload

```bash
# Install PlatformIO if needed: https://platformio.org
platformio run --target upload --upload-port /dev/ttyUSB0
```

### 3. Monitor serial output

```bash
platformio device monitor --baud 115200
```

After boot you will see lines like:

```
[WiFi] Connected! IP: 192.168.1.xxx
[WiFi] Web interface: http://192.168.1.xxx
[WiFi] MQTT broker:   ***REMOVED***:1883
[MQTT] Configured broker: ***REMOVED***:1883
[MQTT] Published -> S1=12.345 kg  S2=11.890 kg  Total=24.235 kg
```

---

## PC Dashboard (Docker)

### What you need

- Docker and Docker Compose installed on your PC
- PC fixed IP: `***REMOVED***` on the `NOL_WIFI` network

### Start the stack

```bash
docker compose -f docker/docker-compose.mqtt.yml up -d
```

This starts two containers:

| Container | Purpose | Port |
|-----------|---------|------|
| `esp32-mosquitto` | MQTT broker | `1883` |
| `esp32-nodered` | Node-RED dashboard | `1880` |

### Open the dashboard

Navigate to **http://localhost:1880/ui** in your browser.

You will see:
- **Sensor 1 (Left)** gauge – live weight from the left load cell
- **Sensor 2 (Right)** gauge – live weight from the right load cell
- **Total (Beehive)** gauge + numeric readout – combined weight
- **History Chart** – scrolling 6-hour line graph of total weight

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

> **Why is the Docker Compose file in `docker/`?**
> PlatformIO only compiles files under `src/`, `include/`, `lib/`, and `test/`.
> Anything in `docker/` is completely ignored by the ESP32 build system.

---

## Project Structure

```
├── include/
│   ├── config.h              # Pins, WiFi, MQTT, calibration constants
│   ├── sensor_manager.h      # Dual HX711 abstraction
│   ├── wifi_manager.h        # STA/AP WiFi setup
│   ├── mqtt_manager.h        # MQTT publish manager
│   ├── storage_manager.h     # LittleFS records
│   ├── web_server_logic.h    # HTTP REST endpoints
│   ├── auto_logger.h         # Hourly weight logger
│   └── web_pages.h           # Embedded HTML/CSS/JS
├── src/
│   ├── main.cpp              # Entry point, dual-core task setup
│   ├── sensor_manager.cpp    # HX711 driver (dual sensor)
│   ├── wifi_manager.cpp      # STA connect + AP fallback
│   ├── mqtt_manager.cpp      # PubSubClient publish loop
│   ├── storage_manager.cpp   # LittleFS + Preferences
│   ├── web_server_logic.cpp  # REST API
│   ├── auto_logger.cpp       # Hourly auto-record
│   └── web_pages.cpp         # Web UI assets
├── docker/
│   ├── docker-compose.mqtt.yml   # Mosquitto + Node-RED stack
│   └── mosquitto/
│       └── config/
│           └── mosquitto.conf    # Broker configuration
├── nodered_data/             # Node-RED project (flows + dashboard)
└── platformio.ini            # Build configuration
```

---

## ESP32 Web Interface

The ESP32 also serves a built-in web dashboard.
After boot, find the IP in the serial output and open `http://<ESP32-IP>` in a browser.

| Panel | Content |
|-------|---------|
| Sensor 1 | Live reading + chart + tare/calibration controls |
| Sensor 2 | Live reading + chart + tare/calibration controls |
| Total | Combined weight, stored records, record management |

### REST API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
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
| GET | `/set-zero1` | Absolute zero calibration – Sensor 1 |
| GET | `/set-zero2` | Absolute zero calibration – Sensor 2 |

---

## Configuration Reference

### WiFi (STA + AP fallback)

```cpp
// include/config.h
const char *const STA_WIFI_SSID = "NOL_WIFI";          // Primary network
const char *const STA_WIFI_PASS = "***REMOVED***";
const char *const AP_WIFI_SSID  = "ESP32_Weight_Scale"; // Fallback AP
const char *const AP_WIFI_PASS  = "***REMOVED***";
```

### MQTT

```cpp
const char *const MQTT_BROKER_IP          = "***REMOVED***";
const int         MQTT_BROKER_PORT        = 1883;
const char *const MQTT_CLIENT_ID          = "esp32-weight-scale";
const char *const MQTT_TOPIC_SENSOR1      = "weight-scale/sensor1";
const char *const MQTT_TOPIC_SENSOR2      = "weight-scale/sensor2";
const char *const MQTT_TOPIC_TOTAL        = "weight-scale/total";
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

### Simulation Mode

```cpp
#define SIMULATE_SENSOR true   // Set false for production
```

---

## Architecture

### Dual-Core Design

| Core | Task |
|------|------|
| Core 1 (loop) | Sensor polling – `updateSensor()` every 500 ms |
| Core 0 (task) | HTTP server + MQTT publish + auto-logger |

### MQTT Connection Flow

1. `initWiFi()` attempts STA connection to `NOL_WIFI` (15 s timeout)
2. On success: STA IP printed to serial; MQTT becomes available
3. On failure: falls back to AP mode (MQTT unavailable, web UI still works)
4. `handleMQTT()` auto-reconnects to the broker with 5 s back-off
5. Weight values are published every `MQTT_PUBLISH_INTERVAL_MS` (default 5 s)

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---------|-------------|-----|
| No weight reading | Wrong pin or scale factor | Check `config.h` pin/calibration |
| WiFi not connecting | Wrong SSID/password | Update `STA_WIFI_SSID` / `STA_WIFI_PASS` |
| MQTT failed (rc=-2) | Broker not running | `docker compose -f docker/docker-compose.mqtt.yml up -d` |
| Dashboard empty | Node-RED not connected to broker | Check that `mosquitto` container is running |
| Storage full | Too many records | Clear via `/clear-records` endpoint |
| Sensor drift | Temperature change | Recalibrate scale factor |

---

## Development

```bash
# Build
platformio run

# Upload
platformio run --target upload --upload-port /dev/ttyUSB0

# Clean build
platformio run --target clean

# Serial monitor
platformio device monitor --baud 115200
```

---

<div align="center">

**[⬆ Back to top](#esp32-iot-weight-scale)**

</div>

[license-shield]: https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge
[license-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/blob/main/LICENSE
