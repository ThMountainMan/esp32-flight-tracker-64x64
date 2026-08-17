#pragma once

#include <Arduino.h>

#include "config.h"
#include "fr24_client.h"
#include "route_cache.h"

/**
 * Non-blocking, rate-limited flight-route resolver.
 *
 * RouteLookup wraps a RouteCache and performs remote HTTP(S) lookups when the
 * cache does not have a fresh entry.  Each call to tick() processes at most
 * one pending Aircraft and at most kMaxLookupsPerInterval lookups are issued
 * within any kLookupIntervalMs window, so the feature degrades gracefully
 * when the remote service is slow or unavailable.
 *
 * Lookup URL template
 * -------------------
 * The URL is configured via AppConfig::routeLookupUrl.  The placeholder
 * {callsign} is replaced with the flight's callsign.  Example:
 *
 *   https://my-tar1090/data/flightaware/{callsign}.json
 *
 * If routeLookupUrl is empty, lookup is disabled and all aircraft remain in
 * RouteState::Unknown.
 *
 * Privacy note
 * ------------
 * Each lookup sends the callsign to the configured endpoint.  No position
 * or identity data beyond the callsign is transmitted.  See docs/ROUTE_LOOKUP.md
 * for full details.
 */
class RouteLookup {
 public:
  static constexpr uint32_t kLookupIntervalMs = 5000;  // min ms between lookups
  static constexpr uint8_t kMaxLookupsPerInterval = 1;

  /** Process pending lookups for the aircraft list.  Call from loop(). */
  void tick(const AppConfig &config, std::vector<Aircraft> &aircraft);

  /** Reset per-flight lookup state (e.g. after a new poll fetch). */
  void resetStates(std::vector<Aircraft> &aircraft);

 private:
  RouteCache cache_;
  uint32_t lastLookupMs_ = 0;
  uint8_t lookupsThisInterval_ = 0;

  bool performLookup(const AppConfig &config, Aircraft &aircraft);
  static bool parseRouteResponse(const String &body, char (&origin)[5],
                                 char (&destination)[5]);
};
