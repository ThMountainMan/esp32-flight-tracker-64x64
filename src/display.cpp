// display.cpp  –  HUB75 64x64 LED matrix rendering implementation
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 KK-ThBer/esp32-flight-tracker-64x64 contributors

#include "display.h"

#include <Arduino.h>

#include "airlines.h"
#include "fr24_client.h"

namespace {
constexpr float kKmToMiles = 0.621371F;
constexpr float kFeetToMeters = 0.3048F;
constexpr float kKnotsToKmh = 1.852F;
constexpr float kKnotsToMph = 1.150779F;
}

// ---------------------------------------------------------------------------
// Panel pin defaults (override via platformio.ini build_flags if needed)
// ---------------------------------------------------------------------------
#ifndef HUB75_R1_PIN
#define HUB75_R1_PIN 25
#endif
#ifndef HUB75_G1_PIN
#define HUB75_G1_PIN 26
#endif
#ifndef HUB75_B1_PIN
#define HUB75_B1_PIN 27
#endif
#ifndef HUB75_R2_PIN
#define HUB75_R2_PIN 14
#endif
#ifndef HUB75_G2_PIN
#define HUB75_G2_PIN 12
#endif
#ifndef HUB75_B2_PIN
#define HUB75_B2_PIN 13
#endif
#ifndef HUB75_A_PIN
#define HUB75_A_PIN 23
#endif
#ifndef HUB75_B_PIN
#define HUB75_B_PIN 19
#endif
#ifndef HUB75_C_PIN
#define HUB75_C_PIN 5
#endif
#ifndef HUB75_D_PIN
#define HUB75_D_PIN 17
#endif
#ifndef HUB75_E_PIN
#define HUB75_E_PIN 18
#endif
#ifndef HUB75_LAT_PIN
#define HUB75_LAT_PIN 4
#endif
#ifndef HUB75_OE_PIN
#define HUB75_OE_PIN 15
#endif
#ifndef HUB75_CLK_PIN
#define HUB75_CLK_PIN 16
#endif

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

bool Display::begin(const AppConfig &config) {
  HUB75_I2S_CFG mxconfig(MATRIX_WIDTH, MATRIX_HEIGHT, 1 /* chain */);
  mxconfig.gpio.r1 = HUB75_R1_PIN;
  mxconfig.gpio.g1 = HUB75_G1_PIN;
  mxconfig.gpio.b1 = HUB75_B1_PIN;
  mxconfig.gpio.r2 = HUB75_R2_PIN;
  mxconfig.gpio.g2 = HUB75_G2_PIN;
  mxconfig.gpio.b2 = HUB75_B2_PIN;
  mxconfig.gpio.a = HUB75_A_PIN;
  mxconfig.gpio.b = HUB75_B_PIN;
  mxconfig.gpio.c = HUB75_C_PIN;
  mxconfig.gpio.d = HUB75_D_PIN;
  mxconfig.gpio.e = HUB75_E_PIN;
  mxconfig.gpio.lat = HUB75_LAT_PIN;
  mxconfig.gpio.oe = HUB75_OE_PIN;
  mxconfig.gpio.clk = HUB75_CLK_PIN;

  rotate180_ = config.rotate180;
  clock24Hour_ = config.clock24Hour;
  distanceUnit_ =
      (config.distanceUnit == "mi") ? DistanceUnit::Mi : DistanceUnit::Km;
  altitudeUnit_ =
      (config.altitudeUnit == "m") ? AltitudeUnit::Meters : AltitudeUnit::Feet;
  if (config.speedUnit == "kmh") {
    speedUnit_ = SpeedUnit::Kmh;
  } else if (config.speedUnit == "mph") {
    speedUnit_ = SpeedUnit::Mph;
  } else {
    speedUnit_ = SpeedUnit::Knots;
  }

  matrix_ = new MatrixPanel_I2S_DMA(mxconfig);
  if (!matrix_->begin()) {
    Serial.println("[display] MatrixPanel init failed");
    return false;
  }
  matrix_->setBrightness8(config.brightness);
  if (rotate180_) matrix_->setRotation(2);  // 180-degree flip via GFX
  matrix_->clearScreen();
  return true;
}

void Display::showSplash() {
  if (!matrix_) return;
  clear();
  // Simple splash: project name centred on the panel
  matrix_->setTextColor(matrix_->color565(0, 180, 255));
  text(4, 10, matrix_->color565(0, 180, 255), "FLIGHT");
  text(4, 22, matrix_->color565(255, 200, 0), "TRACKER");
  text(8, 34, matrix_->color565(200, 200, 200), "64x64");
}

void Display::showStatus(const char *title, const String &detail,
                         uint16_t colour) {
  if (!matrix_) return;
  clear();
  text(0, 2, colour, String(title), 1);
  text(0, 14, 0xFFFF, detail, 1);
}

void Display::showClock(bool networkOk, const String &sourceStatus) {
  if (!matrix_) return;
  clear();

  time_t now = time(nullptr);
  struct tm tm_info;
  localtime_r(&now, &tm_info);

  char timeBuf[6];
  if (clock24Hour_) {
    snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", tm_info.tm_hour,
             tm_info.tm_min);
  } else {
    int hour12 = tm_info.tm_hour % 12;
    if (hour12 == 0) hour12 = 12;
    snprintf(timeBuf, sizeof(timeBuf), "%2d:%02d", hour12, tm_info.tm_min);
  }

  uint16_t timeColour =
      networkOk ? matrix_->color565(0, 220, 255) : matrix_->color565(180, 180, 180);
  text(4, 8, timeColour, String(timeBuf), 2);
  if (!clock24Hour_) {
    text(50, 24, matrix_->color565(180, 180, 180),
         (tm_info.tm_hour >= 12) ? "PM" : "AM", 1);
  }

  // strftime formats struct tm fields directly and avoids snprintf truncation
  // warnings from conservative integer-range analysis.
  char dateBuf[9] = {};
  if (strftime(dateBuf, sizeof(dateBuf), "%d/%m/%y", &tm_info) == 0) {
    snprintf(dateBuf, sizeof(dateBuf), "--/--/--");
  }
  text(0, 30, 0xFFFF, String(dateBuf), 1);

  // Truncate status to fit one line at scale-1 (6px per char, 64px wide = 10 chars)
  String status = sourceStatus;
  if (status.length() > 10) status = status.substring(0, 10);
  text(0, 44, matrix_->color565(180, 180, 0), status, 1);
}

void Display::showFlight(const Aircraft &aircraft, size_t index,
                         size_t total) {
  if (!matrix_) return;
  clear();

  // -----------------------------------------------------------------------
  // Airline icon area (top-left 16x16 block, x=0..15, y=0..15)
  //
  // If a 16x16 RGB565 glyph is registered for this airline ICAO, render it
  // using drawRGBBitmap.  Otherwise fall back to the two-letter text badge
  // with the airline's representative colour as a background fill.
  // -----------------------------------------------------------------------
  const uint16_t *icon = airlineIcon(aircraft.airlineIcao);
  if (icon != nullptr) {
    // drawRGBBitmap copies from RAM; read from PROGMEM into a small stack
    // buffer first (256 * 2 = 512 bytes – acceptable on ESP32).
    uint16_t iconBuf[kIconSize * kIconSize];
    memcpy(iconBuf, icon, sizeof(iconBuf));
    matrix_->drawRGBBitmap(0, 0, iconBuf, kIconSize, kIconSize);
  } else {
    // Fallback: coloured badge with 2-letter initials
    uint16_t bg = airlineColour(aircraft.airlineIcao);
    fillRect(0, 0, kIconSize, kIconSize, bg);
    String badge = airlineBadgeText(aircraft.airlineIcao);
    // Contrasting colour: white unless bg is very bright
    uint16_t fg = (bg > 0xC618) ? 0x0000 : 0xFFFF;
    text(1, 4, fg, badge, 1);
  }

  // -----------------------------------------------------------------------
  // Callsign / flight number (top-right area, starting at x=18)
  // -----------------------------------------------------------------------
  String callsign = aircraft.callsign;
  if (callsign.length() > 7) callsign = callsign.substring(0, 7);
  text(18, 0, 0xFFFF, callsign, 1);

  // -----------------------------------------------------------------------
  // Aircraft type
  // -----------------------------------------------------------------------
  String type = aircraft.type;
  if (type.length() > 7) type = type.substring(0, 7);
  text(18, 10, matrix_->color565(180, 180, 180), type, 1);

  // -----------------------------------------------------------------------
  // Altitude and speed (rows 20-30)
  // -----------------------------------------------------------------------
  int altitudeValue = aircraft.altitudeFeet;
  const char *altitudeSuffix = "ft";
  if (altitudeUnit_ == AltitudeUnit::Meters) {
    altitudeValue =
        static_cast<int>(aircraft.altitudeFeet * kFeetToMeters + 0.5F);
    altitudeSuffix = "m";
  }
  char altBuf[12];
  snprintf(altBuf, sizeof(altBuf), "A%d%s", altitudeValue, altitudeSuffix);
  text(0, 20, matrix_->color565(0, 220, 120), String(altBuf), 1);

  int speedValue = aircraft.speedKnots;
  const char *speedSuffix = "kt";
  if (speedUnit_ == SpeedUnit::Kmh) {
    speedValue = static_cast<int>(aircraft.speedKnots * kKnotsToKmh + 0.5F);
    speedSuffix = "kmh";
  } else if (speedUnit_ == SpeedUnit::Mph) {
    speedValue = static_cast<int>(aircraft.speedKnots * kKnotsToMph + 0.5F);
    speedSuffix = "mph";
  }
  char spdBuf[12];
  snprintf(spdBuf, sizeof(spdBuf), "S%d%s", speedValue, speedSuffix);
  text(0, 30, matrix_->color565(100, 200, 255), String(spdBuf), 1);

  char hdgBuf[8];
  snprintf(hdgBuf, sizeof(hdgBuf), "H%03d", aircraft.headingDegrees);
  text(0, 40, matrix_->color565(180, 180, 180), String(hdgBuf), 1);

  float distanceValue = aircraft.distanceKm;
  const char *distanceSuffix = "km";
  if (distanceUnit_ == DistanceUnit::Mi) {
    distanceValue = aircraft.distanceKm * kKmToMiles;
    distanceSuffix = "mi";
  }
  char distBuf[12];
  snprintf(distBuf, sizeof(distBuf), "%.1f%s", distanceValue, distanceSuffix);
  text(30, 40, matrix_->color565(255, 220, 80), String(distBuf), 1);

  // -----------------------------------------------------------------------
  // Page indicator (bottom row): e.g.  "2/5"
  // -----------------------------------------------------------------------
  String page = String(index + 1) + "/" + String(total);
  text(0, 54, matrix_->color565(120, 120, 120), page, 1);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void Display::applyScheduledBrightness(const AppConfig &config) {
  if (!matrix_) return;
  if (!config.scheduleEnabled) {
    matrix_->setBrightness8(config.brightness);
    return;
  }

  time_t now = time(nullptr);
  // If time has not been synced (time_t < some reasonable epoch), use fixed brightness.
  if (now < 1000000000L) {
    matrix_->setBrightness8(config.brightness);
    return;
  }

  struct tm tm_info;
  localtime_r(&now, &tm_info);
  const uint16_t minuteOfDay =
      static_cast<uint16_t>(tm_info.tm_hour * 60 + tm_info.tm_min);

  // Determine whether we are in the "day" window.
  // Day window: dayStartHhmm <= minuteOfDay < nightStartHhmm (wraps midnight if
  // nightStart < dayStart, e.g. day starts at 06:00, night at 22:00).
  bool isDay;
  if (config.dayStartHhmm < config.nightStartHhmm) {
    // Normal case: day is a contiguous range within one calendar day.
    isDay = (minuteOfDay >= config.dayStartHhmm &&
             minuteOfDay < config.nightStartHhmm);
  } else {
    // Wrapped case: day window spans midnight (e.g. day 22:00→06:00).
    isDay = (minuteOfDay >= config.dayStartHhmm ||
             minuteOfDay < config.nightStartHhmm);
  }

  matrix_->setBrightness8(isDay ? config.dayBrightness : config.nightBrightness);
}

void Display::clear() {
  if (matrix_) matrix_->clearScreen();
}

void Display::text(int x, int y, uint16_t colour, const String &value,
                   uint8_t size) {
  if (!matrix_) return;
  matrix_->setTextSize(size);
  matrix_->setTextColor(colour);
  matrix_->setCursor(x, y);
  matrix_->print(value);
}

void Display::fillRect(int x, int y, int width, int height, uint16_t colour) {
  if (!matrix_) return;
  matrix_->fillRect(x, y, width, height, colour);
}
