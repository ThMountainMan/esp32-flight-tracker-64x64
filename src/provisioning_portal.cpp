#include "provisioning_portal.h"

#include <DNSServer.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>

namespace {
String htmlEscape(const String &value) {
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    if (c == '&') {
      out += "&amp;";
    } else if (c == '<') {
      out += "&lt;";
    } else if (c == '>') {
      out += "&gt;";
    } else if (c == '"') {
      out += "&quot;";
    } else {
      out += c;
    }
  }
  return out;
}

String portalHtml(const AppConfig &config, const String &message, bool success) {
  String html;
  html.reserve(2400);
  html += F("<!doctype html><html><head><meta name='viewport' content='width=device-width,initial-scale=1'>");
  html += F("<title>Flight Tracker Setup</title><style>body{font-family:sans-serif;max-width:38rem;margin:1rem auto;padding:0 1rem;}label{display:block;margin-top:.7rem;}input,select{width:100%;padding:.5rem;}button{margin-top:1rem;padding:.7rem 1rem;}small{color:#555}.ok{color:#146c2e}.err{color:#a50e0e}</style></head><body>");
  html += F("<h1>Flight Tracker Setup</h1><p>Enter Wi-Fi and display settings, then save.</p>");
  if (!message.isEmpty()) {
    html += "<p class='";
    html += success ? "ok" : "err";
    html += "'>";
    html += htmlEscape(message);
    html += "</p>";
  }
  html += F("<form method='post' action='/save'>");
  html += F("<label>Wi-Fi SSID<input name='ssid' required maxlength='32' value='");
  html += htmlEscape(config.wifiSsid);
  html += F("'></label>");
  html += F("<label>Wi-Fi Password<input name='password' type='password' required maxlength='63' value='");
  html += htmlEscape(config.wifiPassword);
  html += F("'></label>");
  html += F("<label>Latitude<input name='latitude' type='number' step='any' min='-90' max='90' required value='");
  html += String(config.latitude, 6);
  html += F("'></label>");
  html += F("<label>Longitude<input name='longitude' type='number' step='any' min='-180' max='180' required value='");
  html += String(config.longitude, 6);
  html += F("'></label>");
  html += F("<label>Radius (km)<input name='radius_km' type='number' step='0.1' min='1' max='150' required value='");
  html += String(config.radiusKm, 1);
  html += F("'></label>");
  html += F("<label>Brightness (0-255)<input name='brightness' type='number' min='0' max='255' required value='");
  html += String(config.brightness);
  html += F("'></label>");
  html += F("<label>Rotation<select name='rotate_180'><option value='0'");
  if (!config.rotate180) html += F(" selected");
  html += F(">Normal</option><option value='1'");
  if (config.rotate180) html += F(" selected");
  html += F(">Rotate 180°</option></select></label>");
  html += F("<fieldset><legend>Scheduled Brightness</legend>");
  html += F("<label>Enable schedule<select name='sched_enabled'><option value='0'");
  if (!config.scheduleEnabled) html += F(" selected");
  html += F(">Off (use fixed brightness)</option><option value='1'");
  if (config.scheduleEnabled) html += F(" selected");
  html += F(">On</option></select></label>");
  html += F("<label>Day brightness (0-255)<input name='day_brightness' type='number' min='0' max='255' value='");
  html += String(config.dayBrightness);
  html += F("'></label>");
  html += F("<label>Night brightness (0-255)<input name='night_brightness' type='number' min='0' max='255' value='");
  html += String(config.nightBrightness);
  html += F("'></label>");
  html += F("<label>Day start (minutes past midnight, 0-1439)<input name='day_start_hhmm' type='number' min='0' max='1439' value='");
  html += String(config.dayStartHhmm);
  html += F("'></label>");
  html += F("<label>Night start (minutes past midnight, 0-1439)<input name='night_start_hhmm' type='number' min='0' max='1439' value='");
  html += String(config.nightStartHhmm);
  html += F("'></label><small>Example: 06:00 = 360, 22:00 = 1320. Requires NTP time sync.</small></fieldset>");
  html += F("<fieldset><legend>Settings Portal Password</legend>");
  html += F("<label>Password (username: admin, 4–64 chars; leave blank to use device default)"
            "<input name='settings_password' type='password' minlength='4' maxlength='64'"
            " autocomplete='new-password' placeholder='leave blank to use device default'></label>"
            "<small>The device-default password is printed to serial as <code>[settings] default password: …</code>"
            " on first boot.</small></fieldset>");
  html += F("<button type='submit'>Save and restart</button></form>");
  html += F("<p><small>After saving, the device restarts and joins your Wi-Fi network.</small></p></body></html>");
  return html;
}

bool parseFloatParam(AsyncWebServerRequest *request, const char *name, float &out) {
  if (!request->hasParam(name, true)) return false;
  const String value = request->getParam(name, true)->value();
  char *end = nullptr;
  out = strtof(value.c_str(), &end);
  return end != value.c_str() && *end == '\0';
}

bool parseUintParam(AsyncWebServerRequest *request, const char *name, uint32_t &out) {
  if (!request->hasParam(name, true)) return false;
  const String value = request->getParam(name, true)->value();
  char *end = nullptr;
  const unsigned long parsed = strtoul(value.c_str(), &end, 10);
  if (end == value.c_str() || *end != '\0') return false;
  out = parsed;
  return true;
}

void sendPortalPage(AsyncWebServerRequest *request, const AppConfig &config,
                    const String &message = String(), bool success = false) {
  request->send(200, "text/html; charset=utf-8", portalHtml(config, message, success));
}
}  // namespace

bool ProvisioningPortal::run(AppConfig &config, Display &display, uint32_t timeoutMs) {
  AppConfig candidate = config;
  DNSServer dnsServer;
  AsyncWebServer server(80);
  volatile bool shouldRestart = false;

  WiFi.disconnect(true, true);
  WiFi.mode(WIFI_AP);
  const String apSsid = "FlightTracker-Setup-" + String((uint32_t)ESP.getEfuseMac(), HEX);
  if (!WiFi.softAP(apSsid.c_str())) {
    Serial.println("[provisioning] Failed to start AP mode.");
    return false;
  }

  const IPAddress apIp = WiFi.softAPIP();
  dnsServer.start(53, "*", apIp);
  dnsServer.setErrorReplyCode(DNSReplyCode::NoError);

  Serial.printf("[provisioning] AP started: %s (%s)\n", apSsid.c_str(), apIp.toString().c_str());
  display.showStatus("SETUP", apSsid.c_str(), 0x07E0);

  auto redirectToRoot = [](AsyncWebServerRequest *request) {
    request->redirect("/");
  };

  server.on("/", HTTP_GET, [&](AsyncWebServerRequest *request) { sendPortalPage(request, candidate); });
  server.on("/generate_204", HTTP_GET, redirectToRoot);
  server.on("/gen_204", HTTP_GET, redirectToRoot);
  server.on("/hotspot-detect.html", HTTP_GET, redirectToRoot);
  server.on("/connecttest.txt", HTTP_GET, redirectToRoot);
  server.on("/fwlink", HTTP_GET, redirectToRoot);
  server.on("/ncsi.txt", HTTP_GET, redirectToRoot);

  server.on("/save", HTTP_POST, [&](AsyncWebServerRequest *request) {
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true) ||
        !request->hasParam("rotate_180", true)) {
      sendPortalPage(request, candidate, "Missing required fields.");
      return;
    }

    const String ssid = request->getParam("ssid", true)->value();
    const String password = request->getParam("password", true)->value();
    const String rotateRaw = request->getParam("rotate_180", true)->value();

    float latitude = 0.0F;
    float longitude = 0.0F;
    float radiusKm = 0.0F;
    uint32_t brightness = 0;
    if (ssid.isEmpty() || password.isEmpty() || ssid.length() > 32 || password.length() > 63 ||
        !parseFloatParam(request, "latitude", latitude) ||
        !parseFloatParam(request, "longitude", longitude) ||
        !parseFloatParam(request, "radius_km", radiusKm) ||
        !parseUintParam(request, "brightness", brightness) ||
        brightness > 255 || (rotateRaw != "0" && rotateRaw != "1")) {
      sendPortalPage(request, candidate, "Invalid values. Please check all fields.");
      return;
    }

    AppConfig next = candidate;
    next.wifiSsid = ssid;
    next.wifiPassword = password;
    next.latitude = latitude;
    next.longitude = longitude;
    next.radiusKm = radiusKm;
    next.brightness = static_cast<uint8_t>(brightness);
    next.rotate180 = rotateRaw == "1";

    // Scheduled brightness (optional fields; keep defaults if absent/invalid)
    const String schedEnabledRaw =
        request->hasParam("sched_enabled", true)
            ? request->getParam("sched_enabled", true)->value()
            : "0";
    next.scheduleEnabled = (schedEnabledRaw == "1");
    if (next.scheduleEnabled) {
      uint32_t dayBr = next.dayBrightness;
      uint32_t nightBr = next.nightBrightness;
      uint32_t dayStart = next.dayStartHhmm;
      uint32_t nightStart = next.nightStartHhmm;
      if (!parseUintParam(request, "day_brightness", dayBr) || dayBr > 255 ||
          !parseUintParam(request, "night_brightness", nightBr) || nightBr > 255 ||
          !parseUintParam(request, "day_start_hhmm", dayStart) || dayStart >= 1440 ||
          !parseUintParam(request, "night_start_hhmm", nightStart) ||
          nightStart >= 1440 || dayStart == nightStart) {
        sendPortalPage(request, candidate,
                       "Invalid schedule values. Check brightness (0-255) and "
                       "start times (0-1439, must differ).");
        return;
      }
      next.dayBrightness = static_cast<uint8_t>(dayBr);
      next.nightBrightness = static_cast<uint8_t>(nightBr);
      next.dayStartHhmm = static_cast<uint16_t>(dayStart);
      next.nightStartHhmm = static_cast<uint16_t>(nightStart);
    }
    // Settings portal password (optional; blank = keep default)
    if (request->hasParam("settings_password", true)) {
      const String pwd = request->getParam("settings_password", true)->value();
      if (!pwd.isEmpty()) {
        if (pwd.length() < 4 || pwd.length() > 64) {
          sendPortalPage(request, candidate,
                         "Settings password must be 4-64 characters.");
          return;
        }
        next.settingsPassword = pwd;
      }
    }
    if (!next.saveAtomic()) {
      sendPortalPage(request, candidate, "Failed to save configuration.");
      return;
    }

    candidate = next;
    sendPortalPage(request, candidate, "Configuration saved. Restarting...", true);
    shouldRestart = true;
  });

  server.onNotFound(redirectToRoot);
  server.begin();

  const uint32_t startedAt = millis();
  while (!shouldRestart && millis() - startedAt < timeoutMs) {
    dnsServer.processNextRequest();
    delay(10);
  }

  server.end();
  dnsServer.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);

  if (shouldRestart) {
    Serial.println("[provisioning] Configuration saved. Restarting now.");
    delay(500);
    ESP.restart();
    return true;
  }

  Serial.println("[provisioning] Timed out; leaving provisioning mode.");
  return false;
}
