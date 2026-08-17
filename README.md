# ESP32 Flight Tracker — 64×64 HUB75

An ESP32/Arduino prototype for displaying nearby FlightRadar24 aircraft on a 64×64 HUB75 RGB LED matrix.

> **Prototype status:** This is a new ESP32 firmware project. It is not a drop-in replacement for the existing Raspberry Pi/Python FlightTracker application.

## Features

- 64×64 HUB75 display through ESP32 I2S/DMA
- Wi-Fi and NTP time synchronization
- Nearby aircraft polling from a bounded FlightRadar24 feed request
- Callsign, ICAO aircraft type, altitude, speed, heading, and distance
- Configurable altitude filters with disable-by-zero thresholds
- Configurable flight-screen rotation interval and unit preferences (`km/mi`, `ft/m`, `kt/kmh/mph`)
- Configurable idle clock format (24-hour or 12-hour)
- Compact airline colour/initial badges stored in program flash
- LittleFS JSON configuration
- First-boot Wi-Fi + location provisioning portal
- Idle clock when no aircraft match the configured radius

## Important notes

- FlightRadar24's feed is an external undocumented service. Endpoints, policies, response data, and rate limits may change. Use the conservative default 60-second polling interval and comply with its terms.
- The ESP32 cannot reuse the Raspberry Pi RGB Matrix Bonnet. Connect it directly to the HUB75 panel using suitable 3.3 V-to-5 V logic level shifters.
- A normal ESP32 has constrained RAM and flash. The initial firmware uses compact airline badges rather than embedding a large third-party bitmap-logo catalogue.

## Hardware

- ESP32 DevKit, or preferably an ESP32-S3 with PSRAM
- One 64×64 HUB75 RGB LED matrix
- Regulated 5 V / 5 A or larger power supply for the panel
- 74AHCT245 or 74HCT245 level shifter(s)
- Shared ground between ESP32, panel supply, and level shifters

Read [docs/WIRING.md](docs/WIRING.md) before applying power.

## Quick start

```bash
git clone https://github.com/KK-ThBer/esp32-flight-tracker-64x64.git
cd esp32-flight-tracker-64x64
python3 -m pip install platformio
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

> **Production / release builds:** Use the `esp32-dev-release` or `esp32-s3-release`
> environments when flashing a device for deployment. These disable verbose serial
> logging (`-DCORE_DEBUG_LEVEL=0`), which saves additional flash space on top of the
> size-optimisation flags (`-Os`) that apply to all environments.
>
> ```bash
> pio run -e esp32-dev-release -t upload   # ESP32
> pio run -e esp32-s3-release -t upload    # ESP32-S3
> ```

On first boot (or after recovery trigger), the device starts a temporary
`FlightTracker-Setup-*` access point and captive portal so you can enter Wi-Fi,
location, radius, brightness, and rotation from a phone or browser.

## Display layout

```text
┌────────────────────────────────────────────────┐
│ badge   BAW123                            1/3   │
│         A320                                   │
├────────────────────────────────────────────────┤
│ A10000ft             S450kt                     │
│ H270                 12.4km                     │
├────────────────────────────────────────────────┤
│ EGLL > KJFK                                    │
└────────────────────────────────────────────────┘
```

## Web flasher

Flash firmware directly from your browser (Chrome/Edge) using a USB-connected ESP32.
No software installation required — the binary is downloaded from a GitHub Release
and written locally through the Web Serial API.

- **Installer page (placeholder — activate GitHub Pages first):**
  `https://kk-thber.github.io/esp32-flight-tracker-64x64/web-installer/`
  See [docs/WEB_FLASHER.md](docs/WEB_FLASHER.md) for activation steps and full documentation.

> ⚠ The web flasher only works while the ESP32 is physically connected to
> **your** computer by USB. GitHub-hosted runners are remote and have no USB access.

> ⚠ If this repository is **private**, GitHub Pages and Release download URLs will
> not be publicly accessible. Make the repository public or configure an alternative
> before promoting the installer URL.

## Documentation

- [Web Flasher](docs/WEB_FLASHER.md)
- [Wiring](docs/WIRING.md)
- [Configuration](docs/CONFIG.md)
- [Setup](docs/SETUP.md)
- [Architecture](docs/ARCHITECTURE.md)
- [Troubleshooting](docs/TROUBLESHOOTING.md)
- [Feature parity roadmap](docs/FEATURE_PARITY.md)

## License

GPL-3.0-or-later. See [LICENSE](LICENSE) and [NOTICE.md](NOTICE.md).
