#include "route_cache.h"

#include <ArduinoJson.h>
#include <LittleFS.h>
#include <cstring>
#include <time.h>

namespace {
// Helper: copy src into a fixed-size char array, always NUL-terminated.
template <size_t N>
void strncpy_safe(char (&dst)[N], const char *src) {
  strncpy(dst, src ? src : "", N - 1);
  dst[N - 1] = '\0';
}

/** Minimal validation: 3-4 upper-case alphanumeric chars. */
bool isValidIcaoAirport(const char *code) {
  if (!code) return false;
  const size_t len = strlen(code);
  if (len < 3 || len > 4) return false;
  for (size_t i = 0; i < len; ++i) {
    if (!isAlphaNumeric((unsigned char)code[i])) return false;
  }
  return true;
}
}  // namespace

bool RouteCache::lookup(const char *callsign, char (&origin)[5],
                         char (&destination)[5]) const {
  if (!callsign || callsign[0] == '\0') return false;

  File file = LittleFS.open(kCacheFile, "r");
  if (!file) return false;

  JsonDocument doc;
  const DeserializationError err = deserializeJson(doc, file);
  file.close();
  if (err) return false;

  const time_t now = time(nullptr);
  const JsonArrayConst entries = doc["entries"].as<JsonArrayConst>();
  for (const JsonObjectConst entry : entries) {
    const char *cs = entry["cs"] | "";
    if (strcmp(cs, callsign) != 0) continue;

    // Validate entry freshness: if clock is not yet synced (< 1 billion),
    // accept any stored entry to avoid refusing cached data unnecessarily.
    const uint32_t storedAt = entry["t"] | 0u;
    if (now > 1000000000L &&
        static_cast<uint32_t>(now) - storedAt > kDefaultTtlSeconds) {
      return false;
    }

    const char *org = entry["org"] | "";
    const char *dst = entry["dst"] | "";
    if (!isValidIcaoAirport(org) || !isValidIcaoAirport(dst)) return false;

    strncpy_safe(origin, org);
    strncpy_safe(destination, dst);
    return true;
  }
  return false;
}

void RouteCache::store(const char *callsign, const char *origin,
                       const char *destination) {
  if (!callsign || callsign[0] == '\0') return;
  if (!isValidIcaoAirport(origin) || !isValidIcaoAirport(destination)) {
    Serial.printf("[route_cache] Rejecting invalid airport codes for %s\n",
                  callsign);
    return;
  }

  // Read existing cache.
  JsonDocument doc;
  {
    File file = LittleFS.open(kCacheFile, "r");
    if (file) {
      deserializeJson(doc, file);  // Ignore errors; we'll rebuild below.
      file.close();
    }
  }
  if (!doc["entries"].is<JsonArray>()) {
    doc.clear();
    doc["entries"].to<JsonArray>();
  }

  JsonArray entries = doc["entries"].as<JsonArray>();
  const time_t now = time(nullptr);

  // Update existing entry if present.
  for (JsonObject entry : entries) {
    const char *cs = entry["cs"] | "";
    if (strcmp(cs, callsign) == 0) {
      entry["org"] = origin;
      entry["dst"] = destination;
      entry["t"] = static_cast<uint32_t>(now);
      goto write_cache;
    }
  }

  // Evict the oldest entry if the cache is full.
  if (entries.size() >= kMaxEntries) {
    JsonArray::iterator oldest = entries.begin();
    uint32_t oldestTime = UINT32_MAX;
    for (JsonArray::iterator it = entries.begin(); it != entries.end(); ++it) {
      const uint32_t t = (*it)["t"] | 0u;
      if (t < oldestTime) {
        oldestTime = t;
        oldest = it;
      }
    }
    entries.remove(oldest);
  }

  {
    JsonObject newEntry = entries.add<JsonObject>();
    newEntry["cs"] = callsign;
    newEntry["org"] = origin;
    newEntry["dst"] = destination;
    newEntry["t"] = static_cast<uint32_t>(now);
  }

write_cache : {
  File tmp = LittleFS.open(kCacheFileTmp, "w");
  if (!tmp) {
    Serial.println("[route_cache] Failed to open tmp file for write");
    return;
  }
  if (serializeJson(doc, tmp) == 0) {
    tmp.close();
    LittleFS.remove(kCacheFileTmp);
    Serial.println("[route_cache] Failed to serialise cache");
    return;
  }
  tmp.flush();
  tmp.close();
  LittleFS.remove(kCacheFile);
  if (!LittleFS.rename(kCacheFileTmp, kCacheFile)) {
    LittleFS.remove(kCacheFileTmp);
    Serial.println("[route_cache] Atomic rename failed");
  }
}
}

void RouteCache::evictExpired(uint32_t ttlSeconds) {
  File file = LittleFS.open(kCacheFile, "r");
  if (!file) return;

  JsonDocument doc;
  if (deserializeJson(doc, file)) {
    file.close();
    return;
  }
  file.close();

  if (!doc["entries"].is<JsonArray>()) return;
  JsonArray entries = doc["entries"].as<JsonArray>();
  const time_t now = time(nullptr);
  if (now < 1000000000L) return;  // Clock not synced; skip eviction.

  bool changed = false;
  JsonArray::iterator it = entries.begin();
  while (it != entries.end()) {
    const uint32_t t = (*it)["t"] | 0u;
    if (static_cast<uint32_t>(now) - t > ttlSeconds) {
      it = entries.remove(it);
      changed = true;
    } else {
      ++it;
    }
  }
  if (!changed) return;

  File tmp = LittleFS.open(kCacheFileTmp, "w");
  if (!tmp) return;
  if (serializeJson(doc, tmp) == 0) {
    tmp.close();
    LittleFS.remove(kCacheFileTmp);
    return;
  }
  tmp.flush();
  tmp.close();
  LittleFS.remove(kCacheFile);
  LittleFS.rename(kCacheFileTmp, kCacheFile);
}
