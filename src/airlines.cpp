#include "airlines.h"
#include "airline_icons.h"

#include <cstring>
#include <cctype>

namespace {
constexpr AirlineStyle kAirlines[] = {
    {"AAL", 0x001F}, {"ACA", 0xF800}, {"AFR", 0x001F}, {"BAW", 0x0019},
    {"DAL", 0x001F}, {"DLH", 0xFFE0}, {"EZY", 0xFD20}, {"FIN", 0x001F},
    {"IBE", 0xD000}, {"KLM", 0x05FF}, {"QFA", 0xF800}, {"QTR", 0x780F},
    {"RYR", 0x001F}, {"SAS", 0x001F}, {"SIA", 0xFDE0}, {"SWR", 0xF800},
    {"THY", 0xF800}, {"UAL", 0x001F}, {"UAE", 0xF800}, {"VIR", 0xD800},
};

// Case-insensitive 3-char ICAO comparison; icao must be NUL-terminated.
bool icaoMatch(const char *a, const char *b) {
  for (int i = 0; i < 3; ++i) {
    if (toupper((unsigned char)a[i]) != toupper((unsigned char)b[i])) return false;
    if (a[i] == '\0') return false;  // shorter than 3 chars
  }
  return true;
}
}  // namespace

uint16_t airlineColour(const char *icao) {
  if (!icao || icao[0] == '\0') return 0x7BEF;
  for (const AirlineStyle &airline : kAirlines) {
    if (icaoMatch(icao, airline.icao)) return airline.colour;
  }
  return 0x7BEF;
}

void airlineBadgeText(const char *icao, char (&out)[3]) {
  if (icao && icao[0] != '\0' && icao[1] != '\0') {
    out[0] = icao[0];
    out[1] = icao[1];
  } else {
    out[0] = '-';
    out[1] = '-';
  }
  out[2] = '\0';
}

const uint16_t *airlineIcon(const char *icao) {
  if (!icao || icao[0] == '\0') return nullptr;
  // On ESP32 (von Neumann architecture), PROGMEM data is directly readable
  // via normal pointer dereference.  No pgm_read_ptr() indirection needed.
  for (uint8_t i = 0; i < kAirlineIconCount; ++i) {
    if (icaoMatch(icao, kAirlineIcons[i].icao)) {
      return kAirlineIcons[i].data;
    }
  }
  return nullptr;
}
