# Architecture

## Overview

```
┌──────────────────────────────────────────────────────┐
│  main.cpp                                            │
│  - setup(): load config → init display → Wi-Fi      │
│  - loop(): poll timer → fetchNearby → render screen  │
└────────┬────────────┬────────────────────────────────┘
         │            │
         ▼            ▼
   fr24_client    display
         │            │
         ▼            ▼
      config       airlines
```

## Modules

### `config` (`src/config.h/.cpp`)

Loads `/config.json` from LittleFS at startup using ArduinoJson.  
Provides `AppConfig` struct consumed by all other modules.

### `fr24_client` (`src/fr24_client.h/.cpp`)

- Issues an HTTP GET to the FlightRadar24 live feed URL with a bounding-box query
  derived from the observer's latitude/longitude and `radius_km`.
- Parses the JSON response into a `std::vector<Aircraft>`.
- Filters to aircraft within the configured radius using the Haversine formula.
- Returns an error string on failure (network, parse, etc.).

### `display` (`src/display.h/.cpp`)

- Owns a single `MatrixPanel_I2S_DMA *` instance.
- Provides four screen modes: **splash**, **status**, **clock**, **flight**.
- Flight screen shows: airline badge, callsign, aircraft type, altitude, speed,
  distance, heading arrow, and pagination counter.
- Airline colours and 2-character badge text come from `airlines`.

### `airlines` (`src/airlines.h/.cpp`)

- Provides `airlineColour(icao)` and `airlineBadgeText(icao)` for a compact list
  of common airline ICAO designators.
- Returns a grey fallback for unknown airlines.
- **No external logo assets** – keeps flash usage low.

### `main` (`src/main.cpp`)

Orchestrates the application:
1. Initialises LittleFS, config, display, Wi-Fi, NTP.
2. Polls FR24 every `poll_interval_seconds`.
3. Cycles flight screens every 5 s; shows the clock when no flights are tracked.
4. Reconnects Wi-Fi automatically on drop.

## Memory and UI Constraints

- The 64×64 panel has 4096 pixels – text size 1 gives 6×8 px glyphs (about 10 chars wide).
- `std::vector<Aircraft>` is heap-allocated; large radii with many aircraft may exhaust
  the 320 KB DRAM on classic ESP32 boards.
- The I2S DMA frame buffer requires ~24 KB of contiguous DRAM per chain.
- No PSRAM or SPIRAM support is assumed; avoid extremely large radii.

## Data Flow

```
LittleFS/config.json
        │
        ▼
   AppConfig
        │
   ┌────┴──────────────────┐
   │                       │
   ▼                       ▼
Fr24Client.fetchNearby  Display.begin
        │                       │
        ▼                       ▼
 vector<Aircraft>       MatrixPanel_I2S_DMA
        │
        └──► Display.showFlight / showClock
```
