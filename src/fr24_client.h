#pragma once

#include <Arduino.h>
#include <vector>

#include "config.h"

/** Resolution state for the optional route/airport lookup. */
enum class RouteState : uint8_t {
  Unknown = 0,  ///< No lookup attempted yet.
  Pending,      ///< Lookup requested but not yet complete.
  Resolved,     ///< Origin and destination are populated.
  Failed,       ///< Lookup attempted but no data returned.
};

struct Aircraft {
  char id[10];          // FR24 hex flight ID (e.g. "3c4b21")
  char callsign[9];     // ICAO callsign or flight number (max 8 chars + NUL)
  char type[5];         // ICAO aircraft type code (max 4 chars + NUL)
  char airlineIcao[4];  // 3-letter ICAO airline designator + NUL
  float latitude = 0.0F;
  float longitude = 0.0F;
  float distanceKm = 0.0F;
  int altitudeFeet = 0;
  int speedKnots = 0;
  int headingDegrees = 0;

  // Optional route data – populated by RouteLookup after a successful lookup.
  char origin[5] = {};       // ICAO airport code (4 chars + NUL), or empty.
  char destination[5] = {};  // ICAO airport code (4 chars + NUL), or empty.
  RouteState routeState = RouteState::Unknown;

  Aircraft() {
    id[0] = '\0';
    callsign[0] = '\0';
    type[0] = '\0';
    airlineIcao[0] = '\0';
    origin[0] = '\0';
    destination[0] = '\0';
  }
};

/** Retrieves a bounded FR24 feed and keeps only aircraft near the observer. */
class Fr24Client {
 public:
  bool fetchNearby(const AppConfig &config, std::vector<Aircraft> &aircraft,
                   String &errorMessage);

 private:
  static float distanceKm(float latA, float lonA, float latB, float lonB);
  static void airlineFromCallsign(const char *callsign, char (&out)[4]);
};
