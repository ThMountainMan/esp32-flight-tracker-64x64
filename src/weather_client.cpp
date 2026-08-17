// weather_client.cpp  –  OpenWeatherMap current-weather fetch
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 KK-ThBer/esp32-flight-tracker-64x64 contributors

#include "weather_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

namespace {
// Maximum JSON body length accepted from the server (bytes).
// OWM current.json responses are typically 400-500 bytes with metric units
// and a minimal set of fields; 1024 bytes gives comfortable headroom while
// keeping heap use predictable on a classic ESP32.
constexpr size_t kMaxResponseBytes = 1024;
constexpr uint16_t kHttpTimeoutMs = 8000;
constexpr int kMinTempC = -90;
constexpr int kMaxTempC = 70;
}  // namespace

bool WeatherClient::fetch(const AppConfig &config, WeatherData &data,
                          String &errorMessage) {
  if (!config.weatherEnabled || config.weatherApiKey.isEmpty()) {
    errorMessage = "Weather disabled";
    return false;
  }

  // Build URL: metric units, current weather endpoint.
  String url = "http://api.openweathermap.org/data/2.5/weather"
               "?lat=";
  url += String(config.latitude, 6);
  url += "&lon=";
  url += String(config.longitude, 6);
  url += "&units=metric&appid=";
  url += config.weatherApiKey;

  HTTPClient http;
  http.setTimeout(kHttpTimeoutMs);
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);

  if (!http.begin(url)) {
    errorMessage = "WX: URL error";
    return false;
  }
  const int httpCode = http.GET();
  if (httpCode != 200) {
    http.end();
    errorMessage = "WX: HTTP " + String(httpCode);
    Serial.printf("[weather] HTTP %d\n", httpCode);
    return false;
  }

  // Read up to kMaxResponseBytes; reject oversized responses.
  const int contentLength = http.getSize();
  if (contentLength > static_cast<int>(kMaxResponseBytes)) {
    http.end();
    errorMessage = "WX: response too large";
    Serial.printf("[weather] Response too large (%d bytes)\n", contentLength);
    return false;
  }

  String body = http.getString();
  http.end();

  if (body.length() > kMaxResponseBytes) {
    errorMessage = "WX: body too large";
    return false;
  }

  // Use a filter document to avoid deserialising unused fields.
  // This keeps heap use very low (classic ESP32 IRAM/DRAM).
  JsonDocument filter;
  filter["main"]["temp"] = true;
  filter["weather"][0]["description"] = true;
  filter["weather"][0]["icon"] = true;

  JsonDocument doc;
  DeserializationError err =
      deserializeJson(doc, body, DeserializationOption::Filter(filter));
  if (err) {
    errorMessage = "WX: JSON " + String(err.c_str());
    Serial.printf("[weather] JSON parse error: %s\n", err.c_str());
    return false;
  }

  const float temp = doc["main"]["temp"] | -999.0F;
  if (temp < kMinTempC || temp > kMaxTempC) {
    errorMessage = "WX: temp out of range";
    Serial.printf("[weather] Temperature %.1f°C outside valid range\n", temp);
    return false;
  }

  const char *desc = doc["weather"][0]["description"] | "";
  const char *icon = doc["weather"][0]["icon"] | "";

  data.tempCelsius = temp;
  data.description = String(desc).substring(0, 32);
  data.description.toLowerCase();
  // Capitalise first letter for display.
  if (!data.description.isEmpty()) {
    data.description.setCharAt(0,
                               toUpperCase(data.description.charAt(0)));
  }
  data.iconCode = String(icon).substring(0, 3);
  data.valid = true;
  data.fetchedAtMs = millis();
  return true;
}

bool WeatherClient::isStale(const WeatherData &data, uint32_t refreshSeconds) {
  if (!data.valid) return true;
  return (millis() - data.fetchedAtMs) >= refreshSeconds * 1000UL;
}
