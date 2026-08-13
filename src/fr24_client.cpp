#include "fr24_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <cmath>

namespace {
constexpr float kEarthRadiusKm = 6371.0F;
constexpr size_t kMaximumAircraft = 12;

String makeFeedUrl(const AppConfig &config) {
  const float latitudeDelta = config.radiusKm / 111.0F;
  const float longitudeDelta = config.radiusKm /
      (111.0F * std::max(0.15F, cosf(config.latitude * DEG_TO_RAD)));

  char url[420];
  snprintf(url, sizeof(url),
           "https://data-cloud.flightradar24.com/zones/fcgi/feed.js?"
           "bounds=%.5f,%.5f,%.5f,%.5f&faa=1&satellite=1&mlat=1&"
           "flarm=1&adsb=1&gnd=0&air=1&vehicles=0&estimated=0&"
           "maxage=14400&gliders=0&stats=0",
           config.latitude + latitudeDelta, config.latitude - latitudeDelta,
           config.longitude - longitudeDelta, config.longitude + longitudeDelta);
  return String(url);
}

String arrayString(JsonArrayConst row, size_t index) {
  const char *value = row[index] | "";
  return String(value);
}

bool passesAltitudeFilter(const AppConfig &config, int altitudeFeet) {
  // FR24 uses 0 when altitude is unknown; keep these entries unless a future
  // ground-traffic exclusion mode is added.
  if (altitudeFeet <= 0) return true;
  if (config.minAltitudeFeet > 0 && altitudeFeet < config.minAltitudeFeet) {
    return false;
  }
  if (config.maxAltitudeFeet > 0 && altitudeFeet > config.maxAltitudeFeet) {
    return false;
  }
  return true;
}
}  // namespace

float Fr24Client::distanceKm(float latA, float lonA, float latB, float lonB) {
  const float latitudeDelta = (latB - latA) * DEG_TO_RAD;
  const float longitudeDelta = (lonB - lonA) * DEG_TO_RAD;
  const float a = sinf(latitudeDelta / 2.0F) * sinf(latitudeDelta / 2.0F) +
                  cosf(latA * DEG_TO_RAD) * cosf(latB * DEG_TO_RAD) *
                  sinf(longitudeDelta / 2.0F) * sinf(longitudeDelta / 2.0F);
  return kEarthRadiusKm * 2.0F * atan2f(sqrtf(a), sqrtf(1.0F - a));
}

String Fr24Client::airlineFromCallsign(const String &callsign) {
  if (callsign.length() < 3) return "";
  String prefix = callsign.substring(0, 3);
  for (char character : prefix) {
    if (!isAlpha(character)) return "";
  }
  prefix.toUpperCase();
  return prefix;
}

bool Fr24Client::fetchNearby(const AppConfig &config,
                             std::vector<Aircraft> &aircraft,
                             String &errorMessage) {
  aircraft.clear();
  errorMessage = "";

  WiFiClientSecure client;
  // TODO: Replace with certificate validation before a production deployment.
  client.setInsecure();
  HTTPClient http;
  http.setConnectTimeout(7000);
  http.setTimeout(12000);
  if (!http.begin(client, makeFeedUrl(config))) {
    errorMessage = "Could not start HTTPS request";
    return false;
  }
  http.addHeader("User-Agent", "ESP32-FlightTracker/0.1");
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    errorMessage = "FR24 HTTP status " + String(status);
    http.end();
    return false;
  }

  JsonDocument document;
  const DeserializationError jsonError = deserializeJson(document, http.getStream());
  http.end();
  if (jsonError) {
    errorMessage = "FR24 JSON: " + String(jsonError.c_str());
    return false;
  }

  for (JsonPairConst entry : document.as<JsonObjectConst>()) {
    if (!entry.value().is<JsonArrayConst>()) continue;
    const JsonArrayConst row = entry.value().as<JsonArrayConst>();
    // feed.js record positions: 1/2 location, 3 heading, 4 altitude,
    // 5 speed, 8 aircraft type and 16 callsign.
    if (row.size() <= 16 || row[1].isNull() || row[2].isNull()) continue;

    Aircraft item;
    item.id = entry.key().c_str();
    item.latitude = row[1] | 0.0F;
    item.longitude = row[2] | 0.0F;
    item.distanceKm = distanceKm(config.latitude, config.longitude,
                                 item.latitude, item.longitude);
    if (item.distanceKm > config.radiusKm) continue;
    item.headingDegrees = row[3] | 0;
    item.altitudeFeet = row[4] | 0;
    if (!passesAltitudeFilter(config, item.altitudeFeet)) continue;
    item.speedKnots = row[5] | 0;
    item.type = arrayString(row, 8);
    item.callsign = arrayString(row, 16);
    item.callsign.trim();
    if (item.callsign.isEmpty()) item.callsign = item.id;
    item.airlineIcao = airlineFromCallsign(item.callsign);
    aircraft.push_back(item);
    if (aircraft.size() == kMaximumAircraft) break;
  }

  std::sort(aircraft.begin(), aircraft.end(),
            [](const Aircraft &a, const Aircraft &b) {
              return a.distanceKm < b.distanceKm;
            });
  return true;
}
