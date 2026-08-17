# Route and Airport Lookup

This document describes the optional lightweight flight-route and airport
lookup feature added in release v0.x.  The feature resolves origin and
destination ICAO airport codes for visible aircraft and displays them on
the 64×64 LED matrix without bundling any airport database into the firmware
image.

---

## Overview

When enabled, the firmware periodically queries a configurable HTTP endpoint
to resolve the origin and destination airports for each callsign it detects.
Successful responses are cached in LittleFS so that future flights on the
same route do not generate additional network requests.

---

## Configuration

Add a `route_lookup` object to `/config.json` (see also
`data/config.example.json`):

```json
"route_lookup": {
  "enabled": true,
  "url": "http://192.168.1.10/data/flightaware/{callsign}.json",
  "cache_ttl_seconds": 86400,
  "home_airport": "EGLL"
}
```

| Key                | Type    | Default | Description |
|--------------------|---------|---------|-------------|
| `enabled`          | boolean | `false` | Master switch.  No lookups are performed when `false`. |
| `url`              | string  | `""`    | URL template.  `{callsign}` is replaced with the flight callsign before each request. |
| `cache_ttl_seconds`| integer | `86400` | How long (in seconds) a cached result is considered fresh.  After expiry the entry is re-fetched on next encounter. |
| `home_airport`     | string  | `""`    | Optional 4-character ICAO code.  When a resolved route contains this code it is rendered in amber on the display. |

---

## Supported Lookup Services

The feature is intentionally endpoint-agnostic; any HTTP(S) service that
returns a JSON object with `"origin"` and `"destination"` keys (4-letter ICAO
values) is compatible.

### tar1090 / dump1090-fa (recommended)

If you already run a local ADS-B receiver with
[tar1090](https://github.com/wiedehopf/tar1090), the FlightAware route data
it optionally downloads can be queried directly:

```
http://<raspberry-pi>/data/flightaware/{callsign}.json
```

This keeps all lookups on your local network and avoids any dependency on
third-party cloud services.

### Custom proxy

You can run a small intermediary (e.g. a Python Flask app) that accepts a
callsign query and returns:

```json
{ "origin": "EGLL", "destination": "KJFK" }
```

---

## Privacy

Each lookup sends only the **callsign** (e.g. `BAW123`) to the configured
endpoint.  No position data, device identifier, or personal information is
transmitted.  When using a local tar1090 instance the data never leaves your
home network.

If you use a remote cloud API you should review that provider's privacy
policy; the firmware makes no additional requests beyond the callsign query.

---

## Network behaviour

- Lookups are **rate-limited**: at most one HTTP request is issued per 5-second
  window from the main loop.
- Lookups are **non-blocking**: the display loop continues to refresh aircraft
  telemetry while a lookup is in progress.
- If a lookup fails (HTTP error, timeout, parse error) the aircraft is marked
  `Failed` and will not be re-tried until the next poll cycle brings it back
  as a new entry.
- Flight telemetry (altitude, speed, heading, distance) is **always** displayed
  promptly regardless of lookup status.

---

## Cache behaviour

Successfully resolved routes are stored in `/route_cache.json` on LittleFS.

- The cache holds at most **32 entries**.  When full, the oldest entry is
  evicted before a new one is written.
- All writes use an atomic write-then-rename strategy to avoid corruption on
  power loss.
- Entries older than `cache_ttl_seconds` are treated as stale and will
  trigger a fresh lookup.
- If the system clock has not yet been synchronised (time < epoch 1 000 000 000),
  any cached entry is accepted without TTL validation to avoid discarding useful
  data during startup.

---

## Display layout

When a route has been resolved the origin and destination codes appear on
row 44 of the 64×64 matrix, between the heading/distance line and the page
indicator.  Example:

```
[Icon] BAW123
       B789
A35000ft
S480kt
H270  12.3km
EGLL > KJFK        ← route row (amber if home airport)
2/5
```

When the lookup is in progress `...` is displayed in that row.  When lookup
is disabled or has failed the row is left blank.

---

## Limitations

- The feature requires a separate HTTP service; no airport data is bundled in
  the firmware.
- Certificate validation is skipped for HTTPS endpoints (consistent with the
  existing FR24 client behaviour).  For production use, restrict to HTTP
  on your local network or implement certificate pinning.
- Only the primary callsign is used as the lookup key; hex IDs are not
  submitted.
- The 32-entry cache is intentionally small.  Aircraft flying repeated routes
  will be cached; rare one-off callsigns may be re-fetched on each
  encounter once their TTL expires.
