# Troubleshooting

## Config / LittleFS

**Symptom:** `[system] LittleFS initialization failed` on serial.

- Ensure you have uploaded the filesystem: `pio run -e esp32-dev --target uploadfs`
- Confirm `data/config.json` exists and is valid JSON before upload (if pre-seeding manually).
- Try erasing flash first: `pio run -e esp32-dev --target erase` then re-upload both FS and firmware.

**Symptom:** Device starts `FlightTracker-Setup-*` AP instead of normal mode.

- No valid `/config.json` was found. Connect to the AP and complete provisioning.
- If you intentionally uploaded `config.json`, verify JSON syntax and required fields.

**Symptom:** Config loads but values seem wrong.

- Check field names exactly match those in [CONFIG.md](CONFIG.md) – they are case-sensitive.
- Values outside constraints are rejected at boot.
- If both altitude filters are non-zero, `min_altitude_ft` must not exceed `max_altitude_ft`.
- Unit fields must use supported values only:
  - `display.distance_unit`: `km` or `mi`
  - `display.altitude_unit`: `ft` or `m`
  - `display.speed_unit`: `kt`, `kmh`, or `mph`

---

## Wi-Fi

**Symptom:** `[wifi] Connection timed out` repeated.

- Verify SSID and password in `config.json` or rerun provisioning.
- Ensure the network is 2.4 GHz – the classic ESP32 does not support 5 GHz.
- Move the ESP32 closer to the access point.
- After repeated failures, the device automatically starts provisioning mode for recovery.

**Symptom:** Wi-Fi connects then drops repeatedly.

- Check power supply; brownouts cause Wi-Fi resets. Use a USB power supply rated ≥ 1 A
  for the ESP32 alone (panel power must be separate).

---

## HUB75 / Panel / Power

**Symptom:** Panel is dark or shows garbled colours.

- Check the shared GND between the ESP32 and panel power supply.
- Verify level-shifter direction (DIR pin → 3.3 V for A→B, i.e. ESP32 → panel).
- Try swapping the HUB75 ribbon connector orientation; `rotate_180: true` can correct mirroring.

**Symptom:** Image offset or split horizontally.

- The E address pin may be required for your 64-row panel. Set `e = 18` in `display.cpp`
  `defaultPins()` or via a `build_flag`.

**Symptom:** Panel flickers or resets.

- The panel is drawing more current than the supply can provide. Use a 5 V / 4 A+ PSU.
- Reduce `brightness` in `config.json`.

---

## PlatformIO Board ID

**Symptom:** `Error: Unknown board ID: esp32devkit` (or similar).

- The correct board IDs used in this project are `esp32dev` and `esp32-s3-devkitc-1`.
- If you are using a custom board, add a `boards/` folder entry or use the closest matching
  PlatformIO board and override `upload_port`.

---

## FR24 Feed Errors

**Symptom:** `[fr24] HTTP error 429` or `rate limited`.

- Increase `poll_interval_seconds` (minimum recommended: 60).
- The FlightRadar24 data feed is **unofficial and undocumented**; it may change or become
  unavailable without notice. Check `src/fr24_client.cpp` for the current URL format.

**Symptom:** `[fr24] JSON parse error`.

- The FR24 feed response format may have changed. Inspect the raw payload by adding
  `Serial.println(payload)` in `fr24_client.cpp` temporarily.

---

## No Flights Shown

The clock/status screen is shown when `aircraft` is empty. Possible causes:

- You are outside a busy flight corridor – increase `radius_km`.
- The FR24 feed returned zero results for the bounding box (normal at night or remote locations).
- `filters.min_altitude_ft` / `filters.max_altitude_ft` may be excluding flights in your area.
- An error occurred during the last poll – check the status line on the clock screen or serial log.

Note: aircraft records with unknown altitude (`0` from FR24) are kept even when altitude
filters are active.
