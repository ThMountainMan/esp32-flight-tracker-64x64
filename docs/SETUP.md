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
```

Optional display and filtering fields are already present in the example file:
`display.flight_screen_seconds`, `display.distance_unit`, `display.altitude_unit`,
`display.speed_unit`, `display.clock_24_hour`, and `filters.min_altitude_ft` /
`filters.max_altitude_ft` (use `0` to disable altitude thresholds).

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

## First-Boot Provisioning (recommended)

After flashing, a fresh device starts a temporary
`FlightTracker-Setup-<chip-id>` Wi-Fi access point and captive portal when no
valid `/config.json` exists in LittleFS.

1. Connect your phone/laptop to that AP.
2. Open any web page (or `http://192.168.4.1/`) and complete the setup form.
3. The device saves `/config.json` and restarts into station mode.

The portal times out after 10 minutes. If no valid config exists it will restart
back into provisioning mode so the device stays recoverable.

### Recovery trigger

With an already configured device, hold the **BOOT** button for 5 seconds to
re-enter provisioning mode and update settings.

## Runtime Settings (after first boot)

Once connected to your Wi-Fi network the device runs a lightweight settings server
on **port 80**. Open a browser to `http://<device-ip>/` to:

- View live device status (Wi-Fi state, IP address, last FR24 refresh result, config validity).
- Modify any supported configuration field without rebuilding or re-flashing.
- Save configuration atomically and restart with the new settings.
- Perform a **factory reset** (erase `/config.json` and re-enter provisioning mode).

The device IP address is printed in the serial monitor on every successful connection:

```
[wifi] Connected: 192.168.1.42
[settings] Settings server listening on http://192.168.1.42/
```

A `GET /status` endpoint returns a JSON object that can be polled programmatically:

```json
{
  "wifi_state": "connected",
  "ip_address": "192.168.1.42",
  "last_refresh": "FR24: 7 flights",
  "config_valid": true
}
```

For full reset and recovery procedures see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

## Upload LittleFS (config.json) — Optional manual pre-seed

`config.json` is still not bundled in `firmware.bin`. If you prefer preloading
config via files instead of the portal:

```bash
cp data/config.example.json data/config.json
# Edit data/config.json first
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

### Optional: flash config.json after flashing firmware

If you don't want to use first-boot provisioning, you can upload a prepared
LittleFS image with `pio run -e esp32-dev --target uploadfs`.

## ESP32-S3 Notes

- The S3 board uses a **USB-OTG** port; select the correct COM/tty port.
- Some S3 boards need `upload_protocol = esptool` added to the `[env:esp32-s3]` section
  in `platformio.ini` if auto-detection fails.
- The default UART0 TX/RX pins differ from classic ESP32; consult your board's datasheet.
