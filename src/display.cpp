/**
 * display.cpp – HUB75 64×64 panel rendering for ESP32 Flight Tracker.
 *
 * Default HUB75 GPIO mapping (ESP32 DevKit):
 *   R1=25  G1=26  B1=27
 *   R2=14  G2=12  B2=13
 *   A=23   B=22   C=21   D=19   E=-1  (for 64-row panels set E=18 if needed)
 *   LAT=4  OE=15  CLK=2
 *
 * These can be overridden in platformio.ini via build_flags (e.g. -DMATRIX_PIN_CLK=2).
 * The panel requires a separate 5 V supply; see docs/WIRING.md.
 */

#include "display.h"

#include <Arduino.h>

// ---------------------------------------------------------------------------
// Panel geometry and default pin map
// ---------------------------------------------------------------------------

#ifndef MATRIX_WIDTH
#  define MATRIX_WIDTH  64
#endif
#ifndef MATRIX_HEIGHT
#  define MATRIX_HEIGHT 64
#endif

// HUB75 default pins – override via build_flags if your board differs.
static HUB75_I2S_CFG::i2s_pins defaultPins() {
  HUB75_I2S_CFG::i2s_pins p = {
    /* r1 */ 25, /* g1 */ 26, /* b1 */ 27,
    /* r2 */ 14, /* g2 */ 12, /* b2 */ 13,
    /* a */  23, /* b */  22, /* c */  21, /* d */ 19, /* e */ -1,
    /* lat*/ 4,  /* oe */  15, /* clk*/ 2
  };
  return p;
}

// ---------------------------------------------------------------------------
// Public interface
// ---------------------------------------------------------------------------

bool Display::begin(const AppConfig &config) {
  rotate180_ = config.rotate180;

  HUB75_I2S_CFG mxcfg(MATRIX_WIDTH, MATRIX_HEIGHT, /*chains=*/1, defaultPins());
  mxcfg.double_buff = false;

  matrix_ = new MatrixPanel_I2S_DMA(mxcfg);
  if (!matrix_->begin()) {
    Serial.println("[display] MatrixPanel begin() failed");
    return false;
  }

  matrix_->setBrightness8(config.brightness);
  if (rotate180_) matrix_->setRotation(2);
  clear();
  return true;
}

// ---------------------------------------------------------------------------
// Screens
// ---------------------------------------------------------------------------

void Display::showSplash() {
  if (!matrix_) return;
  clear();
  // Centred title
  text(4, 10, matrix_->color565(0, 200, 255), "ESP32", 2);
  text(4, 28, matrix_->color565(0, 200, 255), "FLIGHT", 2);
  text(4, 46, matrix_->color565(255, 140, 0), "TRACKER", 2);
}

void Display::showStatus(const char *title, const String &detail,
                         uint16_t colour) {
  if (!matrix_) return;
  clear();
  text(2, 4,  colour,                              String(title));
  text(2, 16, matrix_->color565(200, 200, 200),   detail);
}

void Display::showClock(bool networkOk, const String &sourceStatus) {
  if (!matrix_) return;
  clear();

  // Time (top half)
  struct tm t;
  if (networkOk && getLocalTime(&t, 100)) {
    char buf[9];
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d", t.tm_hour, t.tm_min, t.tm_sec);
    text(2, 4, matrix_->color565(0, 255, 128), String(buf), 2);
  } else {
    text(2, 4, matrix_->color565(255, 80, 0), "No time");
  }

  // Status line (bottom)
  uint16_t sColour = networkOk ? matrix_->color565(180, 180, 180)
                                : matrix_->color565(255, 60, 60);
  // Trim long status to avoid overflow
  String s = sourceStatus;
  if (s.length() > 20) s = s.substring(0, 20);
  text(2, 50, sColour, s);
}

void Display::showFlight(const Aircraft &ac, size_t index, size_t total) {
  if (!matrix_) return;
  clear();

  // Airline badge (top-left 14×14 box)
  uint16_t badgeColour = airlineColour(ac.airlineIcao);
  fillRect(0, 0, 14, 14, badgeColour);
  String badge = airlineBadgeText(ac.airlineIcao);
  text(1, 3, matrix_->color565(0, 0, 0), badge);

  // Callsign
  String cs = ac.callsign.isEmpty() ? ac.id : ac.callsign;
  if (cs.length() > 8) cs = cs.substring(0, 8);
  text(16, 2, matrix_->color565(255, 255, 255), cs);

  // Aircraft type
  text(16, 12, matrix_->color565(160, 160, 160), ac.type.isEmpty() ? "----" : ac.type);

  // Altitude
  char altBuf[12];
  snprintf(altBuf, sizeof(altBuf), "%5dft", ac.altitudeFeet);
  text(2, 24, matrix_->color565(100, 200, 255), String(altBuf));

  // Speed
  char spdBuf[12];
  snprintf(spdBuf, sizeof(spdBuf), "%3dkt", ac.speedKnots);
  text(2, 33, matrix_->color565(100, 255, 100), String(spdBuf));

  // Distance
  char distBuf[12];
  snprintf(distBuf, sizeof(distBuf), "%4.1fkm", ac.distanceKm);
  text(2, 42, matrix_->color565(255, 200, 50), String(distBuf));

  // Heading arrow (simple ASCII)
  const char *arrow = headingArrow(ac.headingDegrees);
  text(48, 33, matrix_->color565(255, 255, 255), String(arrow));

  // Pagination (bottom right)
  char page[8];
  snprintf(page, sizeof(page), "%u/%u", (unsigned)(index + 1), (unsigned)total);
  text(40, 56, matrix_->color565(120, 120, 120), String(page));
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
  if (matrix_) matrix_->fillRect(x, y, width, height, colour);
}

/** Map heading degrees to a simple 8-direction arrow character. */
const char *Display::headingArrow(int deg) {
  // Normalise to 0-359
  deg = ((deg % 360) + 360) % 360;
  // 8 sectors of 45° each, starting at N=up
  static const char *arrows[] = { "^", "\\", ">", "/", "v", "\\", "<", "/" };
  return arrows[(deg + 22) / 45 % 8];
}
