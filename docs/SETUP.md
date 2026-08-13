# Setup Guide

## Prerequisites

- **PlatformIO** – install the [PlatformIO IDE](https://platformio.org/platformio-ide) extension
  for VS Code, or install the CLI: `pip install platformio`
- **Python 3.8+** – required by PlatformIO and the optional `tools/build_icons.py` helper
- **ESP32 board** with USB-to-serial (CP2102 / CH340 / built-in USB)

## Clone and Configure

```bash
git clone https://github.com/KK-ThBer/esp32-flight-tracker-64x64.git
cd esp32-flight-tracker-64x64
cp data/config.example.json data/config.json
# edit data/config.json with your Wi-Fi credentials and location
```

## Select Environment

| Board | Environment flag |
|---|---|
| ESP32 DevKit (classic) | `-e esp32-dev` |
| ESP32-S3 DevKitC-1     | `-e esp32-s3`  |

## Build Firmware

```bash
pio run -e esp32-dev
```

## Upload LittleFS (config.json)

```bash
pio run -e esp32-dev --target uploadfs
```

This uploads `data/config.json` to the LittleFS partition on the chip.

## Upload Firmware

```bash
pio run -e esp32-dev --target upload
```

## Serial Monitor

```bash
pio device monitor -e esp32-dev
```

Baud rate is 115200. Log lines are prefixed with the module name, e.g. `[wifi]`, `[fr24]`.

## ESP32-S3 Notes

- The S3 board uses a **USB-OTG** port; select the correct COM/tty port.
- Some S3 boards need `upload_protocol = esptool` added to the `[env:esp32-s3]` section
  in `platformio.ini` if auto-detection fails.
- The default UART0 TX/RX pins differ from classic ESP32; consult your board's datasheet.
