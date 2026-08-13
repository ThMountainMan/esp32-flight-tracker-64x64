# Configuration

## Config File Location

The firmware reads `/config.json` from **LittleFS** at boot.

## Getting Started

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
  }
}
```

## Constraints

- `radius_km` must stay in the safe range 1–150 km.
- `poll_interval_seconds` below 30 may result in rate-limiting or IP bans from the FR24 feed.
- `brightness` above 128 requires a power supply rated for the panel's peak current draw.
- `flight_screen_seconds` must be between 2 and 60.
- `filters.min_altitude_ft` and `filters.max_altitude_ft` use `0` as disable values.
- If both altitude filters are active (non-zero), `min_altitude_ft` must be `<= max_altitude_ft`.
- FR24 altitude `0` (unknown altitude) is always retained by the altitude filter policy.
