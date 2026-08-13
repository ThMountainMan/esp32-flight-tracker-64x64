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
  const char *ssid = wifi["ssid"] | "";
  const char *password = wifi["password"] | "";
  if (ssid[0] == '\0' || password[0] == '\0') {
    Serial.println("[config] Wi-Fi SSID or password is empty.");
    return false;
  }

  AppConfig parsed = *this;
  parsed.wifiSsid = ssid;
  parsed.wifiPassword = password;
  parsed.latitude = location["latitude"] | parsed.latitude;
  parsed.longitude = location["longitude"] | parsed.longitude;
  parsed.radiusKm = location["radius_km"] | parsed.radiusKm;
  parsed.brightness = display["brightness"] | parsed.brightness;
  parsed.rotate180 = display["rotate_180"] | parsed.rotate180;
  parsed.pollIntervalSeconds = fr24["poll_interval_seconds"] | parsed.pollIntervalSeconds;

  if (!valuesAreInRange(parsed)) {
    Serial.println("[config] Configuration values are outside safe limits.");
    return false;
  }
  *this = parsed;
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
