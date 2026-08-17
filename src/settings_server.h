#pragma once

#include <ESPAsyncWebServer.h>

#include "config.h"

/**
 * Runtime settings and status web server.
 *
 * Runs on port 80 while the device is in normal station (STA) mode.
 * Endpoints:
 *   GET  /         – settings form pre-populated with the current configuration
 *   POST /save     – update settings atomically and restart
 *   GET  /status   – JSON object with device status (Wi-Fi, IP, last refresh …)
 *   POST /reset    – erase /config.json and reboot into provisioning
 */
class SettingsServer {
 public:
  /**
   * Initialise and begin the server.  Call once after Wi-Fi STA is connected.
   *
   * @param config         Reference to the live configuration.
   * @param sourceStatus   Pointer to the null-terminated last-refresh status string.
   */
  void begin(AppConfig &config, const char *sourceStatus);

  /** Stop the server.  Safe to call even if begin() was never called. */
  void stop();

 private:
  AsyncWebServer server_{80};
  bool running_ = false;
};
