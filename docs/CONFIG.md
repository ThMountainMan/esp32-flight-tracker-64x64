# Configuration

## Config File Location

The firmware reads `/config.json` from **LittleFS** at boot.

## Getting Started

On a fresh flash, the firmware starts a temporary `FlightTracker-Setup-*` AP and
captive portal when no valid `/config.json` exists. Use that portal to save:

- Wi-Fi SSID/password
- Observer latitude/longitude
- Radius in km
- Display brightness
- Display rotation

The portal writes `/config.json` atomically and restarts into station mode.

You can still pre-seed LittleFS manually:

1. Copy the example:
   ```
   data/config.example.json  →  data/config.json
   ```
2. Edit `data/config.json` with your values.
3. Upload LittleFS (see [SETUP.md](SETUP.md)).

## All Fields

The config uses nested JSON objects.

| Path | Type | Default | Description |
|---|---|---|---|
| `wifi.ssid` | string | – | 2.4 GHz Wi-Fi SSID (required) |
| `wifi.password` | string | – | Wi-Fi password (required) |
| `location.latitude` | float | `55.87` | Observer latitude in decimal degrees |
| `location.longitude` | float | `-4.25` | Observer longitude in decimal degrees |
| `location.radius_km` | float | `30.0` | Search radius in km (1–150) |
| `display.brightness` | int | `64` | Panel brightness 0–255 (start low: 16–64) |
| `display.rotate_180` | bool | `false` | Rotate panel 180 ° if mounted upside down |
| `display.flight_screen_seconds` | int | `5` | Flight screen rotation interval in seconds (2–60) |
| `display.distance_unit` | string | `"km"` | Distance unit: `km` or `mi` |
| `display.altitude_unit` | string | `"ft"` | Altitude display unit: `ft` or `m` |
| `display.speed_unit` | string | `"kt"` | Speed display unit: `kt`, `kmh`, or `mph` |
| `display.clock_24_hour` | bool | `true` | Clock display format (`true` = 24-hour, `false` = 12-hour) |
| `filters.min_altitude_ft` | int | `0` | Minimum retained altitude in feet (`0` disables the lower filter) |
| `filters.max_altitude_ft` | int | `0` | Maximum retained altitude in feet (`0` disables the upper filter) |
| `fr24.poll_interval_seconds` | int | `60` | FR24 poll interval in seconds (30–3600) |
| `brightness_schedule.enabled` | bool | `false` | Enable scheduled day/night brightness switching |
| `brightness_schedule.day_brightness` | int | `64` | Panel brightness during the day window (0–255) |
| `brightness_schedule.night_brightness` | int | `8` | Panel brightness during the night window (0–255) |
| `brightness_schedule.day_start_hhmm` | int | `360` | Start of day window in minutes past midnight (0–1439); `360` = 06:00 |
| `brightness_schedule.night_start_hhmm` | int | `1320` | Start of night window in minutes past midnight (0–1439); `1320` = 22:00 |

## Example

```json
{
  "wifi": {
    "ssid": "MyNetwork",
    "password": "hunter2"
  },
  "location": {
    "latitude": 51.5074,
    "longitude": -0.1278,
    "radius_km": 50.0
  },
  "display": {
    "brightness": 64,
    "rotate_180": false,
    "flight_screen_seconds": 5,
    "distance_unit": "km",
    "altitude_unit": "ft",
    "speed_unit": "kt",
    "clock_24_hour": true
  },
  "filters": {
    "min_altitude_ft": 0,
    "max_altitude_ft": 0
  },
  "fr24": {
    "poll_interval_seconds": 60
  },
  "brightness_schedule": {
    "enabled": false,
    "day_brightness": 64,
    "night_brightness": 8,
    "day_start_hhmm": 360,
    "night_start_hhmm": 1320
  }
}
```

### Schedule examples

| Intent | `day_start_hhmm` | `night_start_hhmm` | Effect |
|---|---|---|---|
| Dim 22:00–06:00 (typical) | `360` | `1320` | Full brightness 06:00–22:00, dim at night |
| Always dim (overnight use) | `0` | `1` | Bright 00:00–00:01, dim the rest |
| Morning-only bright 07:00–09:00 | `420` | `540` | Bright for two hours, dim otherwise |

> **Time-sync note:** Scheduled brightness requires a successful NTP sync.
> Until the device obtains a valid time (within roughly 30 seconds of connecting
> to Wi-Fi), the fixed `display.brightness` value is used regardless of the
> schedule.  If NTP is permanently unavailable the fixed brightness is used
> for the entire session.  The schedule is re-evaluated once per minute in the
> main loop without interrupting display refresh, Wi-Fi reconnection, or flight
> cycling.

## Constraints

- `radius_km` must stay in the safe range 1–150 km.
- `poll_interval_seconds` below 30 may result in rate-limiting or IP bans from the FR24 feed.
- `brightness` above 128 requires a power supply rated for the panel's peak current draw.
- `flight_screen_seconds` must be between 2 and 60.
- `filters.min_altitude_ft` and `filters.max_altitude_ft` use `0` as disable values.
- If both altitude filters are active (non-zero), `min_altitude_ft` must be `<= max_altitude_ft`.
- FR24 altitude `0` (unknown altitude) is always retained by the altitude filter policy.
- When `brightness_schedule.enabled` is `true`, `day_start_hhmm` and `night_start_hhmm` must be different and both in range 0–1439.
- `day_brightness` and `night_brightness` follow the same 0–255 limits as `display.brightness`.
