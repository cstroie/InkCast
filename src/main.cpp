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
#if defined(ESP32)
#include "SPIFFS.h"
#elif defined(ESP8266)
#include "FS.h"
#endif

// Include WiFi support
#if defined(ESP32)
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// Include HTTP client support
#if defined(ESP32)
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#endif

#include <stdlib.h>


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

  // Display "Hello World" on startup
  displayHelloWorld();

  // Initialize SPIFFS
#if defined(ESP32)
  Serial.println("Mounting SPIFFS...");
  if (!SPIFFS.begin(true)) {
#elif defined(ESP8266)
  Serial.println("Mounting SPIFFS...");
  if (!SPIFFS.begin()) {
#endif
    Serial.println("Failed to mount SPIFFS");
    // Display SPIFFS error with technical font
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.setFont(&FreeMonoBold12pt7b);
      display.setCursor(20, 50);
      display.print("SPIFFS Mount Failed");
    }
    while (display.nextPage());
    display.hibernate();
#if defined(ESP8266)
    // Go to deep sleep
    ESP.deepSleep(0); 
#endif
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
      do
      {
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
    }
  display.hibernate();
  
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
}
