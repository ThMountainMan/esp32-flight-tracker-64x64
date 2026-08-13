# Web Flasher — ESP32 Flight Tracker

This page explains how to flash the ESP32 Flight Tracker firmware directly from a
browser using [ESP Web Tools](https://esphome.github.io/esp-web-tools/) and Web Serial.

> **Key principle:** the firmware is downloaded from a GitHub Release and written
> to an ESP32 **connected by USB to _your_ computer**. GitHub-hosted runners
> are remote virtual machines with no USB port — they cannot flash hardware.

---

## Supported browsers

| Browser | Platform | Supported |
|---------|----------|-----------|
| Google Chrome ≥ 89 | Windows / macOS / Linux | ✅ |
| Microsoft Edge ≥ 89 | Windows / macOS / Linux | ✅ |
| Firefox | any | ❌ (no Web Serial) |
| Safari | any | ❌ (no Web Serial) |
| Chrome / Edge on Android | Android | ⚠ experimental |

---

## Requirements

- A **USB data cable** (not a charge-only cable — those lack data lines).
- An **ESP32 DevKit** board (standard WROOM/WROVER, 4 MB flash).
- The board connected to your computer **before** opening the installer.
- Chrome or Edge opened on the **same machine** the ESP32 is plugged into.

---

## Step-by-step

1. Connect the ESP32 to your computer with a USB data cable.
2. Open the web installer:
   - **GitHub Pages URL:** `https://kk-thber.github.io/esp32-flight-tracker-64x64/web-installer/`
     _(placeholder — see [Activation](#activation) below)_
   - **Local fallback:** open `docs/web-installer/index.html` in Chrome/Edge directly.
3. Select the correct board target (`esp32-dev`).
4. Click **"Connect & Flash esp32-dev"**.
5. In the browser dialog, select the serial port for your ESP32.
6. Follow the on-screen prompts. The installer will erase and write all required
   partitions automatically.

---

## BOOT / reset troubleshooting

Some ESP32 DevKit variants do not auto-reset into the bootloader. If the flash fails
or the port picker times out:

1. Hold the **BOOT** (IO0) button on the ESP32.
2. Click **Connect** in the browser.
3. Release **BOOT** once the port appears in the dialog.
4. If it still fails, press **EN** (reset) once while holding BOOT, then release BOOT.

---

## Linux serial permissions

On Linux you must be in the `dialout` group to access serial ports without `sudo`:

```bash
sudo usermod -aG dialout $USER
# Log out and back in, or run: newgrp dialout
```

---

## Selecting the correct target

| Environment | Chip | Board | Notes |
|-------------|------|-------|-------|
| `esp32-dev` | ESP32 | Standard DevKit (WROOM/WROVER) | ✅ Published |
| `esp32-s3` | ESP32-S3 | S3 DevKit with PSRAM | ⚠ Not yet verified — different bootloader offset (0x0000); manifest will be added after PlatformIO output is confirmed |

Do not attempt to flash an `esp32-s3` manifest onto an `esp32-dev` board (or vice
versa) — the bootloader offsets are different and will brick the device.

---

## What is and is not flashed

### ✅ Flashed by the installer

| Binary | Flash offset | Description |
|--------|-------------|-------------|
| `bootloader.bin` | `0x1000` | ESP-IDF second-stage bootloader |
| `partitions.bin` | `0x8000` | Partition table |
| `boot_app0.bin` | `0xe000` | OTA boot helper (from espressif32 framework) |
| `firmware.bin` | `0x10000` | Application firmware |

### ❌ Not flashed

- **`data/config.json`** — Wi-Fi credentials, latitude/longitude, and other
  user-specific settings. This file is intentionally excluded from all release
  assets and must never be committed to the repository or included in a public
  firmware image.
- **LittleFS image** — the filesystem partition (`/data`) is not written by the
  web installer. The firmware now provisions config on first boot, so a prebuilt
  LittleFS image is optional.

---

## Post-flash configuration (first boot)

When no valid `config.json` exists, firmware starts a temporary
`FlightTracker-Setup-*` AP with a captive portal:

1. Connect your phone/laptop to the setup AP.
2. Open any web page (or `http://192.168.4.1/`) and submit Wi-Fi + location settings.
3. The device writes `/config.json` to LittleFS and reboots into station mode.

The AP automatically times out after 10 minutes. If no valid config exists, the
device restarts back into provisioning mode.

If you prefer pre-seeding manually, upload LittleFS with PlatformIO:

```bash
cp data/config.example.json data/config.json
# Edit data/config.json with your credentials and location first
pio run -e esp32-dev -t uploadfs
```

`data/config.json` stays in `.gitignore`. **Do not commit it.**

---

## Artifact and release availability

Firmware binaries are published as assets on [GitHub Releases](https://github.com/KK-ThBer/esp32-flight-tracker-64x64/releases).
The release workflow (`.github/workflows/release.yml`) runs on version tags or
`workflow_dispatch`. Releases are created as **drafts** — the repository owner
must review and publish each release before it is publicly available.

The web installer manifest (`manifest-esp32-dev.json`) references binaries
relative to the same GitHub Release, which provides CORS-compatible public download
URLs. The installer will fail if the release is not yet published or if the
repository is private without additional access configuration.

---

## Credential safety

- Wi-Fi credentials are **never** included in the firmware binary or the LittleFS
  image published in a public release.
- `data/config.json` is in `.gitignore` and must not be committed.
- Only `data/config.example.json` (no real credentials) is committed to the repo.
- If you accidentally commit credentials, rotate them immediately and consider the
  repository history compromised.

---

## GitHub Pages and private repositories

> ⚠ **Important:** if this repository is **private**, GitHub Pages will not serve
> content publicly (free plan), and the GitHub Release download URLs will require
> authentication. A public web flasher page cannot work from a private repository
> without additional infrastructure (e.g. a separate public hosting site or proxy).
>
> **Action required:** to enable the web installer for public use, either make
> the repository public, or configure an alternative hosting solution and update
> the manifest URLs accordingly.

### Activation steps (repository owner)

1. Make the repository **public** (or configure enterprise Pages).
2. Go to **Settings → Pages → Source** and set it to **"GitHub Actions"**.
3. The `pages.yml` workflow will deploy `docs/` on the next push to `main`.
4. The live URL will be `https://kk-thber.github.io/esp32-flight-tracker-64x64/`.
5. Update the placeholder URL in this document and in `README.md`.

---

## Manual PlatformIO fallback

If the browser installer does not work, flash with PlatformIO directly:

```bash
# Build and upload in one step
pio run -e esp32-dev -t upload

# Or specify a port explicitly
pio run -e esp32-dev -t upload --upload-port /dev/ttyUSB0

# Monitor serial output
pio device monitor -e esp32-dev
```

To flash a downloaded release binary manually with `esptool.py`:

```bash
pip install esptool

esptool.py --chip esp32 --baud 921600 \
  --before default_reset --after hard_reset \
  write_flash \
  0x1000  bootloader.bin \
  0x8000  partitions.bin \
  0xe000  boot_app0.bin \
  0x10000 firmware.bin
```

---

## Related links

- [Installer page](web-installer/index.html)
- [ESP Web Tools documentation](https://esphome.github.io/esp-web-tools/)
- [PlatformIO espressif32 documentation](https://docs.platformio.org/en/latest/platforms/espressif32.html)
- [GitHub Releases](https://github.com/KK-ThBer/esp32-flight-tracker-64x64/releases)
- [README](../README.md)
