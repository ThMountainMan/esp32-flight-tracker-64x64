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
  uint32_t pollIntervalSeconds = 60;
  bool rotate180 = false;

  bool load();
};
