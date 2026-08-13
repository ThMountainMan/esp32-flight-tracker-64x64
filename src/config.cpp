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
  rotate180 = display["rotate_180"] | rotate180;
  pollIntervalSeconds = fr24["poll_interval_seconds"] | pollIntervalSeconds;

  if (latitude < -90.0F || latitude > 90.0F || longitude < -180.0F ||
      longitude > 180.0F || radiusKm <= 0.0F || radiusKm > 150.0F ||
      pollIntervalSeconds < 30 || pollIntervalSeconds > 3600) {
    Serial.println("[config] Configuration values are outside safe limits.");
    return false;
  }
  return true;
}
