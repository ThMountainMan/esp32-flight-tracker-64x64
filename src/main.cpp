#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <time.h>

#include <vector>

#include "config.h"
#include "display.h"
#include "fr24_client.h"

namespace {
constexpr char kNtpServer[] = "pool.ntp.org";

AppConfig config;
Display display;
Fr24Client fr24;
std::vector<Aircraft> aircraft;
String sourceStatus = "Starting";
uint32_t lastPollMs = 0;
uint32_t lastScreenMs = 0;
size_t selectedFlight = 0;

bool connectWifi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(config.wifiSsid.c_str(), config.wifiPassword.c_str());
  display.showStatus("WIFI", "Connecting", 0xFFE0);
  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - startedAt < 20000) {
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
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("[system] ESP32 Flight Tracker starting");

  if (!LittleFS.begin(true) || !config.load()) {
    Serial.println("[system] LittleFS or config initialization failed");
    while (true) delay(1000);
  }
  if (!display.begin(config)) {
    while (true) delay(1000);
  }
  display.showSplash();
  delay(1000);
  connectWifi();
  refreshFlights();
  lastPollMs = lastScreenMs = millis();
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) connectWifi();

  const uint32_t now = millis();
  if (now - lastPollMs >= config.pollIntervalSeconds * 1000UL) {
    refreshFlights();
    lastPollMs = now;
  }
  if (aircraft.empty()) {
    display.showClock(WiFi.status() == WL_CONNECTED, sourceStatus);
  } else if (now - lastScreenMs >= config.flightScreenSeconds * 1000UL) {
    display.showFlight(aircraft[selectedFlight], selectedFlight, aircraft.size());
    selectedFlight = (selectedFlight + 1) % aircraft.size();
    lastScreenMs = now;
  }
  delay(50);
}
