#include "route_lookup.h"

#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <cstring>

namespace {
// Helper: copy src into a fixed-size char array, always NUL-terminated.
template <size_t N>
void strncpy_safe(char (&dst)[N], const char *src) {
  strncpy(dst, src ? src : "", N - 1);
  dst[N - 1] = '\0';
}

/** Replace all occurrences of `placeholder` in `tmpl` with `value`. */
String buildUrl(const String &tmpl, const char *callsign) {
  String url = tmpl;
  url.replace("{callsign}", callsign);
  return url;
}

/** Convert a raw ICAO airport code to upper-case in-place. */
void toUppercase(char *buf) {
  for (; *buf; ++buf) *buf = toupper((unsigned char)*buf);
}
}  // namespace

void RouteLookup::resetStates(std::vector<Aircraft> &aircraft) {
  for (Aircraft &a : aircraft) {
    a.routeState = RouteState::Unknown;
    a.origin[0] = '\0';
    a.destination[0] = '\0';
  }
}

void RouteLookup::tick(const AppConfig &config,
                       std::vector<Aircraft> &aircraft) {
  if (!config.routeLookupEnabled || config.routeLookupUrl.isEmpty()) return;

  const uint32_t now = millis();
  // Reset the per-interval counter once the window has elapsed.
  if (now - lastLookupMs_ >= kLookupIntervalMs) {
    lookupsThisInterval_ = 0;
  }

  for (Aircraft &a : aircraft) {
    if (a.callsign[0] == '\0') continue;
    if (a.routeState != RouteState::Unknown) continue;

    // Try the cache first (no rate limiting needed).
    if (cache_.lookup(a.callsign, a.origin, a.destination)) {
      a.routeState = RouteState::Resolved;
      continue;
    }

    // Rate-limit remote lookups.
    if (lookupsThisInterval_ >= kMaxLookupsPerInterval) break;
    if (now - lastLookupMs_ < kLookupIntervalMs && lastLookupMs_ != 0) break;

    a.routeState = RouteState::Pending;
    const bool ok = performLookup(config, a);
    lastLookupMs_ = millis();
    ++lookupsThisInterval_;

    if (ok) {
      cache_.store(a.callsign, a.origin, a.destination);
      a.routeState = RouteState::Resolved;
    } else {
      a.routeState = RouteState::Failed;
    }
    break;  // At most one lookup per tick call.
  }
}

bool RouteLookup::performLookup(const AppConfig &config, Aircraft &aircraft) {
  if (aircraft.callsign[0] == '\0') return false;

  const String url = buildUrl(config.routeLookupUrl, aircraft.callsign);

  WiFiClientSecure client;
  // setInsecure() skips certificate validation – consistent with the
  // existing FR24 client approach for this hobby device.  Users running
  // a local tar1090 instance over plain HTTP will configure an http:// URL.
  client.setInsecure();

  HTTPClient http;
  http.setConnectTimeout(5000);
  http.setTimeout(8000);
  if (!http.begin(client, url)) {
    Serial.printf("[route] Could not begin request for %s\n",
                  aircraft.callsign);
    return false;
  }
  http.addHeader("User-Agent", "ESP32-FlightTracker/0.1");
  const int status = http.GET();
  if (status != HTTP_CODE_OK) {
    Serial.printf("[route] HTTP %d for %s\n", status, aircraft.callsign);
    http.end();
    return false;
  }

  const String body = http.getString();
  http.end();

  if (!parseRouteResponse(body, aircraft.origin, aircraft.destination)) {
    Serial.printf("[route] Parse failed for %s\n", aircraft.callsign);
    return false;
  }
  Serial.printf("[route] Resolved %s → %s/%s\n", aircraft.callsign,
                aircraft.origin, aircraft.destination);
  return true;
}

// ---------------------------------------------------------------------------
// Response parsing
//
// The lookup endpoint is expected to return a JSON object with at least:
//   { "origin": "EGLL", "destination": "KJFK" }
//
// Both tar1090/dump1090-fa and common flight-info proxy APIs provide at least
// these keys.  Unknown keys are silently ignored.
// ---------------------------------------------------------------------------
bool RouteLookup::parseRouteResponse(const String &body, char (&origin)[5],
                                     char (&destination)[5]) {
  JsonDocument doc;
  if (deserializeJson(doc, body)) return false;

  const char *org = doc["origin"] | "";
  const char *dst = doc["destination"] | "";

  // Validate: 3-4 alphanumeric characters.
  auto valid = [](const char *code) -> bool {
    if (!code) return false;
    const size_t len = strlen(code);
    if (len < 3 || len > 4) return false;
    for (size_t i = 0; i < len; ++i) {
      if (!isAlphaNumeric((unsigned char)code[i])) return false;
    }
    return true;
  };

  if (!valid(org) || !valid(dst)) return false;

  strncpy_safe(origin, org);
  strncpy_safe(destination, dst);
  toUppercase(origin);
  toUppercase(destination);
  return true;
}
