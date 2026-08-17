# Optional Weather Idle Scene

The firmware can display a current-temperature and conditions screen on the
64×64 HUB75 panel when no aircraft are visible.  The feature is **disabled by
default** and requires an OpenWeatherMap API key that you supply yourself.

---

## Data provider

| Item | Detail |
|------|--------|
| Provider | [OpenWeatherMap](https://openweathermap.org/) |
| Endpoint | `api.openweathermap.org/data/2.5/weather` (current weather) |
| Required plan | Free tier is sufficient (60 calls/minute limit) |
| API key | Personal — never included in firmware source or release assets |

---

## Privacy implications

The firmware sends **your configured latitude and longitude** to
`api.openweathermap.org` every `refresh_seconds` (default 600 s).  Only you
and OpenWeatherMap receive these coordinates.  No data is sent to any other
third party.  If precise location privacy is a concern, round your coordinates
to one decimal place (≈ 11 km accuracy) before adding them to `config.json`.

---

## Enabling the feature

1. Register for a free API key at <https://openweathermap.org/api>.
2. Edit `/config.json` on the device's LittleFS partition (or use the
   provisioning portal):

```json
"weather": {
  "enabled": true,
  "api_key": "your_key_here",
  "refresh_seconds": 600
}
```

3. Upload the filesystem image and reboot.

**The placeholder value `REPLACE_WITH_OPENWEATHERMAP_API_KEY` in
`data/config.example.json` must never be committed with a real key.**

---

## Refresh policy

| Parameter | Default | Range |
|-----------|---------|-------|
| `refresh_seconds` | 600 (10 min) | 300 – 3600 s |

- The firmware fetches weather **only** when Wi-Fi is connected and the
  feature is enabled.
- If a fetch fails (network error, HTTP error, JSON parse error, or value
  out of range) the previous valid reading is preserved.
- The source-status bar at the bottom of the weather scene shows the current
  flight-tracker status so you can tell at a glance whether the FR24 feed is
  healthy.
- If `enabled` is `true` but `api_key` is empty the feature is silently
  disabled at boot; a warning is printed to the serial console.

---

## Idle-scene behaviour

When no aircraft match the configured radius the panel alternates between:

1. **Clock scene** — time, date, and source status (always shown).
2. **Weather scene** — temperature and conditions (shown only when enabled and
   a valid reading has been fetched).

The scene switches every `flight_screen_seconds` (default 5 s), matching the
flight-carousel interval so the transitions feel consistent.

---

## Memory and board notes

| Metric | Classic ESP32 | ESP32-S3 (PSRAM) |
|--------|---------------|-----------------|
| HTTP response cap | 1024 bytes | 1024 bytes |
| JSON parse heap | ~200 bytes (filter-only) | ~200 bytes |
| WeatherData struct | ~80 bytes | ~80 bytes |
| PSRAM required? | No | No |

The filter-only ArduinoJson parse (`DeserializationOption::Filter`) discards
all fields except `main.temp`, `weather[0].description`, and
`weather[0].icon`, keeping heap impact minimal on a classic ESP32.  PSRAM is
not required.

The HTTP connection uses plain HTTP (`http://`) to avoid the TLS handshake
overhead and certificate-bundle size on a classic ESP32.  If you run an
ESP32-S3 with PSRAM and are comfortable with the extra flash, you can switch
the `WeatherClient` to `WiFiClientSecure` + `http.begin(wifiClientSecure, url)`
and pin the OWM root CA.

---

## Disabling the feature

Set `"enabled": false` (or remove the `weather` section entirely).  The
firmware falls back to the clock-only idle scene.  No API calls are made and
no keys are loaded into memory.
