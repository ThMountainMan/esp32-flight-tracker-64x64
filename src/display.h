#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "config.h"
#include "fr24_client.h"

/** Renders status, idle, and aircraft screens on one 64x64 HUB75 panel. */
class Display {
 public:
  bool begin(const AppConfig &config);
  void showSplash();
  void showStatus(const char *title, const String &detail, uint16_t colour);
  void showClock(bool networkOk, const String &sourceStatus);
  void showFlight(const Aircraft &aircraft, size_t index, size_t total);

 private:
  MatrixPanel_I2S_DMA *matrix_ = nullptr;
  bool rotate180_ = false;

  void clear();
  void text(int x, int y, uint16_t colour, const String &value,
            uint8_t size = 1);
  void fillRect(int x, int y, int width, int height, uint16_t colour);
  static const char *headingArrow(int deg);
};
