#include "airlines.h"
#include "airline_icons.h"

namespace {
constexpr AirlineStyle kAirlines[] = {
    {"AAL", 0x001F}, {"ACA", 0xF800}, {"AFR", 0x001F}, {"BAW", 0x0019},
    {"DAL", 0x001F}, {"DLH", 0xFFE0}, {"EZY", 0xFD20}, {"FIN", 0x001F},
    {"IBE", 0xD000}, {"KLM", 0x05FF}, {"QFA", 0xF800}, {"QTR", 0x780F},
    {"RYR", 0x001F}, {"SAS", 0x001F}, {"SIA", 0xFDE0}, {"SWR", 0xF800},
    {"THY", 0xF800}, {"UAL", 0x001F}, {"UAE", 0xF800}, {"VIR", 0xD800},
};
}

uint16_t airlineColour(const String &icao) {
  for (const AirlineStyle &airline : kAirlines) {
    if (icao.equalsIgnoreCase(airline.icao)) return airline.colour;
  }
  return 0x7BEF;
}

String airlineBadgeText(const String &icao) {
  return icao.length() >= 2 ? icao.substring(0, 2) : "--";
}

const uint16_t *airlineIcon(const String &icao) {
  for (uint8_t i = 0; i < kAirlineIconCount; ++i) {
    // kAirlineIcons is in PROGMEM; read each field via pgm_read_ptr
    const char *stored_icao =
        reinterpret_cast<const char *>(pgm_read_ptr(&kAirlineIcons[i].icao));
    if (icao.equalsIgnoreCase(stored_icao)) {
      return reinterpret_cast<const uint16_t *>(
          pgm_read_ptr(&kAirlineIcons[i].data));
    }
  }
  return nullptr;
}
