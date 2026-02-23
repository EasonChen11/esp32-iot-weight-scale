<div align="center">

# ESP32 IoT Weight Scale

A feature-rich WiFi-enabled weight measurement system using ESP32 and HX711 load cell sensor with web-based monitoring and data logging.

<!-- [![Forks][forks-shield]][forks-url] -->
<!-- [![Stargazers][stars-shield]][stars-url] -->
<!-- [![Issues][issues-shield]][issues-url] -->
[![MIT License][license-shield]][license-url]
![Status](https://img.shields.io/badge/Status-Active-green?style=for-the-badge)
![Language](https://img.shields.io/badge/Language-C++-blue?style=for-the-badge)
![Hardware](https://img.shields.io/badge/Hardware-ESP32-FF7139?style=for-the-badge&logo=espressif&logoColor=white)

</div>

## Overview

This project transforms an ESP32 microcontroller into an intelligent weight measurement system with WiFi connectivity. It features real-time weight monitoring through a web interface, persistent data storage, and precise sensor calibration with absolute zero-point management.

Perfect for applications such as:
- Smart scales and weighing stations
- IoT monitoring systems
- Inventory management
- Laboratory measurements
- Agricultural applications

## Key Features

- **Real-Time Weight Monitoring**: Continuous sensor updates at 500ms intervals
- **WiFi Connectivity**: Built-in WiFi access point for remote monitoring
- **Web Interface**: Responsive web dashboard for live data visualization
- **Data Logging**: Persistent storage of weight measurements with timestamps
- **Sensor Calibration**: Absolute zero-point management and tare functionality
- **Simulation Mode**: Test without hardware using simulated sensor data
- **SPIFFS Storage**: Efficient on-device data persistence
- **Hardware Abstraction**: Clean modular architecture for sensor management

## Hardware Requirements

- **Microcontroller**: ESP32 Development Board
- **Load Cell Sensor**: HX711 24-bit Analog-to-Digital Converter
- **Components**: Load cell (typically 5kg-50kg capacity)

### Pin Configuration

| Component | GPIO Pin |
|-----------|----------|
| HX711 DT (Data Out) | GPIO 21 |
| HX711 SCK (Serial Clock) | GPIO 22 |

## Software Stack

- **Framework**: PlatformIO + Arduino
- **Sensor Library**: HX711 by bogde
- **JSON Support**: ArduinoJson 7.0+
- **Web Server**: Built-in ESP32 WebServer
- **Language**: C++ (Arduino style)

## Getting Started

### Prerequisites

- PlatformIO installed ([platformio.org](https://platformio.org))
- USB-to-Serial adapter for ESP32 flashing
- Serial monitor at 115200 baud rate

### Installation

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd esp32-weight-scale
   ```

2. **Configure hardware settings**

   Edit `include/config.h` to match your setup:
   ```cpp
   const char *const WIFI_SSID = "Your_Network_SSID";
   const char *const WIFI_PASS = "Your_Network_Password";
   
   // GPIO pins for your hardware
   const int LOADCELL_DOUT_PIN = 21;
   const int LOADCELL_SCK_PIN = 22;
   ```

3. **Calibrate the sensor**

   Update the scale factor in `config.h`:
   ```cpp
   const float LOADCELL_SCALE_FACTOR = 85000.0;
   ```
   
   To calibrate:
   - Place a known weight on the load cell
   - Record the raw ADC reading
   - Calculate: `scale_factor = raw_reading / known_weight`

4. **Build and Upload**
   ```bash
   platformio run --target upload --upload-port /dev/ttyUSB0
   ```

5. **Monitor Serial Output**
   ```bash
   platformio device monitor --baud 115200
   ```

### Testing Without Hardware

Enable simulated sensor mode in `config.h`:
```cpp
#define SIMULATE_SENSOR true
```

This generates realistic weight data for testing the full application logic without physical hardware.

## Architecture

### Project Structure

```
├── include/
│   ├── config.h              # Configuration and hardware pins
│   ├── sensor_manager.h      # Weight sensor abstraction
│   ├── wifi_manager.h        # WiFi connectivity
│   ├── storage_manager.h     # Data persistence layer
│   ├── web_server_logic.h    # HTTP endpoints
│   └── web_pages.h           # HTML/CSS/JS assets
├── src/
│   ├── main.cpp              # Application entry point
│   ├── sensor_manager.cpp    # HX711 driver implementation
│   ├── wifi_manager.cpp      # AP/STA mode setup
│   ├── storage_manager.cpp   # SPIFFS file operations
│   ├── web_server_logic.cpp  # REST API endpoints
│   └── web_pages.cpp         # Static web assets
└── platformio.ini            # Build configuration
```

### Core Modules

#### Sensor Manager
Handles HX711 load cell communication with:
- Real-time weight caching (500ms update rate)
- Tare (zero adjustment) functionality
- Absolute zero-point offset management
- Graceful simulator fallback

#### WiFi Manager
Establishes WiFi connectivity via:
- Access Point (AP) mode by default
- Configurable SSID and password
- Automatic reconnection

#### Storage Manager
Manages persistent data with:
- JSON-formatted measurement records stored in SPIFFS
- Timestamp integration for each measurement
- Calibration offset storage using Preferences API
- Efficient key-value storage for configuration data

#### Web Server Logic
RESTful API endpoints for:
- Real-time weight queries
- Historical data retrieval
- Record management (add/delete/clear)
- Calibration offset adjustment

## Usage

### Connect to the Device

1. Scan for WiFi networks and connect to `ESP32_Weight_Scale`
2. Default password: `***REMOVED***`
3. **Open your web browser and navigate to `http://192.168.4.1`** to access the web interface

> **Important**: You must open `http://192.168.4.1` in your web browser after connecting to the WiFi network to access the weight scale dashboard.

### Web Interface

The dashboard provides:
- **Live Weight Display**: Current measurement in kg
- **Data Records Table**: Complete measurement history
- **Record Management**: Add, delete, or clear records
- **Calibration Controls**: Tare and offset adjustments

### API Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/weight` | Get current weight |
| GET | `/records` | Fetch all stored records |
| POST | `/records` | Add new weight record |
| DELETE | `/records/<id>` | Delete specific record |
| POST | `/tare` | Reset sensor to zero |
| POST | `/calibrate` | Update calibration offset |

## Configuration

### WiFi Settings
```cpp
// include/config.h
const char *const WIFI_SSID = "Your_Network_SSID";
const char *const WIFI_PASS = "Your_Network_Password";
```

### Sensor Calibration
```cpp
const float LOADCELL_SCALE_FACTOR = 85000.0;
```

### Simulation Mode (Testing)
```cpp
#define SIMULATE_SENSOR true    // Set to false for production
```

### Serial Monitor
```ini
monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time, colorize
```

## Troubleshooting

> **No Weight Reading**: Verify pin connections and scale factor calibration

> **WiFi Connection Issues**: Check SSID/password and ensure ESP32 has adequate power

> **SPIFFS Storage Full**: Use web interface or API to clear old records

> **Sensor Reading Drift**: Recalibrate scale factor after temperature changes

## Performance

- **Update Frequency**: 500ms weight cache refresh
- **Storage Capacity**: Up to 1000+ records (varies with SPIFFS partition)
- **WiFi Range**: Standard 802.11b/g/n coverage (~50m typical)
- **Accuracy**: ±5g (depends on load cell quality and calibration)

## Project Requirements

This project uses PlatformIO for build management and deploys to ESP32 devices.

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino

monitor_speed = 115200
monitor_filters = esp32_exception_decoder, time, colorize

lib_deps =
    bogde/HX711
    bblanchon/ArduinoJson @ ^7.0.0
```

## Development

### Build
```bash
platformio run
```

### Upload
```bash
platformio run --target upload --upload-port /dev/ttyUSB0
```

### Clean Build
```bash
platformio run --target clean
```

### Monitor Output
```bash
platformio device monitor --baud 115200
```

## Future Enhancements

- [ ] Multi-user authentication
- [ ] Data export (CSV/JSON)
- [ ] MQTT integration
- [ ] Mobile app
- [ ] Cloud synchronization
- [ ] Advanced analytics
- [ ] Multiple sensor support
- [ ] Over-the-air firmware updates

---

<div align="center">

**[⬆ Back to top](#esp32-iot-weight-scale)**

</div>

[forks-shield]: https://img.shields.io/github/forks/EasonChen11/esp32-iot-weight-scale.svg?style=for-the-badge
[forks-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/network/members

[stars-shield]: https://img.shields.io/github/stars/EasonChen11/esp32-iot-weight-scale.svg?style=for-the-badge
[stars-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/stargazers

[issues-shield]: https://img.shields.io/github/issues/EasonChen11/esp32-iot-weight-scale.svg?style=for-the-badge
[issues-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/issues

<!-- [license-shield]: https://img.shields.io/github/license/EasonChen11/esp32-iot-weight-scale.svg?style=for-the-badge／ -->
[license-shield]: https://img.shields.io/badge/license-MIT-blue.svg?style=for-the-badge
[license-url]: https://github.com/EasonChen11/esp32-iot-weight-scale/blob/main/LICENSE
