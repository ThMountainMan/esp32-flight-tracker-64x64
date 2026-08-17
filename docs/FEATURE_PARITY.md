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
| Altitude filters (min/max) | ✅ | `min_altitude_ft` / `max_altitude_ft` config keys; zero disables the threshold. |
| Configurable flight-rotation interval | 📋 | Currently a compile-time constant (`kFlightScreenIntervalMs`). Move to config.json and settings UI. |
| Aircraft route, origin, destination | ✅ | `RouteLookup` fetches origin/destination ICAO codes per flight; `RouteCache` stores results in LittleFS. |
| Home-airport highlighting | ✅ | `homeAirportIcao` config key; highlighted on the flight-detail screen. |
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
| 12 / 24-hour clock toggle | ✅ | `clock24Hour` config key; both formats rendered in `display.cpp`. |
| Weather and temperature display | ✅ | `WeatherClient` polls Open-Meteo; idle scene alternates clock and weather panels. |
| Rainfall / precipitation chart | ⚠️ | Requires extending the current weather scene; evaluate display real-estate on 64×64. |
| Satellite / ISS pass display | ⚠️ | Requires TLE storage, orbital prediction logic, and scene scheduling; evaluate under memory budget. |
| Display themes | 📋 | Add only after core display layout is stable. |
| Scheduled / night brightness | ✅ | `scheduleEnabled`, `dayBrightness`, `nightBrightness`, `dayStartHour`, `nightStartHour` config keys. |

## Setup, configuration, and maintenance

| Feature | ESP32 status | Notes |
|---------|-------------|-------|
| LittleFS JSON configuration | ✅ | Existing config.json stored on-device. |
| Web-flasher documentation | ✅ | Browser-based firmware install documented in docs/WEB_FLASHER.md. |
| First-boot Wi-Fi provisioning portal | ✅ | `ProvisioningPortal` class; captive-portal AP with location/radius/brightness/rotation fields. |
| Authenticated web settings UI | ✅ | `SettingsServer` serves a password-protected settings page; password stored in config.json. |
| Configuration reset / recovery | ✅ | GPIO 0 held for 5 s triggers a full config reset and re-enters the provisioning portal. |
| Serial diagnostics / logs | 📋 | Basic `Serial.printf` output exists; consider a bounded in-memory log view later. |
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
  and a large logo catalogue. The ESP32 has constrained flash and RAM; the
  route lookup fetches data per-flight and caches results in LittleFS instead.
- **FlightRadar24 is undocumented and fragile.** Prioritise tar1090 as a
  resilient local alternative; keep FR24 as a fallback only.
- **Weather works without PSRAM** using Open-Meteo (no API key). Memory-hungry
  features such as satellite tracking, rich web UI extensions, and OTA still
  require an ESP32-S3 / PSRAM board and a memory + security design review
  before implementation begins.
- **Raspberry Pi hardware is out of scope.** The RGB Matrix Bonnet and the
  desktop simulator are Raspberry Pi concerns, not ESP32 firmware parity targets.

---

## Suggested delivery order

1. ~~**Provisioning and browser-flasher readiness**~~ ✅ First-boot Wi-Fi portal, safe
   credential handling, GPIO recovery, authenticated settings UI, and web-flasher
   documentation are all complete.
2. ~~**CI, board, display, and config stability**~~ ✅ Altitude filters, 12/24-hour
   clock, unit preferences, scheduled brightness, route lookup, and home-airport
   highlight are all merged.
3. **Configurable flight-rotation interval** — move `kFlightScreenIntervalMs` from
   a compile-time constant to a config.json field and expose it in the settings UI.
4. **tar1090 local ADS-B source** — configurable endpoint, FR24 fallback, resilient
   error handling.  Prerequisite: none beyond Wi-Fi.
5. **Rainfall / precipitation chart** — extend the weather idle scene; evaluate
   display real-estate on the 64×64 grid.
6. **Evaluate satellite / ISS pass scene under memory budget** — TLE storage,
   orbital prediction logic, and scene scheduling; requires ESP32-S3 / PSRAM.
7. **OpenSky Network source** — OAuth credential storage, rate-limit handling,
   error recovery; requires a design phase first.
8. **Settings, diagnostics, and OTA** — bounded in-memory log view, signed OTA
   update flow with security design review.
