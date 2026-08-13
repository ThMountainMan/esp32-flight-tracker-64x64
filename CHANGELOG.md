# ESP32 Flight Tracker changelog

## Unreleased

### Added

- Initial ESP32/Arduino firmware for a 64x64 HUB75 panel.
- LittleFS configuration, Wi-Fi, NTP clock and nearby-flight polling.
- Basic hardware, setup, configuration and troubleshooting documentation.

### Known limitations

- FlightRadar24 feed parsing depends on an external undocumented response format.
- The airline marker is a compact generated badge, not a bitmap-logo library.
- HUB75 pin mapping and panel scan-rate settings need validation on the target hardware.
