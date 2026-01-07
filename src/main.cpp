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
#include <TJpg_Decoder.h>
#include "display.h"
#include "weather.h"
#include "icons.h"

// Forward declarations for helper functions
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



/**
 * Setup function - initializes the system, connects to WiFi, and displays weather
 */
void setup() {
  Serial.begin(115200);
  // Initialize random seed with better entropy
#if defined(ESP32)
  randomSeed(esp_random());
#elif defined(ESP8266)
  randomSeed(RANDOM_REG32);
#else
  randomSeed(analogRead(0));
#endif
  
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
#if defined(ESP8266)
    // Go to deep sleep
    ESP.deepSleep(0); 
#endif
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
#if defined(ESP8266)
  // Go to deep sleep
  ESP.deepSleep(0); 
#endif
  return;
#endif
  
  // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  //display.init(115200); 
  // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  display.init(115200, true, 2, false);
  
  // Initialize LED pin if configured
#if CONFIG_LOADED && defined(LED_PIN) && LED_PIN != -1
  pinMode(LED_PIN, OUTPUT);
  ledOff(); // Start with LED off
#endif
  
  // Fetch and display weather information
  Serial.println("Fetching weather information...");
  WeatherData weather;
  
  // Turn on LED while fetching
  ledOn();
  
  if (getWeatherData(weather)) {
    Serial.println("Weather data fetched successfully, displaying...");
    displayWeather(weather);
  } else {
    Serial.println("Failed to fetch weather data, displaying error...");
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.setFont(&FreeMonoBold12pt7b);
      display.setCursor(20, 30);
      display.print("Weather Fetch Failed");
      display.setCursor(20, 60);
      display.print("Check API key and");
      display.setCursor(20, 90);
      display.print("network connectivity");
    }
    while (display.nextPage());
  }
  
  display.hibernate();
  
  // Turn off LED
  ledOff();
  
#if defined(ESP8266) || defined(ESP32)
  // Go to deep sleep
#if CONFIG_LOADED
  if (DEEP_SLEEP_DURATION != -1) {
#if defined(ESP8266)
    ESP.deepSleep(DEEP_SLEEP_DURATION); 
#elif defined(ESP32)
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_DURATION);
    esp_deep_sleep_start();
#endif
  }
#else
#if defined(ESP8266)
  ESP.deepSleep(0); 
#elif defined(ESP32)
  esp_deep_sleep_start();
#endif
#endif
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
    Serial.println("Button pressed, fetching new weather data...");
    
    // Debounce delay
    delay(50);
    
    // Wait for button release
    while (digitalRead(BUTTON_PIN) == LOW) {
      delay(10);
    }
    
    // Fetch and display new weather data
    WeatherData weather;
    
    // Turn on LED while fetching
    ledOn();
    
    if (getWeatherData(weather)) {
      Serial.println("Weather data fetched successfully, displaying...");
      displayWeather(weather);
    } else {
      Serial.println("Failed to fetch weather data, displaying error...");
      display.setRotation(1);
      display.setFullWindow();
      display.firstPage();
      do {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(20, 50);
        display.print("Weather Fetch Failed");
        display.setCursor(20, 80);
        display.print("Check API key and");
        display.setCursor(20, 110);
        display.print("network connectivity");
      } while (display.nextPage());
    }
    
    display.hibernate();
    
    // Turn off LED
    ledOff();
  }
  
  // Small delay to prevent excessive CPU usage
  delay(100);
#endif
}
