#pragma once

#include <Arduino.h>

struct AirlineStyle {
  const char *icao;
  uint16_t colour;
};

/** Return a representative RGB565 colour for an airline ICAO designator. */
uint16_t airlineColour(const String &icao);
/** Return a two-character fallback badge for use on the small matrix. */
String airlineBadgeText(const String &icao);
