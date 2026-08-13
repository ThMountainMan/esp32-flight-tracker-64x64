#include "config.h"

#include <ArduinoJson.h>
#include <LittleFS.h>

bool AppConfig::load() {
  File file = LittleFS.open("/config.json", "r");
  if (!file) {
    Serial.println("[config] Missing /config.json; upload the data filesystem.");
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
