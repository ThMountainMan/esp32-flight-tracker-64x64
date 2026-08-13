# Wiring Guide

## Power

> **Important:** A 64×64 HUB75 panel can draw 3–5 A at full brightness.  
> **Never** power the panel from the ESP32's 3.3 V or 5 V USB rail.

- Connect the panel's 5 V and GND power connector directly to a dedicated **5 V / 4 A+** supply.
- Connect the **GND** of that supply to the **GND** of the ESP32 as well (shared ground).

## Level Shifting (3.3 V → 5 V)

HUB75 panels expect 5 V logic. The ESP32 outputs 3.3 V. Use a **74AHCT245** or **74HCT245**
8-bit bus transceiver (or equivalent) on every HUB75 signal line:

- OE pin of the level shifter → GND (always enabled in output direction)
- DIR pin → 3.3 V (A→B direction, ESP32 → panel)
- VCC → 5 V, GND → GND

A dedicated 8-channel or two 8-channel ICs cover all 13 HUB75 control/data lines.

## Default ESP32 DevKit Pin Mapping

| HUB75 Signal | ESP32 GPIO |
|---|---|
| R1 | 25 |
| G1 | 26 |
| B1 | 27 |
| R2 | 14 |
| G2 | 12 |
| B2 | 13 |
| A  | 23 |
| B  | 22 |
| C  | 21 |
| D  | 19 |
| E  | *-1 (unused for 64-row; use 18 if panel requires)* |
| LAT | 4 |
| OE  | 15 |
| CLK | 2 |

These are the library defaults compiled into `display.cpp`.  
Override any pin via `build_flags` in `platformio.ini`, e.g.:

```ini
build_flags =
    -DMATRIX_PIN_R1=25
    -DMATRIX_PIN_CLK=2
```

## Connector Orientation

HUB75 ribbon cables are keyed but it is easy to plug them in backwards or shifted by one row.  
Check the silk-screen labels on the panel PCB.  If colours appear scrambled or the image is
offset, flip the ribbon 180 ° or recheck the pin-1 marker.

## Safe Initial Bring-Up

1. **Do not connect the panel** – verify the ESP32 boots and logs appear on serial at 115200.
2. Connect the level shifter and panel **without panel power**.  Verify no short on the GND bus.
3. Apply panel 5 V supply (separate from ESP32 USB).
4. Set `brightness` to `16` in `config.json` for first power-on.
5. Confirm the splash screen renders and then gradually increase brightness.
