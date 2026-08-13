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
    "rotate_180": false
  },
  "fr24": {
    "poll_interval_seconds": 60
  }
}
```

## Constraints

- `radius_km` values above 200 km may return large payloads and cause memory issues on ESP32.
- `poll_interval_seconds` below 30 may result in rate-limiting or IP bans from the FR24 feed.
- `brightness` above 128 requires a power supply rated for the panel's peak current draw.
