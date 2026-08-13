#!/usr/bin/env python3
"""
gen_icons.py – Generate original 16×16 RGB565 airline colour glyphs.

These are NOT airline trademark logos.  Each glyph is an original geometric
design created in this project, using only the airline's publicly-known primary
livery colour combined with a simple two-letter initial rendered in a
contrasting colour.  No third-party images are imported or converted.

Usage:
    pip install Pillow
    python3 tools/gen_icons.py

Output:
    src/airline_icons.h   — C++ header with constexpr RGB565 arrays

The generated file is deterministic: given the same AIRLINES table the output
is always identical.
"""

# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright (C) 2026 KK-ThBer/esp32-flight-tracker-64x64 contributors

from __future__ import annotations

from pathlib import Path
from PIL import Image, ImageDraw, ImageFont

# ---------------------------------------------------------------------------
# Airline definition table
#   icao    – 3-letter ICAO designator
#   bg      – background colour as (R, G, B) 8-bit tuple (livery colour)
#   fg      – foreground/text colour as (R, G, B)
#   label   – 2-character label drawn on the glyph
#
# Colours are chosen from publicly available livery references; they are facts
# (colours are not copyrightable).  No logo graphics are reproduced.
# ---------------------------------------------------------------------------
AIRLINES: list[dict] = [
    # ICAO    BG-RGB                          FG-RGB               label
    {"icao": "AAL", "bg": (0x00, 0x17, 0x7F), "fg": (0xFF, 0xFF, 0xFF), "label": "AA"},
    {"icao": "ACA", "bg": (0xD2, 0x00, 0x19), "fg": (0xFF, 0xFF, 0xFF), "label": "AC"},
    {"icao": "AFR", "bg": (0x00, 0x1E, 0x96), "fg": (0xFF, 0xFF, 0xFF), "label": "AF"},
    {"icao": "BAW", "bg": (0x00, 0x27, 0x7C), "fg": (0xFF, 0xFF, 0xFF), "label": "BA"},
    {"icao": "DAL", "bg": (0x00, 0x1E, 0x96), "fg": (0xFF, 0xFF, 0xFF), "label": "DL"},
    {"icao": "DLH", "bg": (0xFF, 0xD0, 0x00), "fg": (0x00, 0x00, 0x00), "label": "LH"},
    {"icao": "EZY", "bg": (0xFF, 0x66, 0x00), "fg": (0xFF, 0xFF, 0xFF), "label": "EZ"},
    {"icao": "FIN", "bg": (0x00, 0x22, 0x7B), "fg": (0xFF, 0xFF, 0xFF), "label": "AY"},
    {"icao": "IBE", "bg": (0xC8, 0x00, 0x00), "fg": (0xFF, 0xFF, 0xFF), "label": "IB"},
    {"icao": "KLM", "bg": (0x00, 0xA3, 0xE0), "fg": (0xFF, 0xFF, 0xFF), "label": "KL"},
    {"icao": "QFA", "bg": (0xFF, 0x00, 0x00), "fg": (0xFF, 0xFF, 0xFF), "label": "QF"},
    {"icao": "QTR", "bg": (0x5C, 0x06, 0x32), "fg": (0xFF, 0xFF, 0xFF), "label": "QR"},
    {"icao": "RYR", "bg": (0x00, 0x31, 0xCA), "fg": (0xFF, 0xFF, 0xFF), "label": "FR"},
    {"icao": "SAS", "bg": (0x00, 0x3F, 0x8F), "fg": (0xFF, 0xFF, 0xFF), "label": "SK"},
    {"icao": "SIA", "bg": (0x00, 0x52, 0x80), "fg": (0xFF, 0xD5, 0x00), "label": "SQ"},
    {"icao": "SWR", "bg": (0xDC, 0x00, 0x32), "fg": (0xFF, 0xFF, 0xFF), "label": "LX"},
    {"icao": "THY", "bg": (0xC8, 0x00, 0x00), "fg": (0xFF, 0xFF, 0xFF), "label": "TK"},
    {"icao": "UAL", "bg": (0x00, 0x2C, 0x77), "fg": (0xFF, 0xFF, 0xFF), "label": "UA"},
    {"icao": "UAE", "bg": (0xC6, 0x00, 0x26), "fg": (0xFF, 0xFF, 0xFF), "label": "EK"},
    {"icao": "VIR", "bg": (0xC8, 0x00, 0x2E), "fg": (0xFF, 0xFF, 0xFF), "label": "VS"},
]

ICON_SIZE = 16  # pixels


def rgb_to_rgb565(r: int, g: int, b: int) -> int:
    """Convert 8-bit RGB to packed 16-bit RGB565."""
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)


def make_glyph(airline: dict) -> list[int]:
    """
    Render a 16x16 original colour glyph for one airline.

    Design: solid background fill with a 2-letter initial centred in
    contrasting foreground colour.  A 1-pixel border in the foreground
    colour provides visual separation.  The design is completely original;
    no airline trademark or logo is reproduced.
    """
    img = Image.new("RGB", (ICON_SIZE, ICON_SIZE), airline["bg"])
    draw = ImageDraw.Draw(img)

    fg = airline["fg"]
    # 1-pixel border
    draw.rectangle([0, 0, ICON_SIZE - 1, ICON_SIZE - 1], outline=fg)

    # Draw 2-letter initial using default bitmap font
    label = airline["label"]
    font = ImageFont.load_default()

    try:
        bbox = draw.textbbox((0, 0), label, font=font)
        tw, th = bbox[2] - bbox[0], bbox[3] - bbox[1]
    except AttributeError:
        tw, th = draw.textsize(label, font=font)
    tx = (ICON_SIZE - tw) // 2
    ty = (ICON_SIZE - th) // 2
    draw.text((tx, ty), label, fill=fg, font=font)

    pixels: list[int] = []
    for y in range(ICON_SIZE):
        for x in range(ICON_SIZE):
            r, g, b = img.getpixel((x, y))
            pixels.append(rgb_to_rgb565(r, g, b))
    return pixels


def format_pixels(pixels: list[int]) -> str:
    """Format 256 RGB565 values as a C initialiser list, 16 per row."""
    rows = []
    for row_start in range(0, ICON_SIZE * ICON_SIZE, ICON_SIZE):
        row = pixels[row_start : row_start + ICON_SIZE]
        rows.append("    " + ", ".join(f"0x{v:04X}" for v in row))
    return ",\n".join(rows)


REPO_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_HEADER = REPO_ROOT / "src" / "airline_icons.h"


def main() -> None:
    icon_decls: list[str] = []
    lookup_entries: list[str] = []

    for airline in AIRLINES:
        icao = airline["icao"]
        pixels = make_glyph(airline)
        var_name = f"kIcon_{icao}"

        icon_decls.append(
            f"// Original glyph for {icao} –"
            f" bg #{airline['bg'][0]:02X}{airline['bg'][1]:02X}{airline['bg'][2]:02X}"
            f"  fg #{airline['fg'][0]:02X}{airline['fg'][1]:02X}{airline['fg'][2]:02X}\n"
            f"static const uint16_t {var_name}[{ICON_SIZE * ICON_SIZE}] PROGMEM = {{\n"
            f"{format_pixels(pixels)}\n"
            f"}};"
        )
        lookup_entries.append(f'  {{"{icao}", {var_name}}}')

    header_parts = [
        """\
// airline_icons.h  -  Original 16x16 RGB565 colour glyphs for airline display
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 KK-ThBer/esp32-flight-tracker-64x64 contributors
//
// *** GENERATED FILE - do not edit by hand ***
// Regenerate with:  python3 tools/gen_icons.py
//
// These glyphs are ORIGINAL ARTWORK created by this project.
// They are NOT copies of airline trademarks or logos.
// Each glyph is a simple coloured tile with the airline's 2-letter IATA
// initials rendered in a contrasting colour over the airline's primary
// livery colour (a publicly-known fact, not protected by copyright).
//
// No images from ColinWaddell/FlightTracker or any other third-party source
// have been imported or converted.  See docs/ASSETS.md for full provenance
// and trademark analysis.
//
// Data format: each array contains 256 uint16_t values (16x16 pixels,
// row-major, RGB565 big-endian as expected by
// MatrixPanel_I2S_DMA::drawRGBBitmap).

#pragma once
#include <Arduino.h>

// Width/height of every icon in pixels.
static constexpr uint8_t kAirlineIconSize = 16;

// Number of entries in kAirlineIcons[].
static constexpr uint8_t kAirlineIconCount = """ + str(len(AIRLINES)) + """;

// Lookup entry: ICAO code mapped to a pointer to its PROGMEM pixel data.
struct AirlineIcon {
  const char *icao;        // 3-letter ICAO designator (null-terminated)
  const uint16_t *data;    // pointer to 256 RGB565 words stored in PROGMEM
};
""",
        "\n\n".join(icon_decls),
        "\n// Ordered lookup table (linear scan at this scale is sufficient).",
        f"static const AirlineIcon kAirlineIcons[kAirlineIconCount] PROGMEM = {{",
        ",\n".join(lookup_entries),
        "};",
    ]

    OUTPUT_HEADER.write_text("\n".join(header_parts), encoding="utf-8")
    print(f"Written {OUTPUT_HEADER} ({len(AIRLINES)} icons)")


if __name__ == "__main__":
    main()
