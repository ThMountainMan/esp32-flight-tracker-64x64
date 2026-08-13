#pragma once

#include <Arduino.h>

struct AirlineStyle {
  const char *icao;
  uint16_t colour;
};

// Side length (pixels) of an airline icon glyph; matches kAirlineIconSize in
// airline_icons.h.  Defined here so callers of airlineIcon() do not need to
// pull in the full (large) airline_icons.h header.
static constexpr uint8_t kIconSize = 16;

/** Return a representative RGB565 colour for an airline ICAO designator. */
uint16_t airlineColour(const String &icao);
/** Return a two-character fallback badge for use on the small matrix. */
String airlineBadgeText(const String &icao);

/**
 * Look up a 16x16 RGB565 icon for an airline ICAO designator.
 *
 * Returns a pointer to a 256-element uint16_t array (row-major, RGB565)
 * stored in PROGMEM/flash, or nullptr if no icon is registered for this
 * ICAO code.  Callers must not free this pointer.
 *
 * When nullptr is returned, fall back to airlineBadgeText() / airlineColour().
 */
const uint16_t *airlineIcon(const String &icao);
