// display.cpp  –  HUB75 64x64 LED matrix rendering implementation
//
// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 KK-ThBer/esp32-flight-tracker-64x64 contributors

#include "display.h"

#include <Arduino.h>

#include "airlines.h"
#include "fr24_client.h"

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
  snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d", tm_info.tm_hour,
           tm_info.tm_min);

  uint16_t timeColour =
      networkOk ? matrix_->color565(0, 220, 255) : matrix_->color565(180, 180, 180);
  text(4, 8, timeColour, String(timeBuf), 2);

  char dateBuf[9];
  snprintf(dateBuf, sizeof(dateBuf), "%02d/%02d/%02d", tm_info.tm_mday,
           tm_info.tm_mon + 1, tm_info.tm_year % 100);
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
  text(18, 0, 0xFFFF, aircraft.callsign, 1);

  // -----------------------------------------------------------------------
  // Aircraft type
  // -----------------------------------------------------------------------
  text(18, 10, matrix_->color565(180, 180, 180), aircraft.type, 1);

  // -----------------------------------------------------------------------
  // Altitude and speed (rows 20-30)
  // -----------------------------------------------------------------------
  char altBuf[12];
  snprintf(altBuf, sizeof(altBuf), "%5dft", aircraft.altitudeFeet);
  text(0, 20, matrix_->color565(0, 220, 120), String(altBuf), 1);

  char spdBuf[12];
  snprintf(spdBuf, sizeof(spdBuf), "%3dkt", aircraft.speedKnots);
  text(0, 30, matrix_->color565(100, 200, 255), String(spdBuf), 1);

  // -----------------------------------------------------------------------
  // Distance
  // -----------------------------------------------------------------------
  char distBuf[12];
  snprintf(distBuf, sizeof(distBuf), "%.1fkm", aircraft.distanceKm);
  text(0, 40, matrix_->color565(255, 220, 80), String(distBuf), 1);

  // -----------------------------------------------------------------------
  // Page indicator (bottom row): e.g.  "2/5"
  // -----------------------------------------------------------------------
  String page = String(index + 1) + "/" + String(total);
  text(0, 54, matrix_->color565(120, 120, 120), page, 1);
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

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
