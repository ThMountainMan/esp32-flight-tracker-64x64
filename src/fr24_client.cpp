#include "fr24_client.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>

#include <algorithm>
#include <cmath>
#include <cstring>

namespace {
constexpr float kEarthRadiusKm = 6371.0F;
constexpr size_t kMaximumAircraft = 12;

// Helper: copy src into a fixed-size char array, always NUL-terminated.
template <size_t N>
void strncpy_safe(char (&dst)[N], const char *src) {
  strncpy(dst, src ? src : "", N - 1);
  dst[N - 1] = '\0';
}

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
  // Cache repeated sin() results to avoid redundant FP calls on the LX6 core.
  const float sinHalfLat = sinf(latitudeDelta / 2.0F);
  const float sinHalfLon = sinf(longitudeDelta / 2.0F);
  const float a = sinHalfLat * sinHalfLat +
                  cosf(latA * DEG_TO_RAD) * cosf(latB * DEG_TO_RAD) *
                  sinHalfLon * sinHalfLon;
  return kEarthRadiusKm * 2.0F * atan2f(sqrtf(a), sqrtf(1.0F - a));
}

void Fr24Client::airlineFromCallsign(const char *callsign, char (&out)[4]) {
  out[0] = '\0';
  if (!callsign || strlen(callsign) < 3) return;
  for (int i = 0; i < 3; ++i) {
    if (!isAlpha((unsigned char)callsign[i])) return;
    out[i] = toupper((unsigned char)callsign[i]);
  }
  out[3] = '\0';
}

bool Fr24Client::fetchNearby(const AppConfig &config,
                             std::vector<Aircraft> &aircraft,
                             String &errorMessage) {
  aircraft.clear();
  errorMessage = "";

  WiFiClientSecure client;
  // TODO: Replace with certificate pinning before a production deployment.
  // setInsecure() skips TLS certificate validation — acceptable for a
  // hobby device on a trusted home network, but vulnerable to MITM on
  // public Wi-Fi.  To fix, pin the FR24 root CA using
  // client.setCACert(kFr24RootCa) with the PEM stored in flash.
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

  // Use a filter document to instruct ArduinoJson to skip all keys except
  // array values (the flight rows).  This dramatically reduces peak heap
  // usage during deserialization of the potentially large FR24 feed.
  JsonDocument filter;
  filter["*"] = true;

  JsonDocument document;
  const DeserializationError jsonError =
      deserializeJson(document, http.getStream(),
                      DeserializationOption::Filter(filter));
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
    strncpy_safe(item.id, entry.key().c_str());
    item.latitude = row[1] | 0.0F;
    item.longitude = row[2] | 0.0F;
    item.distanceKm = distanceKm(config.latitude, config.longitude,
                                 item.latitude, item.longitude);
    if (item.distanceKm > config.radiusKm) continue;
    item.headingDegrees = row[3] | 0;
    item.altitudeFeet = row[4] | 0;
    if (!passesAltitudeFilter(config, item.altitudeFeet)) continue;
    item.speedKnots = row[5] | 0;

    const char *rawType = row[8] | "";
    strncpy_safe(item.type, rawType);

    const char *rawCallsign = row[16] | "";
    // Trim leading/trailing spaces in a fixed buffer.
    {
      char trimmed[9];
      strncpy_safe(trimmed, rawCallsign);
      // ltrim
      size_t start = 0;
      while (trimmed[start] == ' ') ++start;
      // rtrim
      size_t len = strlen(trimmed + start);
      while (len > 0 && trimmed[start + len - 1] == ' ') --len;
      if (len == 0) {
        strncpy_safe(item.callsign, item.id);
      } else {
        strncpy(item.callsign, trimmed + start,
                std::min(len, sizeof(item.callsign) - 1));
        item.callsign[std::min(len, sizeof(item.callsign) - 1)] = '\0';
      }
    }

    airlineFromCallsign(item.callsign, item.airlineIcao);
    aircraft.push_back(item);
    if (aircraft.size() == kMaximumAircraft) break;
  }

  std::sort(aircraft.begin(), aircraft.end(),
            [](const Aircraft &a, const Aircraft &b) {
              return a.distanceKm < b.distanceKm;
            });
  return true;
}

