#pragma once

#include <Arduino.h>

#include "config.h"

/** Current weather data fetched from OpenWeatherMap. */
struct WeatherData {
  float tempCelsius = 0.0F;
  String description;  // max 32 chars
  String iconCode;     // e.g. "01d", "10n"
  bool valid = false;
  uint32_t fetchedAtMs = 0;
};

/**
 * Optional weather client backed by the OpenWeatherMap "current weather"
 * endpoint.  Disabled by default; only active when config.weatherEnabled is
 * true and config.weatherApiKey is non-empty.
 *
 * Memory budget (classic ESP32):
 *   HTTP response cap: 512 bytes (filter-only parse keeps heap use minimal).
 *   WeatherData struct: ~80 bytes.
 *   No heap allocation beyond Arduino String internals.
 *
 * Refresh policy:
 *   Respects config.weatherRefreshSeconds (default 600 s, min 300 s).
 *   Caller checks isStale() before deciding to re-fetch.
 *   On error the previous valid reading is preserved; isError() returns true.
 */
class WeatherClient {
 public:
  /**
   * Fetch current weather for the configured location.
   * Returns true on success; false on network, HTTP, or parse error.
   * On failure errorMessage is set and the previous WeatherData is unchanged.
   */
  bool fetch(const AppConfig &config, WeatherData &data, String &errorMessage);

  /** True when the last reading is older than refreshSeconds. */
  static bool isStale(const WeatherData &data, uint32_t refreshSeconds);
};
