#include "settings_server.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <WiFi.h>

namespace {

// ---------------------------------------------------------------------------
// HTML helpers
// ---------------------------------------------------------------------------

String htmlEscape(const String &value) {
  String out;
  out.reserve(value.length());
  for (size_t i = 0; i < value.length(); ++i) {
    const char c = value.charAt(i);
    if (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else out += c;
  }
  return out;
}

String selectedIf(bool condition) {
  return condition ? F(" selected") : String();
}

// ---------------------------------------------------------------------------
// Form parsing helpers (same pattern as provisioning_portal.cpp)
// ---------------------------------------------------------------------------

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
  out = static_cast<uint32_t>(parsed);
  return true;
}

// ---------------------------------------------------------------------------
// Settings page HTML
// ---------------------------------------------------------------------------

String settingsHtml(const AppConfig &cfg, const String &message = String(),
                    bool success = false) {
  String html;
  html.reserve(5000);

  html += F("<!doctype html><html><head>"
            "<meta name='viewport' content='width=device-width,initial-scale=1'>"
            "<title>Flight Tracker – Settings</title>"
            "<style>"
            "body{font-family:sans-serif;max-width:42rem;margin:1rem auto;padding:0 1rem}"
            "h1{margin-bottom:.25rem}h2{margin-top:1.5rem;margin-bottom:.5rem;font-size:1rem;color:#444}"
            "label{display:block;margin-top:.7rem}input,select{width:100%;padding:.5rem;box-sizing:border-box}"
            "button{margin-top:1rem;padding:.7rem 1rem}"
            "small{color:#555}.ok{color:#146c2e}.err{color:#a50e0e}"
            "fieldset{margin-top:1rem;padding:.75rem}"
            ".status{background:#f4f4f4;border-radius:.5rem;padding:1rem;margin-bottom:1rem}"
            ".status p{margin:.3rem 0}"
            "a.btn{display:inline-block;margin-top:1rem;padding:.5rem .9rem;border:1px solid #900;color:#900;border-radius:.3rem;text-decoration:none}"
            "</style></head><body>");

  html += F("<h1>Flight Tracker Settings</h1>");

  // Status box
  html += F("<div class='status'>"
            "<h2 style='margin-top:0'>Device Status</h2>"
            "<p id='s_wifi'>Loading…</p>"
            "<p id='s_ip'></p>"
            "<p id='s_source'></p>"
            "<p id='s_valid'></p>"
            "</div>"
            "<script>"
            "fetch('/status').then(r=>r.json()).then(d=>{"
            "document.getElementById('s_wifi').textContent='Wi-Fi: '+d.wifi_state;"
            "document.getElementById('s_ip').textContent='IP: '+d.ip_address;"
            "document.getElementById('s_source').textContent='Last refresh: '+d.last_refresh;"
            "document.getElementById('s_valid').textContent='Config valid: '+(d.config_valid?'yes':'no');"
            "}).catch(()=>{"
            "document.getElementById('s_wifi').textContent='Status unavailable';"
            "});"
            "</script>");

  if (!message.isEmpty()) {
    html += "<p class='";
    html += success ? "ok" : "err";
    html += "'>";
    html += htmlEscape(message);
    html += "</p>";
  }

  html += F("<form method='post' action='/save'>");

  // Wi-Fi
  html += F("<h2>Wi-Fi</h2>");
  html += F("<label>SSID<input name='ssid' required maxlength='32' value='");
  html += htmlEscape(cfg.wifiSsid);
  html += F("'></label>");
  html += F("<label>Password<input name='password' type='password' required maxlength='63' value='");
  html += htmlEscape(cfg.wifiPassword);
  html += F("'></label>");

  // Location
  html += F("<h2>Location</h2>");
  html += F("<label>Latitude<input name='latitude' type='number' step='any' min='-90' max='90' required value='");
  html += String(cfg.latitude, 6);
  html += F("'></label>");
  html += F("<label>Longitude<input name='longitude' type='number' step='any' min='-180' max='180' required value='");
  html += String(cfg.longitude, 6);
  html += F("'></label>");
  html += F("<label>Radius (km, 1–150)<input name='radius_km' type='number' step='0.1' min='1' max='150' required value='");
  html += String(cfg.radiusKm, 1);
  html += F("'></label>");

  // Display
  html += F("<h2>Display</h2>");
  html += F("<label>Brightness (0–255)<input name='brightness' type='number' min='0' max='255' required value='");
  html += String(cfg.brightness);
  html += F("'></label>");
  html += F("<label>Rotation<select name='rotate_180'>"
            "<option value='0'");
  html += selectedIf(!cfg.rotate180);
  html += F(">Normal</option><option value='1'");
  html += selectedIf(cfg.rotate180);
  html += F(">Rotate 180°</option></select></label>");
  html += F("<label>Flight screen seconds (2–60)<input name='flight_screen_seconds' type='number' min='2' max='60' required value='");
  html += String(cfg.flightScreenSeconds);
  html += F("'></label>");
  html += F("<label>Distance unit<select name='distance_unit'>"
            "<option value='km'");
  html += selectedIf(cfg.distanceUnit == "km");
  html += F(">km</option><option value='mi'");
  html += selectedIf(cfg.distanceUnit == "mi");
  html += F(">mi</option></select></label>");
  html += F("<label>Altitude unit<select name='altitude_unit'>"
            "<option value='ft'");
  html += selectedIf(cfg.altitudeUnit == "ft");
  html += F(">ft</option><option value='m'");
  html += selectedIf(cfg.altitudeUnit == "m");
  html += F(">m</option></select></label>");
  html += F("<label>Speed unit<select name='speed_unit'>"
            "<option value='kt'");
  html += selectedIf(cfg.speedUnit == "kt");
  html += F(">kt</option><option value='kmh'");
  html += selectedIf(cfg.speedUnit == "kmh");
  html += F(">kmh</option><option value='mph'");
  html += selectedIf(cfg.speedUnit == "mph");
  html += F(">mph</option></select></label>");
  html += F("<label>Clock format<select name='clock_24_hour'>"
            "<option value='1'");
  html += selectedIf(cfg.clock24Hour);
  html += F(">24-hour</option><option value='0'");
  html += selectedIf(!cfg.clock24Hour);
  html += F(">12-hour</option></select></label>");

  // Altitude filters
  html += F("<h2>Altitude Filters</h2>");
  html += F("<small>Use 0 to disable the filter threshold.</small>");
  html += F("<label>Minimum altitude (ft)<input name='min_altitude_ft' type='number' min='0' max='60000' value='");
  html += String(cfg.minAltitudeFeet);
  html += F("'></label>");
  html += F("<label>Maximum altitude (ft)<input name='max_altitude_ft' type='number' min='0' max='60000' value='");
  html += String(cfg.maxAltitudeFeet);
  html += F("'></label>");

  // FR24
  html += F("<h2>FlightRadar24</h2>");
  html += F("<label>Poll interval (seconds, 30–3600)<input name='poll_interval_seconds' type='number' min='30' max='3600' required value='");
  html += String(cfg.pollIntervalSeconds);
  html += F("'></label>");

  // Brightness schedule
  html += F("<fieldset><legend>Scheduled Brightness</legend>");
  html += F("<label>Enable schedule<select name='sched_enabled'>"
            "<option value='0'");
  html += selectedIf(!cfg.scheduleEnabled);
  html += F(">Off (use fixed brightness)</option><option value='1'");
  html += selectedIf(cfg.scheduleEnabled);
  html += F(">On</option></select></label>");
  html += F("<label>Day brightness (0–255)<input name='day_brightness' type='number' min='0' max='255' value='");
  html += String(cfg.dayBrightness);
  html += F("'></label>");
  html += F("<label>Night brightness (0–255)<input name='night_brightness' type='number' min='0' max='255' value='");
  html += String(cfg.nightBrightness);
  html += F("'></label>");
  html += F("<label>Day start (minutes past midnight, 0–1439)<input name='day_start_hhmm' type='number' min='0' max='1439' value='");
  html += String(cfg.dayStartHhmm);
  html += F("'></label>");
  html += F("<label>Night start (minutes past midnight, 0–1439)<input name='night_start_hhmm' type='number' min='0' max='1439' value='");
  html += String(cfg.nightStartHhmm);
  html += F("'></label>"
            "<small>Example: 06:00 = 360, 22:00 = 1320.</small>"
            "</fieldset>");

  html += F("<button type='submit'>Save and restart</button></form>");

  // Factory reset
  html += F("<hr><h2>Factory Reset</h2>"
            "<p><small>Erases the saved configuration and restarts into provisioning mode."
            " Use this if the device is unreachable or the configuration is corrupted.</small></p>"
            "<form method='post' action='/reset' onsubmit=\"return confirm('Erase configuration and enter setup mode?')\">"
            "<button class='btn' type='submit'>Reset to factory defaults</button>"
            "</form>");

  html += F("</body></html>");
  return html;
}

}  // namespace

// ---------------------------------------------------------------------------
// SettingsServer public API
// ---------------------------------------------------------------------------

void SettingsServer::begin(AppConfig &config, const char *sourceStatus) {
  if (running_) return;

  // GET / – settings form
  server_.on("/", HTTP_GET, [&config](AsyncWebServerRequest *request) {
    request->send(200, "text/html; charset=utf-8", settingsHtml(config));
  });

  // GET /status – JSON status
  server_.on("/status", HTTP_GET, [&config, sourceStatus](AsyncWebServerRequest *request) {
    JsonDocument doc;
    const bool connected = (WiFi.status() == WL_CONNECTED);
    doc["wifi_state"] = connected ? "connected" : "disconnected";
    doc["ip_address"] = connected ? WiFi.localIP().toString() : String("N/A");
    doc["last_refresh"] = sourceStatus ? String(sourceStatus) : String("N/A");
    doc["config_valid"] = (!config.wifiSsid.isEmpty());

    String json;
    serializeJson(doc, json);
    request->send(200, "application/json", json);
  });

  // POST /save – update configuration and restart
  server_.on("/save", HTTP_POST, [&config](AsyncWebServerRequest *request) {
    // Required fields
    if (!request->hasParam("ssid", true) || !request->hasParam("password", true)) {
      request->send(200, "text/html; charset=utf-8",
                    settingsHtml(config, "Missing required fields."));
      return;
    }

    const String ssid = request->getParam("ssid", true)->value();
    const String password = request->getParam("password", true)->value();
    const String rotateRaw =
        request->hasParam("rotate_180", true)
            ? request->getParam("rotate_180", true)->value()
            : "0";
    const String clock24Raw =
        request->hasParam("clock_24_hour", true)
            ? request->getParam("clock_24_hour", true)->value()
            : "1";
    const String distUnit =
        request->hasParam("distance_unit", true)
            ? request->getParam("distance_unit", true)->value()
            : "km";
    const String altUnit =
        request->hasParam("altitude_unit", true)
            ? request->getParam("altitude_unit", true)->value()
            : "ft";
    const String spdUnit =
        request->hasParam("speed_unit", true)
            ? request->getParam("speed_unit", true)->value()
            : "kt";

    float latitude = config.latitude;
    float longitude = config.longitude;
    float radiusKm = config.radiusKm;
    uint32_t brightness = config.brightness;
    uint32_t flightScreenSeconds = config.flightScreenSeconds;
    uint32_t minAlt = static_cast<uint32_t>(config.minAltitudeFeet);
    uint32_t maxAlt = static_cast<uint32_t>(config.maxAltitudeFeet);
    uint32_t pollInterval = config.pollIntervalSeconds;

    if (ssid.isEmpty() || password.isEmpty() || ssid.length() > 32 || password.length() > 63 ||
        (rotateRaw != "0" && rotateRaw != "1") ||
        (clock24Raw != "0" && clock24Raw != "1") ||
        (distUnit != "km" && distUnit != "mi") ||
        (altUnit != "ft" && altUnit != "m") ||
        (spdUnit != "kt" && spdUnit != "kmh" && spdUnit != "mph") ||
        !parseFloatParam(request, "latitude", latitude) ||
        !parseFloatParam(request, "longitude", longitude) ||
        !parseFloatParam(request, "radius_km", radiusKm) ||
        !parseUintParam(request, "brightness", brightness) || brightness > 255 ||
        !parseUintParam(request, "flight_screen_seconds", flightScreenSeconds) ||
        !parseUintParam(request, "min_altitude_ft", minAlt) ||
        !parseUintParam(request, "max_altitude_ft", maxAlt) ||
        !parseUintParam(request, "poll_interval_seconds", pollInterval)) {
      request->send(200, "text/html; charset=utf-8",
                    settingsHtml(config, "Invalid values. Please check all fields."));
      return;
    }

    AppConfig next = config;
    next.wifiSsid = ssid;
    next.wifiPassword = password;
    next.latitude = latitude;
    next.longitude = longitude;
    next.radiusKm = radiusKm;
    next.brightness = static_cast<uint8_t>(brightness);
    next.rotate180 = (rotateRaw == "1");
    next.clock24Hour = (clock24Raw == "1");
    next.flightScreenSeconds = flightScreenSeconds;
    next.distanceUnit = distUnit;
    next.altitudeUnit = altUnit;
    next.speedUnit = spdUnit;
    next.minAltitudeFeet = static_cast<int>(minAlt);
    next.maxAltitudeFeet = static_cast<int>(maxAlt);
    next.pollIntervalSeconds = pollInterval;

    // Scheduled brightness (optional)
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
          !parseUintParam(request, "night_start_hhmm", nightStart) || nightStart >= 1440 ||
          dayStart == nightStart) {
        request->send(200, "text/html; charset=utf-8",
                      settingsHtml(config,
                                   "Invalid schedule values. Check brightness (0–255) and "
                                   "start times (0–1439, must differ)."));
        return;
      }
      next.dayBrightness = static_cast<uint8_t>(dayBr);
      next.nightBrightness = static_cast<uint8_t>(nightBr);
      next.dayStartHhmm = static_cast<uint16_t>(dayStart);
      next.nightStartHhmm = static_cast<uint16_t>(nightStart);
    }

    if (!next.saveAtomic()) {
      request->send(200, "text/html; charset=utf-8",
                    settingsHtml(config, "Failed to save configuration."));
      return;
    }

    config = next;
    request->send(200, "text/html; charset=utf-8",
                  settingsHtml(config, "Configuration saved. Restarting…", true));
    delay(500);
    ESP.restart();
  });

  // POST /reset – erase config and reboot into provisioning
  server_.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request) {
    Serial.println("[settings] Factory reset requested via web portal.");
    LittleFS.remove("/config.json");
    request->send(200, "text/html; charset=utf-8",
                  F("<!doctype html><html><body>"
                    "<h2>Resetting…</h2>"
                    "<p>Configuration erased. The device will restart in provisioning mode.</p>"
                    "</body></html>"));
    delay(500);
    ESP.restart();
  });

  server_.begin();
  running_ = true;
  Serial.printf("[settings] Settings server listening on http://%s/\n",
                WiFi.localIP().toString().c_str());
}

void SettingsServer::stop() {
  if (!running_) return;
  server_.end();
  running_ = false;
  Serial.println("[settings] Settings server stopped.");
}
