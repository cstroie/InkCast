/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2025 Costin Stroie <costinstroie@eridu.eu.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

// GxEPD2_HelloWorld.ino by Jean-Marc Zingg
//
// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Display Library based on Demo Example from Good Display: https://www.good-display.com/companyfile/32/
//
// Author: Jean-Marc Zingg
//
// Version: see library.properties
//
// Library: https://github.com/ZinggJM/GxEPD2

// Supporting Arduino Forum Topics (closed, read only):
// Good Display ePaper for Arduino: https://forum.arduino.cc/t/good-display-epaper-for-arduino/419657
// Waveshare e-paper displays with SPI: https://forum.arduino.cc/t/waveshare-e-paper-displays-with-spi/467865
//
// Add new topics in https://forum.arduino.cc/c/using-arduino/displays/23 for new questions and issues

// see GxEPD2_wiring_examples.h for wiring suggestions and examples
// if you use a different wiring, you need to adapt the constructor parameters!

// uncomment next line to use class GFX of library GFX_Root instead of Adafruit_GFX
//#include <GFX.h>

#include <GxEPD2_BW.h>
#include <GxEPD2_3C.h>
#include <GxEPD2_4C.h>
#include <GxEPD2_7C.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include "display.h"

/**
 * Turn on the built-in LED if configured
 */
void ledOn();

/**
 * Turn off the built-in LED if configured
 */
void ledOff();

// Include configuration file (rename config.tpl to config.h)
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

// Include filesystem support
#include "SPIFFS.h"
// Include WiFi support
#include <WiFi.h>
// Include HTTP client support
#include <HTTPClient.h>

#include <stdlib.h>
#include <ArduinoJson.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TimeLib.h>
#include "weathericons.h"


// Weather station variables
String currentLocation = "";
uint16_t currentIconCode = WI_NA;
int   currentWeatherCode = 0;
float currentTempMax    = 0.0f;
float currentTempMin    = 0.0f;
float currentPrecipProb = 0.0f;
char  currentTempUnit   = 'C';
unsigned long lastWeatherUpdate = 0;

bool isSevereWeather(int code) {
  return (code == 56 || code == 57 ||
          code == 66 || code == 67 ||
          code == 75 || code == 82 ||
          code == 86 ||
          code == 95 || code == 96 || code == 99);
}

/**
 * Get geolocation data from ip-api.com
 */
String getGeolocation() {
  Serial.println("Fetching geolocation data from ip-api.com...");
  HTTPClient http;
  String url = "http://ip-api.com/json/?fields=status,message,country,countryCode,region,regionName,city,zip,lat,lon,timezone,offset";

  Serial.print("Connecting to: ");
  Serial.println(url);

  http.begin(url);
  int httpCode = http.GET();

  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.print("Received payload: ");
      Serial.println(payload);

      JsonDocument doc;
      deserializeJson(doc, payload);

      if (doc["status"] == "success") {
        String location = String(doc["city"].as<const char*>()) + ", " +
                         String(doc["regionName"].as<const char*>()) + ", " +
                         String(doc["country"].as<const char*>());
        String lat    = String(doc["lat"].as<float>());
        String lon    = String(doc["lon"].as<float>());
        long   offset = doc["offset"].as<long>();

        Serial.print("Geolocation: ");
        Serial.println(location);
        Serial.printf("Coordinates: %s, %s  UTC offset: %ld s\n",
                      lat.c_str(), lon.c_str(), offset);

        http.end();
        return location + "|" + lat + "|" + lon + "|" + String(offset);
      } else {
        Serial.print("Geolocation API error: ");
        Serial.println(doc["message"].as<const char*>());
      }
    }
  } else {
    Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return "";
}

/**
 * Get weather data from Open-Meteo API
 */
String getWeatherData(float lat, float lon) {
  Serial.println("Fetching weather data from Open-Meteo API...");
  Serial.print("Coordinates: ");
  Serial.print(lat, 6);
  Serial.print(", ");
  Serial.println(lon, 6);

  HTTPClient http;
  String url = "https://api.open-meteo.com/v1/forecast";

  String params = "latitude=" + String(lat, 6) + "&longitude=" + String(lon, 6) +
                  "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_probability_max" +
                  "&timezone=auto&forecast_days=1";

#if WEATHER_UNITS == 0
  params += "&temperature_unit=fahrenheit&wind_speed_unit=mph&precipitation_unit=inch";
  Serial.println("Using imperial units");
#else
  params += "&temperature_unit=celsius&wind_speed_unit=kmh&precipitation_unit=mm";
  Serial.println("Using metric units");
#endif

  String fullUrl = url + "?" + params;
  Serial.print("Request URL: ");
  Serial.println(fullUrl);

  http.begin(fullUrl);
  int httpCode = http.GET();

  Serial.print("HTTP Response Code: ");
  Serial.println(httpCode);

  if (httpCode > 0) {
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      Serial.print("Received weather data: ");
      Serial.println(payload);
      http.end();
      return payload;
    } else {
      Serial.print("Weather API error, HTTP code: ");
      Serial.println(httpCode);
    }
  } else {
    Serial.printf("HTTP error: %s\n", http.errorToString(httpCode).c_str());
  }

  http.end();
  return "";
}

/**
 * Parse weather data and update display variables
 */
void updateWeatherData() {
  String geoData = getGeolocation();
  if (geoData == "") {
    Serial.println("Failed to get geolocation data");
    return;
  }

  int firstPipe  = geoData.indexOf('|');
  int secondPipe = geoData.indexOf('|', firstPipe + 1);
  int thirdPipe  = geoData.indexOf('|', secondPipe + 1);

  if (firstPipe == -1 || secondPipe == -1 || thirdPipe == -1) {
    Serial.println("Invalid geolocation data format");
    return;
  }

  currentLocation = geoData.substring(0, firstPipe);
  float lat       = geoData.substring(firstPipe + 1, secondPipe).toFloat();
  float lon       = geoData.substring(secondPipe + 1, thirdPipe).toFloat();
  long  utcOffset = geoData.substring(thirdPipe + 1).toInt();

  // Sync NTP using the local UTC offset (ip-api returns current offset incl. DST)
  configTime(utcOffset, 0, "pool.ntp.org", "time.nist.gov");
  Serial.print("Waiting for NTP sync");
  struct tm timeinfo;
  int attempts = 0;
  while (!getLocalTime(&timeinfo) && attempts++ < 20) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  if (attempts < 20)
    Serial.printf("Time synced: %04d-%02d-%02d %02d:%02d:%02d\n",
                  1900 + timeinfo.tm_year, timeinfo.tm_mon + 1, timeinfo.tm_mday,
                  timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  else
    Serial.println("NTP sync failed");

  String weatherData = getWeatherData(lat, lon);
  if (weatherData == "") {
    Serial.println("Failed to get weather data");
    return;
  }

  JsonDocument doc;
  deserializeJson(doc, weatherData);

  currentWeatherCode = doc["daily"]["weather_code"][0];
  currentTempMax     = doc["daily"]["temperature_2m_max"][0];
  currentTempMin     = doc["daily"]["temperature_2m_min"][0];
  currentPrecipProb  = doc["daily"]["precipitation_probability_max"][0];
  currentIconCode    = getIconCode(currentWeatherCode);

  #ifdef WEATHER_UNITS
  currentTempUnit = (WEATHER_UNITS == 0) ? 'F' : 'C';
  #else
  currentTempUnit = 'C';
  #endif

  Serial.println("Weather data updated");
  Serial.println("Location: " + currentLocation);
  Serial.printf("WMO code: %d  Icon: 0x%04X\n", currentWeatherCode, currentIconCode);
  Serial.printf("Temp: %.1f / %.1f %c  Precip: %.0f%%\n",
                currentTempMax, currentTempMin, currentTempUnit, currentPrecipProb);

  lastWeatherUpdate = millis();
}

/**
 * Display weather information on the e-paper screen
 *
 * Layout (296x128, landscape):
 *   Left third  (x=0..97):   weather icon, vertically centred
 *   Right two-thirds (x=102..295):
 *     header  – city name, right-aligned
 *     main    – max temp (large), min temp, precip probability
 *     footer  – date and time, small font
 */
void displayWeather() {
  static const int COL = 142;  // left edge of text column (clear of 48pt icon + breathing room)

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

    // --- Icon: left third, vertically centred ---
    // 48pt glyphs are ~96px tall; baseline at y=100 centres them in 128px
    display.setFont(WI_FONT);
    display.drawChar(12, 100, currentIconCode, iconColor, GxEPD_WHITE, 1);

    display.setTextColor(GxEPD_BLACK);

    // --- Header: city name bold, right-aligned ---
    int firstComma = currentLocation.indexOf(',');
    String city = (firstComma != -1) ? currentLocation.substring(0, firstComma) : currentLocation;
    display.setFont(&FreeSansBold9pt7b);
    int16_t cx, cy; uint16_t cw, ch;
    display.getTextBounds(city.c_str(), 0, 0, &cx, &cy, &cw, &ch);
    display.setCursor(display.width() - (int16_t)cw - 2, 16);
    display.print(city);

    // --- Main: max temperature (large) ---
    display.setFont(&FreeSansBold24pt7b);
    char tempMaxStr[10];
    snprintf(tempMaxStr, sizeof(tempMaxStr), "%.1f%c", currentTempMax, currentTempUnit);
    display.setTextColor(tempColor);
    display.setCursor(COL, 58);
    display.print(tempMaxStr);

    // Min temperature
    display.setFont(&FreeSans12pt7b);
    display.setTextColor(GxEPD_BLACK);
    char tempMinStr[12];
    snprintf(tempMinStr, sizeof(tempMinStr), "min %.1f%c", currentTempMin, currentTempUnit);
    display.setCursor(COL, 84);
    display.print(tempMinStr);

    // Precipitation (own line)
    char precipStr[14];
    snprintf(precipStr, sizeof(precipStr), "rain %.0f%%", currentPrecipProb);
    display.setCursor(COL, 108);
    display.print(precipStr);

    // --- Footer: "DD.MM.YYYY HH:MM", small built-in font, centred in right area ---
    if (timeOk) {
      char footer[20];
      snprintf(footer, sizeof(footer), "%02d.%02d.%04d %02d:%02d",
               timeinfo.tm_mday, timeinfo.tm_mon + 1, 1900 + timeinfo.tm_year,
               timeinfo.tm_hour, timeinfo.tm_min);
      display.setFont(NULL);          // built-in 6x8 bitmap font
      display.setTextSize(1);
      int16_t fx, fy; uint16_t fw, fh;
      display.getTextBounds(footer, 0, 0, &fx, &fy, &fw, &fh);
      int centerX = COL + (display.width() - COL - (int16_t)fw) / 2;
      display.setCursor(centerX, 120);
      display.print(footer);
    }
  } while (display.nextPage());
}

/**
 * Clear the entire e-paper display
 */
void clearDisplay() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
  }
  while (display.nextPage());
}

/**
 * Display "Hello World" message on the e-paper screen
 */
void displayHelloWorld() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do
  {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Display "Hello World" message
    display.setCursor(20, 50);
    display.print("Hello World");
  }
  while (display.nextPage());
}

/**
 * Setup function - initializes the system, connects to WiFi, mounts SPIFFS,
 * and displays an image (either from server or SPIFFS)
 */
void setup() {
  Serial.begin(115200);
  // Initialize random seed with better entropy
randomSeed(esp_random());
  
#if CONFIG_LOADED
  // Connect to WiFi
  Serial.println("Connecting to WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  Serial.println();
  
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to WiFi");
    // Display WiFi error with technical font
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.setFont(&FreeMonoBold12pt7b);
      display.setCursor(20, 50);
      display.print("WiFi Connection Failed");
    }
    while (display.nextPage());
    display.hibernate();
    return;
  }
  
  Serial.println("Connected to WiFi");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
#else
  Serial.println("Configuration file not found. Please rename src/config.tpl to src/config.h and update with your settings.");
  // Display config error with technical font
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(20, 30);
    display.print("Config File Missing");
    display.setCursor(20, 60);
    display.print("Rename config.tpl");
    display.setCursor(20, 90);
    display.print("to config.h and");
    display.setCursor(20, 120);
    display.print("update settings");
  }
  while (display.nextPage());
  display.hibernate();
  return;
#endif
  
  // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  //display.init(115200); 
  // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  display.init(115200, true, 2, false);

  // Display "Hello World" on startup
  displayHelloWorld();

  // Initialize weather station if enabled
#if WEATHER_ENABLED
  Serial.println("Initializing weather station...");

  updateWeatherData();
  if (currentLocation != "") {
    displayWeather();
  }
  display.hibernate();
#if CONFIG_LOADED
  if (DEEP_SLEEP_DURATION != -1) {
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
    esp_deep_sleep_start();
  }
#endif
  return;
#endif

  // Initialize SPIFFS
  Serial.println("Mounting SPIFFS...");
  if (!SPIFFS.begin(true)) {
    Serial.println("Failed to mount SPIFFS");
    // Display SPIFFS error with technical font
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do {
      display.fillScreen(GxEPD_WHITE);
      display.setFont(&FreeMonoBold12pt7b);
      display.setCursor(20, 50);
      display.print("SPIFFS Mount Failed");
    }
    while (display.nextPage());
    display.hibernate();
    return;
  }
  Serial.println("SPIFFS mounted successfully");
  
  // Initialize LED pin if configured
#if CONFIG_LOADED && defined(LED_PIN) && LED_PIN != -1
  pinMode(LED_PIN, OUTPUT);
  ledOff(); // Start with LED off
#endif
  
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do {
    display.fillScreen(GxEPD_WHITE);
    display.setFont(&FreeMonoBold12pt7b);
    display.setCursor(20, 30);
    display.print("No Image Files Found");
    display.setCursor(20, 60);
    display.print("Please add image files");
    display.setCursor(20, 90);
    display.print("to SPIFFS or check");
    display.setCursor(20, 120);
    display.print("server connectivity");
  }
  while (display.nextPage());
  display.hibernate();
  
  // Go to deep sleep
#if CONFIG_LOADED
  if (DEEP_SLEEP_DURATION != -1) {
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
    esp_deep_sleep_start();
  }
#else
  esp_deep_sleep_start();
#endif
}


/**
 * Turn on the built-in LED if configured (active low)
 */
void ledOn() {
#if defined(LED_PIN) && LED_PIN != -1
  digitalWrite(LED_PIN, LOW);
#endif
}

/**
 * Turn off the built-in LED if configured (active low)
 */
void ledOff() {
#if defined(LED_PIN) && LED_PIN != -1
  digitalWrite(LED_PIN, HIGH);
#endif
}

/**
 * Main loop function - handles button presses when deep sleep is disabled
 */
void loop() {
#if CONFIG_LOADED && defined(DEEP_SLEEP_DURATION) && DEEP_SLEEP_DURATION == -1 && defined(BUTTON_PIN) && BUTTON_PIN != -1
  // Check if button is pressed (active low)
  if (digitalRead(BUTTON_PIN) == LOW) {
    Serial.println("Button pressed, fetching new image...");
    
    // Debounce delay
    delay(50);
    
    // Wait for button release
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
    
        // If no image files found, display error
        display.setRotation(1);
        display.setFullWindow();
        display.firstPage();
        do {
          display.fillScreen(GxEPD_WHITE);
          display.setFont(&FreeMonoBold12pt7b);
          display.setCursor(20, 50);
          display.print("No Images Available");
        } while (display.nextPage());
    display.hibernate();
  }
  
  // Small delay to prevent excessive CPU usage
  delay(100);
#endif

#if WEATHER_ENABLED
  // Update weather data periodically
  unsigned long currentTime = millis();
  if (currentTime - lastWeatherUpdate > (WEATHER_UPDATE_INTERVAL * 60 * 1000)) {
    updateWeatherData();
    if (currentLocation != "") {
      displayWeather();
    }
  }
#endif
}
