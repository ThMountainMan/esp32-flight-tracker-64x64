#pragma once

#include <Arduino.h>

/** Settings loaded from /config.json in LittleFS. */
struct AppConfig {
  String wifiSsid;
  String wifiPassword;
  float latitude = 55.87F;
  float longitude = -4.25F;
  float radiusKm = 30.0F;
  uint8_t brightness = 64;
  uint32_t flightScreenSeconds = 5;
  String distanceUnit = "km";
  String altitudeUnit = "ft";
  String speedUnit = "kt";
  bool clock24Hour = true;
  uint32_t pollIntervalSeconds = 60;
  int minAltitudeFeet = 0;
  int maxAltitudeFeet = 0;
  bool rotate180 = false;

  bool load();
  bool saveAtomic() const;
};
