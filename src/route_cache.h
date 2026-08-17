#pragma once

#include <Arduino.h>

/**
 * Bounded, LittleFS-backed cache for resolved flight routes.
 *
 * The cache stores up to kMaxEntries route records in
 * /route_cache.json.  Each entry records the callsign, origin, and
 * destination ICAO codes together with the Unix timestamp at which the
 * entry was written.  Entries older than ttlSeconds are treated as stale
 * and are not returned by lookup().
 *
 * All writes are atomic (write-then-rename) and the total number of
 * entries is capped so that the file never exceeds a few kilobytes.
 */
class RouteCache {
 public:
  static constexpr size_t kMaxEntries = 32;
  static constexpr uint32_t kDefaultTtlSeconds = 24UL * 3600UL;  // 24 h

  /** Look up a cached route for `callsign`.
   *  Returns true and populates `origin` / `destination` (4-char ICAO + NUL)
   *  if a fresh (non-expired) entry exists.  Returns false otherwise.
   */
  bool lookup(const char *callsign, char (&origin)[5],
               char (&destination)[5]) const;

  /** Persist a resolved route.  Silently discards invalid inputs. */
  void store(const char *callsign, const char *origin,
             const char *destination);

  /** Remove entries older than ttlSeconds from the on-disk cache. */
  void evictExpired(uint32_t ttlSeconds = kDefaultTtlSeconds);

 private:
  static constexpr const char *kCacheFile = "/route_cache.json";
  static constexpr const char *kCacheFileTmp = "/route_cache.json.tmp";
};
