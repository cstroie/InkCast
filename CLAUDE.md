# CLAUDE.md — InkCast project context

## Project summary

ESP32-C3 firmware (Arduino/PlatformIO) for a 2.9" tri-color e-paper weather display.
Fetches geolocation (ip-api.com, HTTP) and daily weather (Open-Meteo, HTTPS), renders
on a GDEM029C90 128×296 panel via GxEPD2. No API keys required for either service.

## Target hardware

- **MCU**: ESP32-C3 DevKitM-1 (`esp32-c3-devkitm-1` board in PlatformIO)
- **Display**: GDEM029C90, SSD1680, 128×296, 3-color (Black / White / Red)
- **SPI wiring** (from `src/display.h`): CS=SS, DC=8, RST=9, BUSY=10
- **Button**: GPIO 9 default (active-low, INPUT_PULLUP)
- **LED**: GPIO 8 default (active-low)

## Source files

| File | Purpose |
|------|---------|
| `src/main.cpp` | Application core: display, weather fetch, WiFi, NTP, LED, deep sleep |
| `src/display.h` | GxEPD2 display instantiation (multi-board header from the library) |
| `src/config_manager.h/.cpp` | NVS-backed runtime config via ESP32 Preferences (`namespace "epaper"`) |
| `src/portal.h/.cpp` | Two-mode config portal: blocking AP-mode (first boot) + non-blocking STA-mode (always-on) |
| `src/weathericons.h/.cpp` | WMO code → Weather Icons glyph lookup table |
| `src/WeatherIcons44pt7b.h` | Weather Icons font at 44pt — main weather icon (`WI_FONT`) |
| `src/WeatherIcons12pt7b.h` | Weather Icons font at 12pt, umbrella glyph only (`WI_SMALL_FONT`) |
| `data/weathericons.ttf` | Source font for generating bitmap headers via fontconvert |

`src/config.h` and `src/config.tpl` are **obsolete** — replaced by NVS + portal. Do not use them.

## Key design decisions

- **HTTPS weather fetch**: use `http.getString()` then `deserializeJson(doc, payload)` — NOT `http.getStream()`. WiFiClientSecure TLS streams are unreliable as ArduinoJson Stream sources on ESP32.
- **HTTP geolocation**: `getStream()` is fine here (plain HTTP, no TLS).
- **Geolocation cache**: `geoCached` flag — geo + NTP fetched once per power cycle. Deep sleep clears RAM so it re-fetches on every wake. `loop()` reuses the cached values.
- **Config portal modes**:
  - `runConfigPortal()` — blocks forever, WiFi AP + captive DNS, used at first boot or when button held
  - `startConfigServer()` / `handleConfigServer()` — non-blocking WebServer on station IP, started only when `deepSleepMins == -1`, polled at the top of every `loop()` iteration
- **Font rendering**: weather icon uses `display.drawChar()` with a `uint16_t` codepoint — NOT `display.print()`. The font is `weathericons44pt8b` (note the `8b` suffix, not `7b`).
- **Color use**: `GxEPD_RED` for severe weather icon (WMO codes 56,57,66,67,75,82,86,95,96,99) and for current temp ≥ 30 °C / 86 °F. Min–max range stays black.
- **Fetch retry**: on failure, waits a random 1–5 min then doubles each retry, capped at 30 min. Display is not updated until data arrives (old e-paper content preserved).
- **Deep sleep**: `esp_sleep_enable_timer_wakeup()` in microseconds. `-1` means stay awake (loop runs), `0` means no timer (sleeps but never auto-wakes).

## Display layout (296×128, rotation=1)

```
cx=68 cy=60           Weather icon, 44pt, centred in left column (COL=136, 2px margin each side)
x=right-2 y=16        City name, FreeSansBold9pt, right-aligned
centred [COL..295] y=62   Current temperature, FreeSansBold24pt (red if >= 30C/86F)
centred [COL..295] y=88   Min ... Max range, FreeSans12pt
centred [COL..295] y=112  Umbrella icons 0–5, WeatherIcons12pt (1 per 20%)
centred y=120         Footer: SSID | IP | forecast date | NTP time, 6×8 built-in font
```

Footer layout: suffix `IP | date | time` is built first; SSID is prepended trimmed to fit within
296 / 6 = 49 chars total (built-in font is 6 px per character including spacing).
Footer date comes from `daily.time[0]` in the Open-Meteo response (format `YYYY-MM-DD`, stored as `DD.MM.YY`). Time comes from NTP via `getLocalTime()`.
Current temperature comes from `current.temperature_2m` in the Open-Meteo response.

## LED blink conventions

| State | Pattern |
|-------|---------|
| Portal active | Steady on |
| WiFi connecting | Slow blink 250ms on/off |
| NTP wait loop | Fast blink 100ms on / 400ms off |
| Network fetch in progress | Steady on |
| Success | 2 short flashes (80/80ms) |
| Error | 3 long flashes (300/200ms) |

## Generating font headers

Font headers are generated from `data/weathericons.ttf` using the `fontconvert` tool
bundled with the Adafruit GFX Library. Build it once, then run it for any size/range needed.

```bash
# Build fontconvert (requires libfreetype-dev)
cd .pio/libdeps/esp32c3/Adafruit\ GFX\ Library/fontconvert
make

# Generate a font header — pass codepoints as DECIMAL (atoi can't parse 0xHHHH)
# fontconvert <ttf> <size> <first_decimal> <last_decimal> > src/Output.h
# Example: umbrella glyph (0xF084 = 61572) at 12pt
./fontconvert ../../../../../data/weathericons.ttf 12 61572 61572 > ../../../../../src/WeatherIcons12pt7b.h

# Full icon set at 44pt (0xF001=61441 to 0xF0B6=61622)
./fontconvert ../../../../../data/weathericons.ttf 44 61441 61622 > ../../../../../src/WeatherIcons44pt7b.h
```

The output filename suffix is `8b` when `last > 127` (all Weather Icons codepoints qualify).
Always add the GPL header and `#pragma once` manually after generation.

## Build

```bash
pio run                   # compile
pio run --target upload   # flash
pio device monitor        # serial at 115200
```

Flash usage is tight (~94% of 1.3 MB partition). Avoid adding large assets or libraries.

## Copyright

All source files: GPL-3.0, Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
