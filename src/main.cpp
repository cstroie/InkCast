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
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
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

RTC_DATA_ATTR static time_t  rtcSavedTime   = 0;  // epoch saved just before sleep
RTC_DATA_ATTR static uint32_t rtcSleepSecs  = 0;  // sleep duration in seconds
RTC_DATA_ATTR static time_t  rtcLastNtpSync = 0;  // epoch of last NTP sync

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

void deepSleep() {
  if (config.deepSleepMins > 0) {
    uint32_t secs = (uint32_t)config.deepSleepMins * 60u;
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

// Show up to three lines of error text, red, vertically centred.
void displayError(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
  int lines  = 1 + (line2 ? 1 : 0) + (line3 ? 1 : 0);
  int lineH  = 22;
  int startY = (display.height() + lines * lineH) / 2 - (lines - 1) * lineH;

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(8, startY);
    display.print(line1);
    if (line2) { display.setCursor(8, startY + lineH);     display.print(line2); }
    if (line3) { display.setCursor(8, startY + lineH * 2); display.print(line3); }
  } while (display.nextPage());
}

// Show the config-portal SSID and instructions on screen.
void displayPortalInfo(const char* apName) {
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setTextColor(GxEPD_BLACK);

    display.setFont(&FreeSansBold9pt7b);
    display.setCursor(8, 20);
    display.print("Setup mode");

    display.setFont(&FreeSans9pt7b);
    display.setCursor(8, 42);
    display.print("Connect to WiFi network:");

    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(8, 64);
    display.print(apName);

    display.setFont(&FreeSans9pt7b);
    display.setTextColor(GxEPD_BLACK);
    display.setCursor(8, 86);
    display.print("then open  192.168.4.1");
    display.setCursor(8, 108);
    display.print("in your browser");
  } while (display.nextPage());
}

void displayWeather() {
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
    display.setFont(WI_FONT);
    {
      const GFXglyph* g = &WI_FONT->glyph[currentIconCode - WI_FONT->first];
      int16_t ix = ICON_CX - g->xAdvance / 2;
      int16_t iy = ICON_CY - g->yOffset - (int16_t)g->height / 2;
      display.drawChar(ix, iy, currentIconCode, iconColor, GxEPD_WHITE, 1);
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
    display.setFont(WI_SMALL_FONT);
    display.setTextColor(GxEPD_BLACK);
    if (umbrellas > 0) {
      int totalW = umbrellas * 24 - 2;  // last glyph needs no trailing gap
      int x = COL + (display.width() - COL - totalW) / 2;
      for (int i = 0; i < umbrellas; i++) {
        display.drawChar(x, 112, WI_UMBRELLA, GxEPD_BLACK, GxEPD_WHITE, 1);
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
  ledOff();
  return true;
}

void syncNTP() {
  configTime(cachedUtcOffset, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("NTP sync");
  struct tm t;
  int tries = 0;
  while (!getLocalTime(&t) && tries++ < 20) {
    ledOn(); delay(100); ledOff(); delay(400);  // fast blink = waiting for NTP
    Serial.print(".");
  }
  ledOff();
  Serial.println();
  if (tries < 20) {
    rtcLastNtpSync = time(nullptr);
    Serial.printf("Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  1900 + t.tm_year, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec);
  } else {
    Serial.println("NTP sync failed");
  }
}

bool fetchWeatherData() {
  Serial.printf("Fetching weather for (%.4f, %.4f)...\n", cachedLat, cachedLon);
  ledOn();  // steady = network busy

  String url = "http://api.open-meteo.com/v1/forecast"
               "?latitude="  + String(cachedLat, 4) +
               "&longitude=" + String(cachedLon, 4) +
               "&current=weather_code,temperature_2m"
               "&daily=weather_code,temperature_2m_max,temperature_2m_min"
               ",precipitation_probability_max"
               "&timezone=auto&forecast_days=" + String(config.forecastDays) +
               (config.tempUnits == 0
                 ? "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch"
                 : "&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm");

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error: %d\n", code);
    http.end();
    ledOff();
    return false;
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
  currentWeatherCode = doc["daily"]["weather_code"][0];
  currentTempMax     = doc["daily"]["temperature_2m_max"][0];
  currentTempMin     = doc["daily"]["temperature_2m_min"][0];
  currentPrecipProb  = doc["daily"]["precipitation_probability_max"][0];
  currentIconCode    = getIconCode(currentWeatherCode);
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

// Geo + NTP on first call only; NTP re-synced once per day; weather every call.
bool updateWeatherData() {
  if (!geoCached) {
    if (!fetchGeolocation()) return false;
    syncNTP();
    geoCached = true;
  } else if (time(nullptr) - rtcLastNtpSync > 28800) {
    syncNTP();
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
  Serial.printf("Connecting to %s", config.wifiSsid);
  WiFi.setHostname(apName);
  WiFi.begin(config.wifiSsid, config.wifiPassword);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 20) {
    ledOn(); delay(250); ledOff(); delay(250);  // slow blink = connecting
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() != WL_CONNECTED) {
    ledBlink(3, 300, 200);  // 3 long flashes = error
    display.hibernate();
    if (config.deepSleepMins > 0) {
      deepSleepRetry();   // never returns
    } else {
      displayError("WiFi failed", config.wifiSsid);
    }
    return;
  }
  Serial.printf("WiFi OK — %s\n", WiFi.localIP().toString().c_str());

  // Start background config server so settings are always reachable
  if (config.deepSleepMins == -1)
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
