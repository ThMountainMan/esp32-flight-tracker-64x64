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

  // Weather (optional).  Disabled by default.  Set weatherEnabled to true and
  // provide a valid OpenWeatherMap API key to enable the weather idle scene.
  // The API key is never stored in source code or included in release assets.
  // refreshSeconds is clamped to [300, 3600] at load time.
  bool weatherEnabled = false;
  String weatherApiKey;                   // empty → feature disabled
  uint32_t weatherRefreshSeconds = 600;   // 10-minute default

  // Scheduled brightness (optional).  When scheduleEnabled is true the
  // firmware switches between dayBrightness and nightBrightness according to
  // the HHMM window boundaries below.  Values are in the range 0-255.
  // dayStartHhmm / nightStartHhmm are stored as minutes-past-midnight
  // (0-1439) so "06:30" → 390 and "22:00" → 1320.
  bool scheduleEnabled = false;
  uint8_t dayBrightness = 64;
  uint8_t nightBrightness = 8;
  uint16_t dayStartHhmm = 360;    // 06:00
  uint16_t nightStartHhmm = 1320; // 22:00

  bool load();
  bool saveAtomic() const;
};
