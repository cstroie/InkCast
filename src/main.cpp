/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "display.h"

#include <WiFi.h>
#include <DNSServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include "weathericons.h"
#include "config_manager.h"
#include "portal.h"

// ---------------------------------------------------------------------------
// Runtime configuration (loaded from NVS at boot)
// ---------------------------------------------------------------------------

static Config config;

// ---------------------------------------------------------------------------
// Deep-sleep retry counter — survives deep sleep via RTC memory
// ---------------------------------------------------------------------------

RTC_DATA_ATTR static int    fetchRetryIndex = 0;

// Progressive retry schedule in minutes (last entry is the cap)
static const int retrySchedule[]  = {1, 2, 3, 4, 5, 10, 15};
static const int retryScheduleLen = (int)(sizeof(retrySchedule) / sizeof(retrySchedule[0]));

// ---------------------------------------------------------------------------
// Timekeeping across deep sleep — stored in RTC memory
// ---------------------------------------------------------------------------

RTC_DATA_ATTR static time_t   rtcSavedTime  = 0;     // epoch saved just before sleep
RTC_DATA_ATTR static uint32_t rtcSleepSecs  = 0;     // sleep duration in seconds
RTC_DATA_ATTR static bool     everDisplayed = false;  // true once weather has been shown

// ---------------------------------------------------------------------------
// Geolocation cache — stored in RTC memory, survives deep sleep
// ---------------------------------------------------------------------------

RTC_DATA_ATTR static float cachedLat           = 0.0f;
RTC_DATA_ATTR static float cachedLon           = 0.0f;
RTC_DATA_ATTR static long  cachedUtcOffset     = 0;
RTC_DATA_ATTR static bool  geoCached           = false;
RTC_DATA_ATTR static char  currentLocation[96] = "";

// ---------------------------------------------------------------------------
// Weather state
// ---------------------------------------------------------------------------

static uint16_t currentIconCode    = WI_NA;
static int      currentWeatherCode = 0;
static float    currentTemp        = 0.0f;  // actual current temperature
static float    currentTempMax     = 0.0f;
static float    currentTempMin     = 0.0f;
static float    currentPrecipProb  = 0.0f;
static char     currentTempUnit    = 'C';
static char currentForecastDate[12] = "";  // "DD.MM.YY" from current.time
static int  currentReportHour       = -1;  // local hour from current.time
static int  currentReportMin        = -1;  // local minute from current.time
static unsigned long lastWeatherUpdate = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void ledOn() {
  if (config.ledPin != -1) digitalWrite(config.ledPin, LOW);   // active-low
}

void ledOff() {
  if (config.ledPin != -1) digitalWrite(config.ledPin, HIGH);
}

void ledBlink(int times, int onMs, int offMs) {
  for (int i = 0; i < times; i++) {
    ledOn();  delay(onMs);
    ledOff(); if (i < times - 1) delay(offMs);
  }
}

void saveTimeForSleep(uint32_t sleepSecs) {
  rtcSavedTime  = time(nullptr);
  rtcSleepSecs  = sleepSecs;
}

// Open-Meteo updates at :00/:15/:30/:45; we fetch 2 minutes later.
static const int WAKE_SLOTS[] = {2 * 60, 17 * 60, 32 * 60, 47 * 60};

// Returns seconds until the next aligned wake slot.
// Jumps exactly intervalMins/15 slots forward from the last passed slot,
// so the device always wakes at :02/:17/:32/:47 on the correct cadence.
// Falls back to intervalMins*60 if local time is unavailable.
static uint32_t secsUntilAlignedSlot(int intervalMins) {
  struct tm t;
  if (!getLocalTime(&t)) return (uint32_t)intervalMins * 60u;
  int curSec = t.tm_min * 60 + t.tm_sec;

  // Find the most recent past slot (may be negative if before :02 this hour)
  int lastSlot = WAKE_SLOTS[3] - 3600;  // :47 of previous hour as default
  for (int i = 3; i >= 0; i--) {
    if (WAKE_SLOTS[i] <= curSec) { lastSlot = WAKE_SLOTS[i]; break; }
  }

  int targetSec    = lastSlot + (intervalMins / 15) * 15 * 60;
  uint32_t wait    = (uint32_t)(targetSec - curSec);
  Serial.printf("Aligned sleep: %us (next slot in %um%02us)\n",
                wait, wait / 60, wait % 60);
  return wait;
}

void deepSleep() {
  if (config.deepSleepMins > 0) {
    uint32_t secs = secsUntilAlignedSlot(config.updateInterval);
    saveTimeForSleep(secs);
    esp_sleep_enable_timer_wakeup((uint64_t)secs * 1000000ULL);
    esp_deep_sleep_start();
  }
}

// Sleep for the next retry interval without updating the display.
// Only effective when periodic deep sleep is configured (deepSleepMins > 0).
void deepSleepRetry() {
  if (config.deepSleepMins <= 0) return;
  int idx  = min(fetchRetryIndex, retryScheduleLen - 1);
  int mins = retrySchedule[idx];
  fetchRetryIndex++;
  Serial.printf("Retry %d: sleeping %d min before next attempt\n", fetchRetryIndex, mins);
  uint32_t secs = (uint32_t)mins * 60u;
  saveTimeForSleep(secs);
  esp_sleep_enable_timer_wakeup((uint64_t)secs * 1000000ULL);
  esp_deep_sleep_start();
}

bool isSevereWeather(int code) {
  switch (code) {
    case 56: case 57:           // freezing drizzle
    case 66: case 67:           // freezing rain
    case 75: case 82:           // heavy snow / violent showers
    case 86:                    // heavy snow showers
    case 95: case 96: case 99:  // thunderstorm
      return true;
    default:
      return false;
  }
}

// ---------------------------------------------------------------------------
// Display helpers
// ---------------------------------------------------------------------------

// drawChar() in Adafruit GFX truncates the codepoint to uint8_t, so Weather
// Icons glyphs (0xF001–0xF0B6) render as garbage. This helper does the same
// bitmap walk with proper uint16_t arithmetic.
static void drawGlyph(int16_t x, int16_t y, uint16_t code,
                      uint16_t color, const GFXfont* font) {
  if (code < pgm_read_word(&font->first) || code > pgm_read_word(&font->last))
    return;
  uint16_t idx    = code - pgm_read_word(&font->first);
  GFXglyph* glyph = (GFXglyph*)pgm_read_ptr(&font->glyph) + idx;
  uint8_t* bitmap = (uint8_t*)pgm_read_ptr(&font->bitmap);

  uint16_t bo = pgm_read_word(&glyph->bitmapOffset);
  uint8_t  w  = pgm_read_byte(&glyph->width);
  uint8_t  h  = pgm_read_byte(&glyph->height);
  int8_t   xo = pgm_read_byte(&glyph->xOffset);
  int8_t   yo = pgm_read_byte(&glyph->yOffset);

  uint8_t bits = 0, bit = 0;
  for (uint8_t yy = 0; yy < h; yy++) {
    for (uint8_t xx = 0; xx < w; xx++) {
      if (!bit) { bits = pgm_read_byte(&bitmap[bo++]); bit = 8; }
      if (bits & 0x80) display.drawPixel(x + xo + xx, y + yo + yy, color);
      bits <<= 1;
      bit--;
    }
  }
}

// Error screen: WI_NA icon in left column (red), message lines in right column,
// optional footer line full-width at y=120 (built-in 6×8 font, like weather screen).
void displayNetworkError(const char* line1, const char* line2 = nullptr,
                         const char* footer = nullptr) {
  static const int COL     = 136;
  static const int ICON_CX = 68;
  static const int ICON_CY = 60;

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    {
      GFXglyph* g = (GFXglyph*)pgm_read_ptr(&WI_FONT->glyph) + (WI_CLOUD_REFRESH - pgm_read_word(&WI_FONT->first));
      int16_t ix = ICON_CX - pgm_read_byte(&g->xAdvance) / 2;
      int16_t iy = ICON_CY - (int8_t)pgm_read_byte(&g->yOffset) - (int16_t)pgm_read_byte(&g->height) / 2;
      drawGlyph(ix, iy, WI_CLOUD_REFRESH, GxEPD_RED, WI_FONT);
    }

    // Device name — right-aligned, top row
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t cx, cy; uint16_t cw, ch;
    char apName[] = "InkCast";
    display.getTextBounds(apName, 0, 0, &cx, &cy, &cw, &ch);
    display.setCursor(display.width() - (int16_t)cw - 2, 16);
    display.print(apName);

    int lineH  = 22;
    int lines  = 1 + (line2 ? 1 : 0);
    int areaH  = footer ? 108 : display.height();
    int startY = (areaH + lines * lineH) / 2 - (lines - 1) * lineH;

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(COL + 4, startY);
    display.print(line1);
    if (line2) {
      display.setFont(&FreeSans9pt7b);
      display.setCursor(COL + 4, startY + lineH);
      display.print(line2);
    }

    if (footer) {
      display.setFont(NULL);
      display.setTextSize(1);
      int16_t fx, fy; uint16_t fw, fh;
      display.getTextBounds(footer, 0, 0, &fx, &fy, &fw, &fh);
      display.setCursor((display.width() - (int16_t)fw) / 2, 120);
      display.print(footer);
    }
  } while (display.nextPage());
}

// Show the config-portal SSID and instructions on screen.
void displayPortalInfo(const char* apName) {
  static const int COL     = 136;
  static const int ICON_CX = 68;
  static const int ICON_CY = 60;

  // Build footer: "AP: <apName>  192.168.4.1"
  char footer[64];
  snprintf(footer, sizeof(footer), "AP: %s  192.168.4.1", apName);

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Icon — wi-refresh, black, centred in left column
    {
      GFXglyph* g = (GFXglyph*)pgm_read_ptr(&WI_FONT->glyph) + (WI_REFRESH - pgm_read_word(&WI_FONT->first));
      int16_t ix = ICON_CX - pgm_read_byte(&g->xAdvance) / 2;
      int16_t iy = ICON_CY - (int8_t)pgm_read_byte(&g->yOffset) - (int16_t)pgm_read_byte(&g->height) / 2;
      drawGlyph(ix, iy, WI_REFRESH, GxEPD_BLACK, WI_FONT);
    }

    // App name — right-aligned, top row
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    {
      int16_t cx, cy; uint16_t cw, ch;
      char label[] = "InkCast";
      display.getTextBounds(label, 0, 0, &cx, &cy, &cw, &ch);
      display.setCursor(display.width() - (int16_t)cw - 2, 16);
      display.print(label);
    }

    // Instruction lines in right column, centred vertically above footer
    static const char* lines[] = {
      "Setup mode",
      "Connect to WiFi:",
      apName,
    };
    static const int nLines = 3;
    static const int lineH  = 22;
    int areaH  = 108;  // leave room for footer at y=120
    int startY = (areaH + nLines * lineH) / 2 - (nLines - 1) * lineH;

    for (int i = 0; i < nLines; i++) {
      if (i == 0)
        display.setFont(&FreeSansBold9pt7b);
      else if (i == 2) {
        display.setFont(&FreeSansBold9pt7b);
        display.setTextColor(GxEPD_RED);
      } else {
        display.setFont(&FreeSans9pt7b);
        display.setTextColor(GxEPD_BLACK);
      }
      display.setCursor(COL + 4, startY + i * lineH);
      display.print(lines[i]);
    }

    // Footer
    display.setFont(NULL);
    display.setTextSize(1);
    display.setTextColor(GxEPD_BLACK);
    {
      int16_t fx, fy; uint16_t fw, fh;
      display.getTextBounds(footer, 0, 0, &fx, &fy, &fw, &fh);
      display.setCursor((display.width() - (int16_t)fw) / 2, 120);
      display.print(footer);
    }
  } while (display.nextPage());
}

void displayWeather() {
  everDisplayed = true;
  static const int COL     = 136;  // 132px max icon width + 2px margin each side
  static const int ICON_CX = 68;   // horizontal centre of left icon column
  static const int ICON_CY = 60;   // vertical centre (footer at y=120 → mid = 60)

  bool severe  = isSevereWeather(currentWeatherCode);
  bool hot     = (currentTempUnit == 'C') ? (currentTemp >= 30.0f) : (currentTemp >= 86.0f);
  uint16_t iconColor = severe  ? GxEPD_RED : GxEPD_BLACK;
  uint16_t currColor = hot     ? GxEPD_RED : GxEPD_BLACK;

  // Umbrella count: 0–5, one per 20 pp (0% → 0, 1–20% → 1, …, 81–100% → 5)
  int umbrellas = (int)((currentPrecipProb + 19.0f) / 20.0f);
  if (umbrellas > 5) umbrellas = 5;

  bool timeOk = (currentReportHour >= 0);

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Weather icon — centred in left column both horizontally and vertically
    {
      GFXglyph* g = (GFXglyph*)pgm_read_ptr(&WI_FONT->glyph) + (currentIconCode - pgm_read_word(&WI_FONT->first));
      int16_t ix = ICON_CX - pgm_read_byte(&g->xAdvance) / 2;
      int16_t iy = ICON_CY - (int8_t)pgm_read_byte(&g->yOffset) - (int16_t)pgm_read_byte(&g->height) / 2;
      drawGlyph(ix, iy, currentIconCode, iconColor, WI_FONT);
    }

    // City name — right-aligned, top row
    const char* comma = strchr(currentLocation, ',');
    char city[64];
    if (comma)
      snprintf(city, sizeof(city), "%.*s", (int)(comma - currentLocation), currentLocation);
    else
      snprintf(city, sizeof(city), "%s", currentLocation);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t cx, cy; uint16_t cw, ch;
    display.getTextBounds(city, 0, 0, &cx, &cy, &cw, &ch);
    display.setCursor(display.width() - (int16_t)cw - 2, 16);
    display.print(city);

    // Line 1 — current temperature, large, red when hot, centred in right column
    display.setFont(&FreeSansBold24pt7b);
    display.setTextColor(currColor);
    char currStr[10];
    snprintf(currStr, sizeof(currStr), "%.1f%c", currentTemp, currentTempUnit);
    {
      int16_t tx, ty; uint16_t tw, th;
      display.getTextBounds(currStr, 0, 0, &tx, &ty, &tw, &th);
      display.setCursor(COL + (display.width() - COL - (int16_t)tw) / 2, 62);
    }
    display.print(currStr);

    // Line 2 — min–max range, centred in right column
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);
    char rangeStr[16];
    snprintf(rangeStr, sizeof(rangeStr), "%.0f ... %.0f%c", currentTempMin, currentTempMax, currentTempUnit);
    {
      int16_t tx, ty; uint16_t tw, th;
      display.getTextBounds(rangeStr, 0, 0, &tx, &ty, &tw, &th);
      display.setCursor(COL + (display.width() - COL - (int16_t)tw) / 2, 88);
    }
    display.print(rangeStr);

    // Line 3 — umbrellas (0–5), centred in right column
    if (umbrellas > 0) {
      int totalW = umbrellas * 24 - 2;  // last glyph needs no trailing gap
      int x = COL + (display.width() - COL - totalW) / 2;
      for (int i = 0; i < umbrellas; i++) {
        drawGlyph(x, 112, WI_UMBRELLA, GxEPD_BLACK, WI_SMALL_FONT);
        x += 24;
      }
    }

    // Footer — SSID | IP | date | time, built-in 6×8 font, centred
    {
      // Build fixed suffix (IP + optional date + optional time) first,
      // then prepend as much of the SSID as the remaining pixels allow.
      // Built-in font: 6px wide + 1px spacing = 7px per char.
      String ip = WiFi.localIP().toString();
      bool hasDate = currentForecastDate[0] != '\0';
      char suffix[48];
      if (hasDate && timeOk)
        snprintf(suffix, sizeof(suffix), "%s | %s | %02d:%02d",
                 ip.c_str(), currentForecastDate, currentReportHour, currentReportMin);
      else if (hasDate)
        snprintf(suffix, sizeof(suffix), "%s | %s", ip.c_str(), currentForecastDate);
      else
        snprintf(suffix, sizeof(suffix), "%s", ip.c_str());

      const int charW   = 6;
      int maxChars      = display.width() / charW;
      int suffixChars   = strlen(suffix);
      int ssidMax       = maxChars - suffixChars - 3;  // 3 for " | "

      char footer[64];
      if (ssidMax > 0) {
        String ssid = WiFi.SSID();
        if ((int)ssid.length() > ssidMax) ssid = ssid.substring(0, ssidMax);
        snprintf(footer, sizeof(footer), "%s | %s", ssid.c_str(), suffix);
      } else {
        snprintf(footer, sizeof(footer), "%s", suffix);
      }

      display.setFont(NULL);
      display.setTextSize(1);
      int16_t fx, fy; uint16_t fw, fh;
      display.getTextBounds(footer, 0, 0, &fx, &fy, &fw, &fh);
      display.setCursor((display.width() - (int16_t)fw) / 2, 120);
      display.print(footer);
    }
  } while (display.nextPage());
}

// ---------------------------------------------------------------------------
// Data fetching
// ---------------------------------------------------------------------------

// Try each configured network, preferring those visible in a scan.
// Returns true when connected.
static bool connectWiFi() {
  if (config.wifiCount == 0) return false;

  int scanCount = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false);

  // Find best RSSI for each stored network in scan results
  int32_t rssiOf[WIFI_MAX];
  bool    inRange[WIFI_MAX];
  for (int i = 0; i < config.wifiCount; i++) {
    rssiOf[i]  = -999;
    inRange[i] = false;
    for (int s = 0; s < scanCount && scanCount > 0; s++) {
      if (WiFi.SSID(s) == config.wifi[i].ssid) {
        if (WiFi.RSSI(s) > rssiOf[i]) rssiOf[i] = WiFi.RSSI(s);
        inRange[i] = true;
      }
    }
  }
  WiFi.scanDelete();

  // Build attempt order: in-range networks by RSSI desc, then out-of-range in stored order
  int  order[WIFI_MAX], orderN = 0;
  bool used[WIFI_MAX]  = {};
  while (true) {
    int best = -1;
    for (int i = 0; i < config.wifiCount; i++)
      if (!used[i] && inRange[i] && (best == -1 || rssiOf[i] > rssiOf[best])) best = i;
    if (best == -1) break;
    order[orderN++] = best; used[best] = true;
  }
  for (int i = 0; i < config.wifiCount; i++)
    if (!used[i]) order[orderN++] = i;

  for (int c = 0; c < orderN; c++) {
    int i = order[c];
    Serial.printf("WiFi trying \"%s\"%s\n", config.wifi[i].ssid, inRange[i] ? "" : " (not in scan)");
    WiFi.begin(config.wifi[i].ssid, config.wifi[i].pass);
    for (int t = 0; t < 20 && WiFi.status() != WL_CONNECTED; t++) {
      ledOn(); delay(250); ledOff(); delay(250);
      Serial.print(".");
    }
    Serial.println();
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("WiFi connected: \"%s\" — %s\n", config.wifi[i].ssid, WiFi.localIP().toString().c_str());
      return true;
    }
    WiFi.disconnect(false);
    delay(100);
  }
  return false;
}

static bool ensureWiFiConnected() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.println("WiFi disconnected, reconnecting...");
  if (!connectWiFi()) {
    Serial.println("WiFi reconnect failed");
    return false;
  }
  return true;
}

bool fetchGeolocation() {
  Serial.println("Fetching geolocation...");
  ledOn();  // steady = network busy
  HTTPClient http;
  http.begin("http://ip-api.com/json/?fields=status,message,city,regionName,country,lat,lon,offset");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Geolocation HTTP error: %d\n", code);
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err || doc["status"] != "success") {
    Serial.println("Geolocation parse/API error");
    ledOff();
    return false;
  }

  snprintf(currentLocation, sizeof(currentLocation), "%s, %s, %s",
           doc["city"].as<const char*>(),
           doc["regionName"].as<const char*>(),
           doc["country"].as<const char*>());
  cachedLat       = doc["lat"].as<float>();
  cachedLon       = doc["lon"].as<float>();
  cachedUtcOffset = doc["offset"].as<long>();

  Serial.printf("Location: %s  (%.4f, %.4f)  UTC+%lds\n",
                currentLocation, cachedLat, cachedLon, cachedUtcOffset);
  // Build a POSIX TZ string from the UTC offset so getLocalTime() returns local time.
  // POSIX sign is inverted: "UTC-1" means UTC+1.  Handles sub-hour offsets (e.g. India UTC+5:30).
  long m = (abs(cachedUtcOffset) % 3600) / 60;
  char tzStr[16];
  if (m) snprintf(tzStr, sizeof(tzStr), "UTC%+ld:%02ld", -(cachedUtcOffset / 3600), m);
  else   snprintf(tzStr, sizeof(tzStr), "UTC%+ld",        -(cachedUtcOffset / 3600));
  setenv("TZ", tzStr, 1);
  tzset();
  ledOff();
  return true;
}

// Parse an HTTP Date header ("Fri, 29 May 2026 14:42:38 GMT") to a UTC epoch.
// Uses the civil-days formula (Howard Hinnant) — no TZ side effects.
static time_t parseHttpDate(const String& dateStr) {
  struct tm t = {};
  if (!strptime(dateStr.c_str(), "%a, %d %b %Y %H:%M:%S GMT", &t)) return 0;
  int y = t.tm_year + 1900, m = t.tm_mon + 1, d = t.tm_mday;
  y -= m <= 2;
  int era = (y >= 0 ? y : y - 399) / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  long days = (long)era * 146097 + doe - 719468;
  return (time_t)(days * 86400L + t.tm_hour * 3600L + t.tm_min * 60L + t.tm_sec);
}

bool fetchWeatherData() {
  Serial.printf("Fetching weather for (%.4f, %.4f)...\n", cachedLat, cachedLon);
  ledOn();  // steady = network busy

  String url = "http://api.open-meteo.com/v1/forecast"
               "?latitude="  + String(cachedLat, 4) +
               "&longitude=" + String(cachedLon, 4) +
               "&current=weather_code,temperature_2m,is_day"
               "&daily=weather_code,temperature_2m_max,temperature_2m_min"
               ",precipitation_probability_max"
               "&timezone=auto&forecast_days=" + String(config.forecastDays) +
               (config.tempUnits == 0
                 ? "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch"
                 : "&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm");

  HTTPClient http;
  http.begin(url);
  const char* hdrs[] = {"Date"};
  http.collectHeaders(hdrs, 1);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error: %d\n", code);
    http.end();
    ledOff();
    return false;
  }

  // Set system clock from the response Date header (UTC epoch + stored UTC offset)
  String dateHdr = http.header("Date");
  if (dateHdr.length() > 0) {
    time_t utc = parseHttpDate(dateHdr);
    if (utc > 0) {
      struct timeval tv = { utc, 0 };
      settimeofday(&tv, nullptr);
      Serial.printf("Time from HTTP: %s\n", dateHdr.c_str());
    }
  }

  String payload = http.getString();
  http.end();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err) {
    Serial.printf("Weather JSON error: %s\n", err.c_str());
    ledOff();
    return false;
  }
  ledOff();

  currentTemp        = doc["current"]["temperature_2m"].as<float>();
  bool isDay         = doc["current"]["is_day"].as<int>() != 0;
  currentWeatherCode = doc["daily"]["weather_code"][0];
  currentTempMax     = doc["daily"]["temperature_2m_max"][0];
  currentTempMin     = doc["daily"]["temperature_2m_min"][0];
  currentPrecipProb  = doc["daily"]["precipitation_probability_max"][0];
  currentIconCode    = getIconCode(currentWeatherCode, isDay);
  currentTempUnit    = (config.tempUnits == 0) ? 'F' : 'C';

  // Parse current.time "YYYY-MM-DDTHH:MM" → date "DD.MM.YY" + hour/minute
  const char* reportTime = doc["current"]["time"];
  if (reportTime && strlen(reportTime) == 16) {
    snprintf(currentForecastDate, sizeof(currentForecastDate),
             "%c%c.%c%c.%c%c",
             reportTime[8],  reportTime[9],   // DD
             reportTime[5],  reportTime[6],   // MM
             reportTime[2],  reportTime[3]);  // YY
    currentReportHour = (reportTime[11] - '0') * 10 + (reportTime[12] - '0');
    currentReportMin  = (reportTime[14] - '0') * 10 + (reportTime[15] - '0');
  }

  Serial.printf("WMO %d  now %.1f%c  %.1f...%.1f%c  precip %.0f%%\n",
                currentWeatherCode, currentTemp, currentTempUnit,
                currentTempMin, currentTempMax, currentTempUnit, currentPrecipProb);
  return true;
}

static String urlEncode(const char* s) {
  String out;
  while (*s) {
    unsigned char c = (unsigned char)*s++;
    if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
        (c >= '0' && c <= '9') || c == '-' || c == '_' || c == '.' || c == '~') {
      out += (char)c;
    } else {
      char buf[4];
      snprintf(buf, sizeof(buf), "%%%02X", c);
      out += buf;
    }
  }
  return out;
}

bool fetchManualGeolocation() {
  Serial.printf("Geocoding city: %s\n", config.city);
  ledOn();
  WiFiClient client;
  HTTPClient http;
  http.begin(client, "http://geocoding-api.open-meteo.com/v1/search?name="
             + urlEncode(config.city) + "&count=1&language=en&format=json");
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Geocoding HTTP error: %d\n", code);
    http.end();
    ledOff();
    return false;
  }
  String payload = http.getString();
  http.end();
  ledOff();

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, payload);
  if (err || !doc["results"].is<JsonArray>() || doc["results"].as<JsonArray>().size() == 0) {
    Serial.printf("Geocoding: city '%s' not found\n", config.city);
    return false;
  }

  JsonObject r = doc["results"][0];
  cachedLat = r["latitude"].as<float>();
  cachedLon = r["longitude"].as<float>();
  snprintf(currentLocation, sizeof(currentLocation), "%s, %s, %s",
           r["name"].as<const char*>(),
           r["admin1"].as<const char*>(),
           r["country"].as<const char*>());
  Serial.printf("Geocoded: %s  (%.4f, %.4f)\n", currentLocation, cachedLat, cachedLon);
  return true;
}

// Geo on first call only; time set from HTTP Date header on every weather fetch.
bool updateWeatherData() {
  if (!ensureWiFiConnected()) return false;
  if (!geoCached) {
    if (!fetchGeolocation()) return false;   // always: sets utcOffset + fallback lat/lon
    if (config.city[0] != '\0') {
      if (!fetchManualGeolocation())
        Serial.println("Geocoding failed — using IP-based location");
    }
    geoCached = true;
  }
  if (!fetchWeatherData()) return false;
  lastWeatherUpdate = millis();
  return true;
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  SPI.begin(6, -1, 7, 10); // SCK, MISO (unused=-1), MOSI, CS
  randomSeed(esp_random());

  ConfigManager::load(config);

  // Restore system clock from RTC memory (saved before the last sleep)
  if (rtcSavedTime > 0) {
    struct timeval tv = { (time_t)(rtcSavedTime + rtcSleepSecs), 0 };
    settimeofday(&tv, nullptr);
    Serial.printf("Clock restored: epoch %lld + %us\n", (long long)rtcSavedTime, rtcSleepSecs);
  }

  display.init(115200, true, 2, false);

  // Init hardware pins from config
  if (config.ledPin != -1)    { pinMode(config.ledPin,    OUTPUT);       ledOff(); }
  if (config.buttonPin != -1) { pinMode(config.buttonPin, INPUT_PULLUP);           }

  // Build AP name from MAC (used for both portal trigger check and portal itself)
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char apName[24];
  snprintf(apName, sizeof(apName), "InkCast-%02X%02X", mac[4], mac[5]);

  // Enter config portal if: no WiFi credentials saved, or button held at boot
  bool forcePortal = (config.buttonPin != -1 && digitalRead(config.buttonPin) == LOW);
  if (!ConfigManager::isConfigured() || forcePortal) {
    displayPortalInfo(apName);
    display.hibernate();
    ledOn();                          // steady = portal active
    runConfigPortal(config, apName);  // blocks until reboot
    return;
  }

  // Connect to WiFi
  Serial.printf("Connecting to %d configured network(s)...\n", config.wifiCount);
  WiFi.setHostname(apName);
  if (!connectWiFi()) {
    ledBlink(3, 300, 200);
    Serial.println("WiFi failed — opening AP for recovery");

    // Open AP alongside STA so the user can fix settings while we keep retrying.
    WiFi.setAutoReconnect(false);  // prevent background reconnect storms disrupting AP
    WiFi.disconnect(false);        // stop current STA attempt, keep stack alive
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(apName);
    delay(500);                  // let the AP interface fully initialise
    IPAddress apIP = WiFi.softAPIP();

    char netLine[64], apFooter[64];
    snprintf(netLine,   sizeof(netLine),   "WiFi: %d/%d networks", config.wifiCount, WIFI_MAX);
    snprintf(apFooter,  sizeof(apFooter),  "%s | %s",  apName, apIP.toString().c_str());
    displayNetworkError("WiFi failed", netLine, apFooter);
    display.hibernate();
    ledOn();  // steady = portal active

    startConfigServer(config);

    DNSServer dns;
    dns.start(53, "*", apIP);

    int lastWifiCount = config.wifiCount;

    unsigned long nextRetry = millis() + 30000UL;
    while (true) {
      dns.processNextRequest();
      handleConfigServer();
      // Refresh display and retry sooner if user added/removed a network
      if (config.wifiCount != lastWifiCount) {
        lastWifiCount = config.wifiCount;
        snprintf(netLine, sizeof(netLine), "WiFi: %d/%d networks", config.wifiCount, WIFI_MAX);
        displayNetworkError("WiFi failed", netLine, apFooter);
        display.hibernate();
        nextRetry = millis() + 5000UL;
      }
      if (millis() >= nextRetry) {
        Serial.printf("Retrying WiFi (%d networks)...\n", config.wifiCount);
        // Attempt each configured network in turn; service portal/DNS between tries
        bool recovered = false;
        for (int ni = 0; ni < config.wifiCount && !recovered; ni++) {
          WiFi.begin(config.wifi[ni].ssid, config.wifi[ni].pass);
          delay(100);  // let STA start without disrupting AP
          for (int i = 0; i < 20 && WiFi.status() != WL_CONNECTED; i++) {
            dns.processNextRequest();
            handleConfigServer();
            delay(500);
          }
          if (WiFi.status() == WL_CONNECTED) recovered = true;
          else { WiFi.disconnect(false); delay(100); }
        }
        if (recovered) {
          Serial.println("WiFi recovered — restarting");
          ESP.restart();
        }
        nextRetry = millis() + 30000UL;
      }
      delay(10);
    }
  }
  Serial.printf("WiFi OK — %s\n", WiFi.localIP().toString().c_str());

  // Start config server (runs in background; in sleep mode only active during windows)
  startConfigServer(config);

  // Each fetch attempt: try once, wait 10 s, try once more before giving up.
  auto tryFetchTwice = []() -> bool {
    if (updateWeatherData()) return true;
    ledBlink(3, 300, 200);
    Serial.println("Fetch failed, quick retry in 10 s");
    delay(10000);
    return updateWeatherData();
  };

  if (tryFetchTwice()) {
    fetchRetryIndex = 0;
    ledBlink(2, 80, 80);
    displayWeather();
  } else {
    ledBlink(3, 300, 200);
    if (!everDisplayed) displayNetworkError("No weather data", "Check connection");
    if (config.deepSleepMins > 0) {
      display.hibernate();
      deepSleepRetry();   // never returns
      return;
    }
    // Stay-awake mode: spin with exponential back-off, two tries each round
    unsigned long retryMs = (unsigned long)random(60, 300) * 1000UL;
    while (true) {
      Serial.printf("Fetch failed, retrying in %lus\n", retryMs / 1000);
      unsigned long waitUntil = millis() + retryMs;
      while (millis() < waitUntil) {
        handleConfigServer();
        delay(100);
      }
      if (tryFetchTwice()) {
        ledBlink(2, 80, 80);
        displayWeather();
        break;
      }
      ledBlink(3, 300, 200);
      retryMs = min(retryMs * 2, 30UL * 60UL * 1000UL);
    }
  }

  display.hibernate();

  // Config portal window — shown after display refresh so the user knows when to act.
  // Fast LED blink signals the window is open. Button press extends to 60 s.
  if (config.deepSleepMins > 0) {
    unsigned long windowEnd = millis() + 5000UL;
    Serial.printf("Config portal: http://%s/ (5 s window)\n", WiFi.localIP().toString().c_str());
    while (millis() < windowEnd) {
      // Fast blink: 50 ms on / 50 ms off
      ledOn();  delay(50);
      ledOff(); delay(50);
      handleConfigServer();
      if (config.buttonPin != -1 && digitalRead(config.buttonPin) == LOW) {
        windowEnd = millis() + 60000UL;
        Serial.println("Button pressed — portal extended to 60 s");
        while (digitalRead(config.buttonPin) == LOW) delay(10);
      }
      if (configServerPageServed()) {
        windowEnd = millis() + 60000UL;
        Serial.println("Page served — portal extended to 60 s");
      }
    }
    ledOff();
  }

  deepSleep();
}

void loop() {
  // Only reached when deep sleep is disabled (deepSleepMins == -1)

  handleConfigServer();

  // Periodic weather refresh with exponential backoff on failure
  {
    static unsigned long fetchRetryMs = 0;  // 0 = use normal interval
    static unsigned long nextRetryMs  = 0;

    unsigned long now      = millis();
    unsigned long interval = (unsigned long)config.updateInterval * 60UL * 1000UL;
    bool timeToFetch = (fetchRetryMs > 0)
                       ? (now >= nextRetryMs)
                       : (now - lastWeatherUpdate >= interval);

    if (timeToFetch) {
      if (updateWeatherData()) {
        displayWeather();
        display.hibernate();
        fetchRetryMs = 0;
      } else {
        ledBlink(3, 300, 200);
        if (fetchRetryMs == 0)
          fetchRetryMs = (unsigned long)random(60, 300) * 1000UL;
        else
          fetchRetryMs = min(fetchRetryMs * 2, 30UL * 60UL * 1000UL);
        nextRetryMs = millis() + fetchRetryMs;
        Serial.printf("Fetch failed, retry in %lus\n", fetchRetryMs / 1000);
      }
    }
  }

  // Button: re-enter config portal or force an immediate refresh
  if (config.buttonPin != -1 && digitalRead(config.buttonPin) == LOW) {
    delay(50);
    unsigned long held = millis();
    while (digitalRead(config.buttonPin) == LOW) {
      delay(10);
      // Hold for 5 s → enter config portal
      if (millis() - held > 5000) {
        uint8_t mac[6];
        WiFi.macAddress(mac);
        char apName[24];
        snprintf(apName, sizeof(apName), "InkCast-%02X%02X", mac[4], mac[5]);
        displayPortalInfo(apName);
        display.hibernate();
        runConfigPortal(config, apName);
        return;
      }
    }
    // Short press → immediate refresh
    if (updateWeatherData())
      displayWeather();
    display.hibernate();
  }

  delay(100);
}
