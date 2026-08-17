#include <Arduino.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <cstring>
#include <time.h>

#include <vector>

#include "config.h"
#include "display.h"
#include "fr24_client.h"
#include "provisioning_portal.h"

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
ProvisioningPortal provisioningPortal;
std::vector<Aircraft> aircraft;
char sourceStatus[32] = "Starting";
uint32_t lastPollMs = 0;
uint32_t lastScreenMs = 0;
uint32_t recoveryButtonPressedAtMs = 0;
uint8_t wifiFailures = 0;
size_t selectedFlight = 0;

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
    strncpy(sourceStatus, "WiFi disconnected", sizeof(sourceStatus) - 1);
    sourceStatus[sizeof(sourceStatus) - 1] = '\0';
    return;
  }
  String error;
  if (fr24.fetchNearby(config, aircraft, error)) {
    snprintf(sourceStatus, sizeof(sourceStatus), "FR24: %u flights",
             (unsigned)aircraft.size());
    if (selectedFlight >= aircraft.size()) selectedFlight = 0;
  } else {
    strncpy(sourceStatus, error.c_str(), sizeof(sourceStatus) - 1);
    sourceStatus[sizeof(sourceStatus) - 1] = '\0';
    Serial.printf("[fr24] %s\n", sourceStatus);
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
  lastPollMs = lastScreenMs = millis();
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
  if (aircraft.empty()) {
    display.showClock(WiFi.status() == WL_CONNECTED, sourceStatus);
  } else if (now - lastScreenMs >= config.flightScreenSeconds * 1000UL) {
    display.showFlight(aircraft[selectedFlight], selectedFlight, aircraft.size());
    selectedFlight = (selectedFlight + 1) % aircraft.size();
    lastScreenMs = now;
  }
  delay(50);
}
