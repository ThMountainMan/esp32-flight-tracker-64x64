# Asset Provenance, Licensing, and Trademark Analysis

<!-- SPDX-License-Identifier: GPL-3.0-or-later -->

## Summary

**No third-party airline logo images have been imported into this project.**

The 16×16 airline glyphs in `src/airline_icons.h` are **original artwork**
created by this project.  They are simple coloured tiles with two-letter
initials; they do not reproduce any airline's trademark logo or livery device.

---

## Upstream Investigation – ColinWaddell/FlightTracker

The problem statement referenced
[ColinWaddell/FlightTracker @ 680e413](https://github.com/ColinWaddell/FlightTracker/tree/680e413fea00c205f244afed33558a95f5c2006f)
and specifically `assets/airlines/airline_logos_16/`.

| Item | Finding |
|---|---|
| Repository license | GPL-3.0-or-later (`LICENSE.md`) |
| `airline_logos_16/` contents | Actual airline brand logos (trademark images) |
| Provenance of logo images | Not documented in the repository |
| Explicit redistribution grant from airlines | None found |
| Compatible with copying into this project | **No** |

### Analysis

The GPL-3.0 license covers the *code* in ColinWaddell/FlightTracker.
Airline logos are independent trademark-protected works owned by the respective
airlines.  Including them in a GPL repository does not grant redistribution
rights.  No airline has explicitly licensed their logo for inclusion in
open-source firmware projects.

**Decision: no upstream airline logo images were copied.**

---

## Glyphs in This Project

### What they are

Each entry in `src/airline_icons.h` is a 16×16 pixel RGB565 bitmap generated
by `tools/gen_icons.py` at build time.  The design is:

1. Solid background fill using the airline's publicly-known *primary livery
   colour*.  Livery colours are factual information, not copyrightable.
2. A two-letter IATA-derived initial code rendered in a contrasting colour.
3. A 1-pixel contrasting border for visual separation.

These glyphs do not reproduce any airline's wordmark, logo, livery device,
fuselage graphic, or any other trademark element.

### Trademark caveat

Even though the glyphs are original artwork, displaying an airline's name or
ICAO/IATA code alongside its representative colour on a consumer device is
generally acceptable for informational purposes (flight tracking) under fair
use / fair dealing principles in most jurisdictions.  This project does not
claim affiliation with any airline.

---

## Supported ICAO Codes

| ICAO | Label | Background colour | Airline (informational) |
|------|-------|-------------------|-------------------------|
| AAL  | AA    | #00177F (navy)    | American Airlines        |
| ACA  | AC    | #D20019 (red)     | Air Canada               |
| AFR  | AF    | #001E96 (blue)    | Air France               |
| BAW  | BA    | #00277C (dark blue) | British Airways        |
| DAL  | DL    | #001E96 (blue)    | Delta Air Lines          |
| DLH  | LH    | #FFD000 (yellow)  | Lufthansa                |
| EZY  | EZ    | #FF6600 (orange)  | easyJet                  |
| FIN  | AY    | #00227B (blue)    | Finnair                  |
| IBE  | IB    | #C80000 (red)     | Iberia                   |
| KLM  | KL    | #00A3E0 (light blue) | KLM Royal Dutch Airlines |
| QFA  | QF    | #FF0000 (red)     | Qantas                   |
| QTR  | QR    | #5C0632 (maroon)  | Qatar Airways            |
| RYR  | FR    | #0031CA (blue)    | Ryanair                  |
| SAS  | SK    | #003F8F (blue)    | Scandinavian Airlines    |
| SIA  | SQ    | #005280 (teal)    | Singapore Airlines       |
| SWR  | LX    | #DC0032 (red)     | Swiss International      |
| THY  | TK    | #C80000 (red)     | Turkish Airlines         |
| UAL  | UA    | #002C77 (blue)    | United Airlines          |
| UAE  | EK    | #C60026 (red)     | Emirates                 |
| VIR  | VS    | #C8002E (red)     | Virgin Atlantic          |

---

## Fallback Behaviour

If `airlineIcon(icao)` returns `nullptr` (no glyph registered for that
designator), `Display::showFlight` automatically falls back to the existing
compact badge:

* Background: `airlineColour(icao)` – the representative RGB565 colour.
* Text: `airlineBadgeText(icao)` – the first two characters of the ICAO code.

This ensures every airline is displayed even if it is not in the curated set.

---

## Icon Generation Instructions

### Requirements

```
Python >= 3.9
Pillow >= 9.2
```

Install dependencies:

```bash
pip install Pillow
```

### Regenerate `src/airline_icons.h`

```bash
python3 tools/gen_icons.py
```

The script is deterministic: given the same `AIRLINES` table in
`tools/gen_icons.py`, the output is always identical.

### Adding a new icon

1. Add an entry to the `AIRLINES` list in `tools/gen_icons.py`:

   ```python
   {"icao": "XYZ", "bg": (R, G, B), "fg": (R, G, B), "label": "XY"},
   ```

2. Re-run `python3 tools/gen_icons.py`.
3. Commit both `tools/gen_icons.py` and the regenerated `src/airline_icons.h`.

### Design constraints

* `bg` and `fg` must be `(R, G, B)` tuples with 8-bit components.
* `label` must be exactly 2 ASCII characters.
* Icon size is fixed at 16×16 px; do not change `ICON_SIZE`.

---

## Flash/RAM Trade-offs

| Item | Size |
|---|---|
| One 16×16 RGB565 glyph | 512 bytes (256 × uint16_t) |
| 20 glyphs (full table) | ~10 240 bytes in flash (PROGMEM) |
| Runtime RAM during render | 512 bytes stack buffer (one icon at a time) |
| Fallback text badge | 0 bytes additional flash |

The entire icon table consumes ≈10 KB of flash.  An ESP32 with 4 MB flash
can accommodate this comfortably.  No icons are copied into heap RAM; a
single 512-byte stack buffer is used for one `drawRGBBitmap` call at a time.

---

*Last updated: 2026-08-13*
