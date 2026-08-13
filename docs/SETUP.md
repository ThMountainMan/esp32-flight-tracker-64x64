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
# Edit data/config.json with your Wi-Fi credentials and location
```

## Select Environment

| Board | Environment flag |
|---|---|
| ESP32 DevKit (classic) | `-e esp32-dev` |
| ESP32-S3 DevKitC-1     | `-e esp32-s3`  |

## Build Locally

```bash
pio run -e esp32-dev
```

## Upload Firmware

```bash
pio run -e esp32-dev --target upload
```

## Upload LittleFS (config.json) — Required

> **Important:** `config.json` is **not** bundled in the normal `firmware.bin`.
> It lives in the LittleFS partition and must be uploaded separately.
> Without this step the device cannot connect to Wi-Fi or FlightRadar24.

```bash
pio run -e esp32-dev --target uploadfs
```

## Serial Monitor

```bash
pio device monitor -e esp32-dev
```

Baud rate is 115200. Log lines are prefixed with the module name, e.g. `[wifi]`, `[fr24]`.

---

## Downloading Pre-Built Firmware from GitHub Actions

Every push and every pull request triggers the CI workflow, which compiles firmware for
both `esp32-dev` and `esp32-s3` and uploads the binaries as downloadable artifacts.
**GitHub-hosted runners have no USB connection; flashing must always be done locally.**

### How to download an artifact

1. Go to the repository on GitHub → **Actions** tab → click the most recent successful
   workflow run.
2. Scroll to the **Artifacts** section and click the artifact name
   (e.g. `esp32-flight-tracker-esp32-dev`) to download a ZIP.
3. Extract the ZIP to obtain `firmware.bin`.

### Flash the downloaded firmware with esptool

```bash
# Install esptool if needed
pip install esptool

# Flash firmware (replace /dev/ttyUSB0 with your serial port)
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 921600 \
  write_flash 0x10000 firmware.bin
```

For ESP32-S3 (use `--chip esp32s3` and the correct USB-OTG port):

```bash
esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 \
  write_flash 0x10000 firmware.bin
```

### Flash config.json after flashing firmware

`config.json` is stored in the LittleFS partition and is **not** included in
`firmware.bin`. You must still build and upload the filesystem from your local clone:

```bash
# Edit data/config.json with your credentials first
pio run -e esp32-dev --target uploadfs
```

## ESP32-S3 Notes

- The S3 board uses a **USB-OTG** port; select the correct COM/tty port.
- Some S3 boards need `upload_protocol = esptool` added to the `[env:esp32-s3]` section
  in `platformio.ini` if auto-detection fails.
- The default UART0 TX/RX pins differ from classic ESP32; consult your board's datasheet.
