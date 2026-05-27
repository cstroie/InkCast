/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2025 Costin Stroie <costinstroie@eridu.eu.org>
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

#if defined(__has_include)
  #if __has_include("config.h")
    #include "config.h"
    #define CONFIG_LOADED 1
  #else
    #define CONFIG_LOADED 0
  #endif
#else
  #define CONFIG_LOADED 0
#endif

#include <WiFi.h>
#include "SPIFFS.h"
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "weathericons.h"

// Geolocation — fetched once per power cycle, reused for subsequent loop() refreshes
static float  cachedLat       = 0.0f;
static float  cachedLon       = 0.0f;
static long   cachedUtcOffset = 0;
static bool   geoCached       = false;

// Weather state shown on display
static String   currentLocation   = "";
static uint16_t currentIconCode   = WI_NA;
static int      currentWeatherCode = 0;
static float    currentTempMax    = 0.0f;
static float    currentTempMin    = 0.0f;
static float    currentPrecipProb = 0.0f;
static char     currentTempUnit   = 'C';
static unsigned long lastWeatherUpdate = 0;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

void ledOn();
void ledOff();

void deepSleep() {
#if CONFIG_LOADED
  if (DEEP_SLEEP_DURATION != -1) {
    esp_sleep_enable_timer_wakeup((uint64_t)DEEP_SLEEP_DURATION * 1000000ULL);
    esp_deep_sleep_start();
  }
#endif
}

// Show up to three lines of error text centred vertically on the display.
void displayError(const char* line1, const char* line2 = nullptr, const char* line3 = nullptr) {
  int lines = 1 + (line2 ? 1 : 0) + (line3 ? 1 : 0);
  int lineH  = 22;
  int startY = (display.height() + lines * lineH) / 2 - (lines - 1) * lineH;

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_RED);
    display.setCursor(8, startY);          display.print(line1);
    if (line2) { display.setCursor(8, startY + lineH);     display.print(line2); }
    if (line3) { display.setCursor(8, startY + lineH * 2); display.print(line3); }
  } while (display.nextPage());
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
// Data fetching
// ---------------------------------------------------------------------------

// Fetch geolocation from ip-api.com and populate cachedLat/Lon/UtcOffset +
// currentLocation. Returns true on success.
bool fetchGeolocation() {
  Serial.println("Fetching geolocation...");
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
    Serial.println("Geolocation parse error or API failure");
    return false;
  }

  currentLocation = String(doc["city"].as<const char*>()) + ", " +
                    String(doc["regionName"].as<const char*>()) + ", " +
                    String(doc["country"].as<const char*>());
  cachedLat       = doc["lat"].as<float>();
  cachedLon       = doc["lon"].as<float>();
  cachedUtcOffset = doc["offset"].as<long>();

  Serial.printf("Location: %s  (%.4f, %.4f)  UTC+%lds\n",
                currentLocation.c_str(), cachedLat, cachedLon, cachedUtcOffset);
  return true;
}

// Sync NTP once using the cached UTC offset (includes current DST).
// Called only after a successful geolocation fetch.
void syncNTP() {
  configTime(cachedUtcOffset, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("NTP sync");
  struct tm t;
  int tries = 0;
  while (!getLocalTime(&t) && tries++ < 20) { delay(500); Serial.print("."); }
  Serial.println();
  if (tries < 20)
    Serial.printf("Time: %04d-%02d-%02d %02d:%02d:%02d\n",
                  1900 + t.tm_year, t.tm_mon + 1, t.tm_mday,
                  t.tm_hour, t.tm_min, t.tm_sec);
  else
    Serial.println("NTP sync failed");
}

// Fetch today's forecast from Open-Meteo. Returns true on success.
bool fetchWeatherData() {
  Serial.printf("Fetching weather for (%.4f, %.4f)...\n", cachedLat, cachedLon);

  String url = "https://api.open-meteo.com/v1/forecast"
               "?latitude="  + String(cachedLat, 4) +
               "&longitude=" + String(cachedLon, 4) +
               "&daily=weather_code,temperature_2m_max,temperature_2m_min"
               ",precipitation_probability_max"
               "&timezone=auto&forecast_days=1"
#if WEATHER_UNITS == 0
               "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch";
#else
               "&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm";
#endif

  HTTPClient http;
  http.begin(url);
  int code = http.GET();
  if (code != HTTP_CODE_OK) {
    Serial.printf("Weather HTTP error: %d\n", code);
    http.end();
    return false;
  }

  JsonDocument doc;
  DeserializationError err = deserializeJson(doc, http.getStream());
  http.end();

  if (err) {
    Serial.println("Weather JSON parse error");
    return false;
  }

  currentWeatherCode = doc["daily"]["weather_code"][0];
  currentTempMax     = doc["daily"]["temperature_2m_max"][0];
  currentTempMin     = doc["daily"]["temperature_2m_min"][0];
  currentPrecipProb  = doc["daily"]["precipitation_probability_max"][0];
  currentIconCode    = getIconCode(currentWeatherCode);
#if WEATHER_UNITS == 0
  currentTempUnit    = 'F';
#else
  currentTempUnit    = 'C';
#endif

  Serial.printf("WMO %d  %.1f/%.1f%c  precip %.0f%%\n",
                currentWeatherCode, currentTempMax, currentTempMin,
                currentTempUnit, currentPrecipProb);
  return true;
}

// Orchestrate a full data refresh.
// Geolocation + NTP are fetched only on the first call (or after deep-sleep reset).
// Returns true if weather data is ready to display.
bool updateWeatherData() {
  if (!geoCached) {
    if (!fetchGeolocation()) return false;
    syncNTP();
    geoCached = true;
  }
  if (!fetchWeatherData()) return false;
  lastWeatherUpdate = millis();
  return true;
}

// ---------------------------------------------------------------------------
// Display
// ---------------------------------------------------------------------------

void displayWeather() {
  static const int COL = 142;

  uint16_t iconColor = isSevereWeather(currentWeatherCode) ? GxEPD_RED : GxEPD_BLACK;
  bool hot = (currentTempUnit == 'C') ? (currentTempMax >= 30.0f) : (currentTempMax >= 86.0f);
  uint16_t tempColor = hot ? GxEPD_RED : GxEPD_BLACK;

  struct tm timeinfo;
  bool timeOk = getLocalTime(&timeinfo);

  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);

    // Icon — left column, baseline y=100 (~96px ascent centres in 128px)
    display.setFont(WI_FONT);
    display.drawChar(12, 100, currentIconCode, iconColor, GxEPD_WHITE, 1);

    // Header — city name bold, right-aligned
    int firstComma = currentLocation.indexOf(',');
    String city = (firstComma != -1) ? currentLocation.substring(0, firstComma) : currentLocation;
    display.setFont(&FreeSansBold9pt7b);
    display.setTextColor(GxEPD_BLACK);
    int16_t cx, cy; uint16_t cw, ch;
    display.getTextBounds(city.c_str(), 0, 0, &cx, &cy, &cw, &ch);
    display.setCursor(display.width() - (int16_t)cw - 2, 16);
    display.print(city);

    // Max temperature
    display.setFont(&FreeSansBold24pt7b);
    display.setTextColor(tempColor);
    char tempMaxStr[10];
    snprintf(tempMaxStr, sizeof(tempMaxStr), "%.1f%c", currentTempMax, currentTempUnit);
    display.setCursor(COL, 58);
    display.print(tempMaxStr);

    // Min temperature
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);
    char tempMinStr[12];
    snprintf(tempMinStr, sizeof(tempMinStr), "min %.1f%c", currentTempMin, currentTempUnit);
    display.setCursor(COL, 84);
    display.print(tempMinStr);

    // Precipitation
    char precipStr[14];
    snprintf(precipStr, sizeof(precipStr), "rain %.0f%%", currentPrecipProb);
    display.setCursor(COL, 108);
    display.print(precipStr);

    // Footer — date + time, built-in 6×8 font, centred in text column
    if (timeOk) {
      static const char* wd[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
      char footer[22];
      snprintf(footer, sizeof(footer), "%02d.%02d.%04d %02d:%02d",
               timeinfo.tm_mday, timeinfo.tm_mon + 1, 1900 + timeinfo.tm_year,
               timeinfo.tm_hour, timeinfo.tm_min);
      display.setFont(NULL);
      display.setTextSize(1);
      int16_t fx, fy; uint16_t fw, fh;
      display.getTextBounds(footer, 0, 0, &fx, &fy, &fw, &fh);
      display.setCursor(COL + (display.width() - COL - (int16_t)fw) / 2, 120);
      display.print(footer);
    }
  } while (display.nextPage());
}

// ---------------------------------------------------------------------------
// Arduino entry points
// ---------------------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  randomSeed(esp_random());

#if !CONFIG_LOADED
  display.init(115200, true, 2, false);
  displayError("Config missing —", "rename config.tpl", "to config.h");
  display.hibernate();
  return;
#endif

  // Connect to WiFi before initialising the display (saves one refresh)
  Serial.printf("Connecting to %s", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int tries = 0;
  while (WiFi.status() != WL_CONNECTED && tries++ < 20) { delay(500); Serial.print("."); }
  Serial.println();

  display.init(115200, true, 2, false);

  if (WiFi.status() != WL_CONNECTED) {
    displayError("WiFi failed");
    display.hibernate();
    deepSleep();   // retry after the configured interval
    return;
  }
  Serial.printf("WiFi OK — %s\n", WiFi.localIP().toString().c_str());

#if WEATHER_ENABLED
  if (updateWeatherData())
    displayWeather();
  else
    displayError("Weather unavailable");
  display.hibernate();
  deepSleep();
  return;
#endif

  // --- SPIFFS image fallback (non-weather mode) ---
  if (!SPIFFS.begin(true)) {
    displayError("SPIFFS mount failed");
    display.hibernate();
    deepSleep();
    return;
  }
#if CONFIG_LOADED && defined(LED_PIN) && LED_PIN != -1
  pinMode(LED_PIN, OUTPUT);
  ledOff();
#endif
  displayError("No image files found");
  display.hibernate();
  deepSleep();
}

void loop() {
#if WEATHER_ENABLED
  // Periodic refresh (only reached when deep sleep is disabled)
  if (millis() - lastWeatherUpdate > (unsigned long)WEATHER_UPDATE_INTERVAL * 60UL * 1000UL) {
    if (updateWeatherData())
      displayWeather();
    display.hibernate();
  }

  // Button: immediate refresh
#if CONFIG_LOADED && defined(BUTTON_PIN) && BUTTON_PIN != -1
  if (digitalRead(BUTTON_PIN) == LOW) {
    delay(50);
    while (digitalRead(BUTTON_PIN) == LOW) delay(10);
    if (updateWeatherData())
      displayWeather();
    display.hibernate();
  }
#endif

  delay(100);
#endif
}

// ---------------------------------------------------------------------------
// LED helpers (active-low)
// ---------------------------------------------------------------------------

void ledOn() {
#if CONFIG_LOADED && defined(LED_PIN) && LED_PIN != -1
  digitalWrite(LED_PIN, LOW);
#endif
}

void ledOff() {
#if CONFIG_LOADED && defined(LED_PIN) && LED_PIN != -1
  digitalWrite(LED_PIN, HIGH);
#endif
}
