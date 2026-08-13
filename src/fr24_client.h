#pragma once

#include <Arduino.h>
#include <vector>

#include "config.h"

struct Aircraft {
  String id;
  String callsign;
  String type;
  String airlineIcao;
  float latitude = 0.0F;
  float longitude = 0.0F;
  float distanceKm = 0.0F;
  int altitudeFeet = 0;
  int speedKnots = 0;
  int headingDegrees = 0;
};

/** Retrieves a bounded FR24 feed and keeps only aircraft near the observer. */
class Fr24Client {
 public:
  bool fetchNearby(const AppConfig &config, std::vector<Aircraft> &aircraft,
                   String &errorMessage);

 private:
  static float distanceKm(float latA, float lonA, float latB, float lonB);
  static String airlineFromCallsign(const String &callsign);
};
