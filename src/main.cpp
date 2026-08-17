#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

#include <vector>

#include "config.h"
#include "display.h"
#include "fr24_client.h"
#include "provisioning_portal.h"
#include "weather_client.h"

namespace {
constexpr uint32_t kFlightScreenIntervalMs = 5000;
constexpr uint32_t kWifiConnectTimeoutMs = 20000;
constexpr uint32_t kProvisioningTimeoutMs = 10UL * 60UL * 1000UL;
constexpr uint8_t kMaxWifiFailuresBeforeProvisioning = 3;
constexpr uint8_t kRecoveryButtonPin = 0;
constexpr uint32_t kRecoveryButtonHoldMs = 5000;
constexpr char kNtpServer[] = "pool.ntp.org";

AppConfig config;
Display display;
Fr24Client fr24;
WeatherClient weatherClient;
ProvisioningPortal provisioningPortal;
std::vector<Aircraft> aircraft;
WeatherData weatherData;
String sourceStatus = "Starting";
uint32_t lastPollMs = 0;
uint32_t lastScreenMs = 0;
uint32_t lastBrightnessCheckMs = 0;
uint32_t lastWeatherMs = 0;
uint32_t recoveryButtonPressedAtMs = 0;
uint8_t wifiFailures = 0;
size_t selectedFlight = 0;
// Idle scene selector: alternates clock / weather when no aircraft are visible.
bool idleShowWeather = false;

bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  display.showStatus("WIFI", "Connecting", 0xFFE0);
  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < kWifiConnectTimeoutMs) {
    delay(250);
  }
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[wifi] Connection timed out");
    display.showStatus("WIFI", "Failed", 0xF800);
    return false;
  }
  Serial.printf("[wifi] Connected: %s\n", WiFi.localIP().toString().c_str());
  configTime(0, 0, kNtpServer);
  return true;
}

void startProvisioning(bool hasFallbackConfig) {
  Serial.println("[provisioning] Entering provisioning mode.");
  const bool completed = provisioningPortal.run(config, display, kProvisioningTimeoutMs);
  if (!completed && !hasFallbackConfig) {
    Serial.println("[provisioning] No valid config available; restarting provisioning mode.");
    delay(500);
    ESP.restart();
  }
}

void refreshFlights() {
  if (WiFi.status() != WL_CONNECTED) {
    sourceStatus = "WiFi disconnected";
    return;
  }
  String error;
  if (fr24.fetchNearby(config, aircraft, error)) {
    sourceStatus = "FR24: " + String(aircraft.size()) + " flights";
    if (selectedFlight >= aircraft.size()) selectedFlight = 0;
  } else {
    sourceStatus = error;
    Serial.printf("[fr24] %s\n", error.c_str());
  }
}

void refreshWeather() {
  if (!config.weatherEnabled) return;
  if (WiFi.status() != WL_CONNECTED) return;
  String error;
  if (!weatherClient.fetch(config, weatherData, error)) {
    Serial.printf("[weather] %s\n", error.c_str());
  }
}
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("[system] ESP32 Flight Tracker starting");

  if (!LittleFS.begin(true)) {
    Serial.println("[system] LittleFS initialization failed");
    while (true) delay(1000);
  }
  const bool hasValidConfig = config.load();
  if (!display.begin(config)) {
    while (true) delay(1000);
  }
  pinMode(kRecoveryButtonPin, INPUT_PULLUP);
  display.showSplash();
  delay(1000);

  if (!hasValidConfig) {
    Serial.println("[system] No valid config found; starting first-boot provisioning.");
    startProvisioning(false);
  }

  if (!connectWifi()) {
    wifiFailures = 1;
    startProvisioning(true);
    connectWifi();
  } else {
    wifiFailures = 0;
  }
  refreshFlights();
  if (config.weatherEnabled) refreshWeather();
  display.applyScheduledBrightness(config);
  lastPollMs = lastScreenMs = lastBrightnessCheckMs = lastWeatherMs = millis();
}

void loop() {
  if (digitalRead(kRecoveryButtonPin) == LOW) {
    if (recoveryButtonPressedAtMs == 0) {
      recoveryButtonPressedAtMs = millis();
    } else if (millis() - recoveryButtonPressedAtMs >= kRecoveryButtonHoldMs) {
      Serial.println("[provisioning] Recovery button held; starting provisioning mode.");
      display.showStatus("SETUP", "Recovery mode", 0xFFE0);
      startProvisioning(true);
      recoveryButtonPressedAtMs = 0;
    }
  } else {
    recoveryButtonPressedAtMs = 0;
  }

  if (WiFi.status() != WL_CONNECTED) {
    if (!connectWifi()) {
      if (wifiFailures < 255) ++wifiFailures;
      if (wifiFailures >= kMaxWifiFailuresBeforeProvisioning) {
        Serial.println("[wifi] Repeated connection failures; starting provisioning.");
        startProvisioning(true);
        wifiFailures = 0;
      }
      delay(250);
      return;
    }
    wifiFailures = 0;
  }

  const uint32_t now = millis();
  if (now - lastPollMs >= config.pollIntervalSeconds * 1000UL) {
    refreshFlights();
    lastPollMs = now;
  }
  // Weather refresh (independent of flight poll).
  if (config.weatherEnabled &&
      (now - lastWeatherMs >= config.weatherRefreshSeconds * 1000UL ||
       lastWeatherMs == 0)) {
    refreshWeather();
    lastWeatherMs = now;
  }
  // Check scheduled brightness once per minute (non-disruptive to refresh).
  if (now - lastBrightnessCheckMs >= 60000UL || lastBrightnessCheckMs == 0) {
    display.applyScheduledBrightness(config);
    lastBrightnessCheckMs = now;
  }
  if (aircraft.empty()) {
    // Idle scenes: alternate clock and weather (when enabled and valid).
    // Each idle scene is held for flightScreenSeconds so the switch rate
    // matches the flight-carousel interval.
    if (now - lastScreenMs >= config.flightScreenSeconds * 1000UL || lastScreenMs == 0) {
      if (config.weatherEnabled && idleShowWeather) {
        // Temperature unit: show °F when distance unit is "mi" (US locale).
        const bool tempInCelsius = (config.distanceUnit != "mi");
        display.showWeather(weatherData, tempInCelsius, sourceStatus);
      } else {
        display.showClock(WiFi.status() == WL_CONNECTED, sourceStatus);
      }
      if (config.weatherEnabled) idleShowWeather = !idleShowWeather;
      lastScreenMs = now;
    }
  } else if (now - lastScreenMs >= config.flightScreenSeconds * 1000UL) {
    display.showFlight(aircraft[selectedFlight], selectedFlight, aircraft.size());
    selectedFlight = (selectedFlight + 1) % aircraft.size();
    lastScreenMs = now;
    idleShowWeather = false;  // Reset idle scene on transition back to idle.
  }
  delay(50);
}
