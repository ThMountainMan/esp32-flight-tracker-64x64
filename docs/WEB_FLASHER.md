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

### Part 1 — Flash the firmware

1. Connect the ESP32 to your computer with a USB data cable.
2. Open the web installer:
   - **GitHub Pages URL:** `https://thmountainman.github.io/esp32-flight-tracker-64x64/web-installer/`
     _(placeholder — see [Activation](#activation) below)_
   - **Local fallback:** open `docs/web-installer/index.html` in Chrome/Edge directly.
3. Click **"Connect & Flash esp32-dev"**.
4. In the browser dialog, select the serial port for your ESP32.
5. Follow the on-screen prompts. The installer will erase and write all required
   partitions automatically. This takes about 30–60 seconds.

### Part 2 — Configure on first boot

Once the flash completes, the device starts a temporary Wi-Fi setup network
automatically. No extra software or USB connection is needed for this step.

6. **Disconnect from your normal Wi-Fi** on your phone or laptop.
7. Connect to the Wi-Fi network named **`FlightTracker-Setup-XXXXXX`**
   (the last characters are unique to your device).
8. Open a browser and go to **`http://192.168.4.1/`**
   (on most phones a captive-portal popup will appear automatically — tap it).
9. Fill in:
   - **Wi-Fi SSID** and **Password** — your home/office network credentials.
   - **Latitude** and **Longitude** — your location (decimal degrees).
   - **Radius (km)** — how far around you to track aircraft.
   - Any other display settings you want to change.
10. Tap **Save**. The device will write the configuration, then reboot and join
    your Wi-Fi network. The setup AP disappears at this point.
11. **Reconnect your phone/laptop to your normal Wi-Fi** and you are done.

> **Tip:** if you miss the setup window, the AP times out after 10 minutes and
> the device reboots back into setup mode automatically — just reconnect to
> `FlightTracker-Setup-XXXXXX` and try again.

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

> **This is the normal configuration path — no extra tools or USB connection required.**

When the firmware boots for the first time (or any time no valid `config.json` exists on
the device), it automatically starts a temporary Wi-Fi access point and serves a
configuration page. Follow the steps in [Part 2](#part-2--configure-on-first-boot) above.

The provisioning AP is named `FlightTracker-Setup-XXXXXX` and listens on `192.168.4.1`.
The portal page accepts:

| Field | Description |
|-------|-------------|
| Wi-Fi SSID | Your network name |
| Wi-Fi Password | Your network password |
| Latitude / Longitude | Your location in decimal degrees |
| Radius (km) | Aircraft tracking radius |
| Brightness | Display brightness (0–255) |
| Rotate 180° | Flip the display if mounted upside-down |

After you submit the form the device writes `/config.json` to its internal filesystem
and reboots. The setup AP is no longer available once the device has joined your network.

If the device cannot reach the saved network on boot (wrong password, network
unavailable) it will fall back into provisioning mode after a short delay, so you can
correct the settings.

### Advanced: pre-seeding config with PlatformIO (optional)

If you prefer to set credentials before the first boot — for example when deploying
multiple devices — you can upload a pre-filled `config.json` directly via PlatformIO
instead of using the captive portal:

```bash
cp data/config.example.json data/config.json
# Edit data/config.json with your credentials and location
pio run -e esp32-dev -t uploadfs
```

`data/config.json` stays in `.gitignore`. **Do not commit it.**

---

## Artifact and release availability

Firmware binaries are published as assets on [GitHub Releases](https://github.com/ThMountainMan/esp32-flight-tracker-64x64/releases).
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
4. The live URL will be `https://thmountainman.github.io/esp32-flight-tracker-64x64/`.
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
- [GitHub Releases](https://github.com/ThMountainMan/esp32-flight-tracker-64x64/releases)
- [README](../README.md)
