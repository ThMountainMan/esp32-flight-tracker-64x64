#pragma once

#include <ESP32-HUB75-MatrixPanel-I2S-DMA.h>

#include "config.h"
#include "fr24_client.h"

/** Renders status, idle, and aircraft screens on one 64x64 HUB75 panel. */
class Display {
 public:
  bool begin(const AppConfig &config);
  void showSplash();
  void showStatus(const char *title, const char *detail, uint16_t colour);
  void showClock(bool networkOk, const char *sourceStatus);
  void showFlight(const Aircraft &aircraft, size_t index, size_t total);

 private:
  enum class DistanceUnit : uint8_t { Km, Mi };
  enum class AltitudeUnit : uint8_t { Feet, Meters };
  enum class SpeedUnit : uint8_t { Knots, Kmh, Mph };

  MatrixPanel_I2S_DMA *matrix_ = nullptr;
  bool rotate180_ = false;
  bool clock24Hour_ = true;
  DistanceUnit distanceUnit_ = DistanceUnit::Km;
  AltitudeUnit altitudeUnit_ = AltitudeUnit::Feet;
  SpeedUnit speedUnit_ = SpeedUnit::Knots;

  void clear();
  void text(int x, int y, uint16_t colour, const char *value,
            uint8_t size = 1);
  void fillRect(int x, int y, int width, int height, uint16_t colour);
};
