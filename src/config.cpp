#include "config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

namespace {
bool valuesAreInRange(const AppConfig &config) {
  return !(config.latitude < -90.0F || config.latitude > 90.0F ||
           config.longitude < -180.0F || config.longitude > 180.0F ||
           config.radiusKm <= 0.0F || config.radiusKm > 150.0F ||
           config.pollIntervalSeconds < 30 || config.pollIntervalSeconds > 3600);
}
}  // namespace

bool AppConfig::load() {
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    Serial.println("[config] Missing /config.json.");
    return false;
  }

  JsonDocument document;
  const DeserializationError error = deserializeJson(document, file);
  file.close();
  if (error) {
    Serial.printf("[config] Invalid JSON: %s\n", error.c_str());
    return false;
  }

  JsonObjectConst wifi = document["wifi"];
  JsonObjectConst location = document["location"];
  JsonObjectConst display = document["display"];
  JsonObjectConst fr24 = document["fr24"];
  JsonObjectConst filters = document["filters"];
  const char *ssid = wifi["ssid"] | "";
  const char *password = wifi["password"] | "";
  if (ssid[0] == '\0' || password[0] == '\0') {
    Serial.println("[config] Wi-Fi SSID or password is empty.");
    return false;
  }

  wifiSsid = ssid;
  wifiPassword = password;
  latitude = location["latitude"] | latitude;
  longitude = location["longitude"] | longitude;
  radiusKm = location["radius_km"] | radiusKm;
  brightness = display["brightness"] | brightness;
  flightScreenSeconds = display["flight_screen_seconds"] | flightScreenSeconds;
  rotate180 = display["rotate_180"] | rotate180;
  clock24Hour = display["clock_24_hour"] | clock24Hour;
  minAltitudeFeet = filters["min_altitude_ft"] | minAltitudeFeet;
  maxAltitudeFeet = filters["max_altitude_ft"] | maxAltitudeFeet;
  pollIntervalSeconds = fr24["poll_interval_seconds"] | pollIntervalSeconds;

  distanceUnit = String(display["distance_unit"] | distanceUnit.c_str());
  altitudeUnit = String(display["altitude_unit"] | altitudeUnit.c_str());
  speedUnit = String(display["speed_unit"] | speedUnit.c_str());
  distanceUnit.toLowerCase();
  altitudeUnit.toLowerCase();
  speedUnit.toLowerCase();

  if (distanceUnit != "km" && distanceUnit != "mi") {
    Serial.printf("[config] display.distance_unit must be 'km' or 'mi' (got '%s').\n",
                  distanceUnit.c_str());
    return false;
  }
  if (altitudeUnit != "ft" && altitudeUnit != "m") {
    Serial.printf("[config] display.altitude_unit must be 'ft' or 'm' (got '%s').\n",
                  altitudeUnit.c_str());
    return false;
  }
  if (speedUnit != "kt" && speedUnit != "kmh" && speedUnit != "mph") {
    Serial.printf(
        "[config] display.speed_unit must be 'kt', 'kmh', or 'mph' (got '%s').\n",
        speedUnit.c_str());
    return false;
  }

  if (latitude < -90.0F || latitude > 90.0F || longitude < -180.0F ||
      longitude > 180.0F || radiusKm <= 0.0F || radiusKm > 150.0F ||
      pollIntervalSeconds < 30 || pollIntervalSeconds > 3600 ||
      flightScreenSeconds < 2 || flightScreenSeconds > 60 ||
      minAltitudeFeet < 0 || minAltitudeFeet > 60000 || maxAltitudeFeet < 0 ||
      maxAltitudeFeet > 60000) {
    Serial.println("[config] Configuration values are outside safe limits.");
    return false;
  }
  if (minAltitudeFeet > 0 && maxAltitudeFeet > 0 &&
      minAltitudeFeet > maxAltitudeFeet) {
    Serial.printf(
        "[config] filters.min_altitude_ft (%d) must not exceed "
        "filters.max_altitude_ft (%d).\n",
        minAltitudeFeet, maxAltitudeFeet);
    return false;
  }
  return true;
}

bool AppConfig::saveAtomic() const {
  if (wifiSsid.isEmpty() || wifiPassword.isEmpty() || !valuesAreInRange(*this)) {
    Serial.println("[config] Refusing to write invalid configuration.");
    return false;
  }

  JsonDocument document;
  document["wifi"]["ssid"] = wifiSsid;
  document["wifi"]["password"] = wifiPassword;
  document["location"]["latitude"] = latitude;
  document["location"]["longitude"] = longitude;
  document["location"]["radius_km"] = radiusKm;
  document["display"]["brightness"] = brightness;
  document["display"]["rotate_180"] = rotate180;
  document["display"]["flight_screen_seconds"] = flightScreenSeconds;
  document["display"]["clock_24_hour"] = clock24Hour;
  document["display"]["distance_unit"] = distanceUnit;
  document["display"]["altitude_unit"] = altitudeUnit;
  document["display"]["speed_unit"] = speedUnit;
  document["filters"]["min_altitude_ft"] = minAltitudeFeet;
  document["filters"]["max_altitude_ft"] = maxAltitudeFeet;
  document["fr24"]["poll_interval_seconds"] = pollIntervalSeconds;

  File file = LittleFS.open("/config.json.tmp", "w");
  if (!file) {
    Serial.println("[config] Failed to open temporary config file for writing.");
    return false;
  }
  if (serializeJson(document, file) == 0) {
    file.close();
    LittleFS.remove("/config.json.tmp");
    Serial.println("[config] Failed to write configuration JSON.");
    return false;
  }
  file.flush();
  file.close();

  LittleFS.remove("/config.json");
  if (!LittleFS.rename("/config.json.tmp", "/config.json")) {
    LittleFS.remove("/config.json.tmp");
    Serial.println("[config] Failed to atomically replace /config.json.");
    return false;
  }
  Serial.println("[config] Saved /config.json successfully.");
  return true;
}
