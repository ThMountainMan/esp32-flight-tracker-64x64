# ESP32 Feature-Parity Roadmap

This document tracks the functional gap between this ESP32 firmware and the
original Raspberry Pi / Python project at
[ColinWaddell/FlightTracker](https://github.com/ColinWaddell/FlightTracker).
It is a **planning guide**, not a promise to reproduce every Raspberry Pi
feature unchanged on ESP32 hardware.

## Status legend

| Symbol | Meaning |
|--------|---------|
| ✅ | Implemented |
| 🚧 | In progress / pending PR |
| 📋 | Planned |
| ⚠️ | Requires design or external service before implementation |
| ❌ | Not planned for ESP32 |

---

## Core flight tracking

| Feature | ESP32 status | Notes |
|---------|-------------|-------|
| Nearby aircraft via FlightRadar24 | ✅ | Bounded feed request, radius filtering, flight telemetry display. |
| Callsign, type, altitude, speed, heading, distance | ✅ | Compact display on 64×64 panel. |
| Altitude filters (min/max) | 🚧 | Added in display-preferences work; not yet merged. |
| Configurable flight-rotation interval | 🚧 | Part of display-preferences PR; not yet merged. |
| Aircraft route, origin, destination | 📋 | Add lightweight per-flight lookup with a small LittleFS cache; do not bundle a large airport database. |
| Home-airport highlighting | 📋 | Depends on route/airport cache. |
| Optional bounding-box search mode | 📋 | Keep centre + radius as the default; add bounding box only if needed. |

## Data sources

| Feature | ESP32 status | Notes |
|---------|-------------|-------|
| FlightRadar24 feed | ✅ | Undocumented external feed. Retain conservative 60 s polling. FR24 should be treated as a fragile primary source. |
| Local tar1090 / ADS-B receiver | 📋 | High-priority resilient alternative. Add configurable endpoint with FR24 as fallback. Prefer this over FR24 long-term. |
| OpenSky Network | 📋 | Requires OAuth credential storage, rate-limit handling, and error recovery. |

## Display and idle scenes

| Feature | ESP32 status | Notes |
|---------|-------------|-------|
| Flight telemetry screen | ✅ | Callsign, type, altitude, speed, heading, distance. |
| Compact airline badges / colour icons | ✅ | Asset set is intentionally small; avoid embedding large third-party logo catalogues. |
| Idle clock | ✅ | Shown when no aircraft match the configured radius. |
| 12 / 24-hour clock toggle | 🚧 | Part of display-preferences PR; not yet merged. |
| Weather and temperature display | ⚠️ | Requires API-key management and memory budget analysis; evaluate under ESP32-S3 / PSRAM constraints. |
| Rainfall / precipitation chart | ⚠️ | Depends on weather module above. |
| Satellite / ISS pass display | ⚠️ | Requires TLE storage, orbital prediction logic, and scene scheduling; evaluate under memory budget. |
| Display themes | 📋 | Add only after core display layout is stable. |
| Scheduled / night brightness | 📋 | Embedded-friendly feature; can be implemented before weather or satellite work. |

## Setup, configuration, and maintenance

| Feature | ESP32 status | Notes |
|---------|-------------|-------|
| LittleFS JSON configuration | ✅ | Existing config.json stored on-device. |
| Web-flasher documentation | ✅ | Browser-based firmware install documented in docs/WEB_FLASHER.md. |
| First-boot Wi-Fi provisioning portal | 🚧 | Required before credential-free browser-flashed firmware is practical. |
| Authenticated web settings UI | 📋 | Reuse the provisioning web stack post-setup; scope carefully to stay within RAM. |
| Configuration reset / recovery | 📋 | Document and implement a deliberate recovery path (e.g. GPIO-triggered reset). |
| Serial diagnostics / logs | 📋 | Start with serial output; consider a bounded in-memory log view later. |
| OTA firmware update check and apply | ⚠️ | Requires a signed release pipeline and OTA security design before implementation. |
| External loading LED (optional) | 📋 | Low-priority; optional GPIO setting. |

## Hardware and platform differences

| Feature | ESP32 status | Notes |
|---------|-------------|-------|
| 64×64 HUB75 panel via I2S / DMA | ✅ | Direct ESP32 drive with 3.3 V → 5 V level shifters. |
| Raspberry Pi RGB Matrix Bonnet | ❌ | Raspberry Pi HAT; not compatible with ESP32 and not a parity target. |
| Raspberry Pi desktop simulator | ❌ | Separate platform architecture; not part of firmware parity. |
| ESP32-S3 with PSRAM | 📋 | Preferred target for weather, satellite, OTA, and richer web UI due to larger RAM. |

---

## ESP32 constraints and decisions

- **No large bundled datasets.** The original project ships an airport database
  and a large logo catalogue. The ESP32 has constrained flash and RAM; use a
  small LittleFS cache populated at runtime instead.
- **FlightRadar24 is undocumented and fragile.** Prioritise tar1090 as a
  resilient local alternative; keep FR24 as a fallback only.
- **Memory-hungry features need a design phase.** Weather display, satellite
  tracking, a rich settings web UI, and OTA all require an ESP32-S3 / PSRAM
  board and a memory + security design review before implementation begins.
- **Raspberry Pi hardware is out of scope.** The RGB Matrix Bonnet and the
  desktop simulator are Raspberry Pi concerns, not ESP32 firmware parity targets.

---

## Suggested delivery order

1. **Provisioning and browser-flasher readiness** — first-boot Wi-Fi portal,
   safe credential handling, and browser flasher tested end-to-end.
2. **CI, board, display, and config stability** — board IDs, display-preferences
   PR merged, configuration validation, and CI green on both targets.
3. **tar1090 local ADS-B source** — configurable endpoint, FR24 fallback,
   resilient error handling.
4. **Route and airport cache** — lightweight per-flight lookup, small LittleFS
   cache, home-airport highlight.
5. **Brightness scheduling and user preferences** — scheduled/night brightness,
   display theme option.
6. **Evaluate weather and satellite under memory budget** — only proceed if an
   ESP32-S3 / PSRAM build can comfortably accommodate the additional modules.
7. **Settings, diagnostics, and OTA** — authenticated settings page, serial/web
   diagnostics, signed OTA update flow.
