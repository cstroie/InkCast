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
#include "pbm.h"
#include "display.h"

// Forward declarations for helper functions
void listSPIFFSContent();
void fetchAndDisplayImage();
bool displayGIFFile(const char* filename);

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

void listImageFiles() {
  Serial.println("Listing image files in SPIFFS...");
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
  display.firstPage();
  do
  {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Display title
    display.setCursor(20, 30);
    display.print("Image Files in SPIFFS:");
    
    // List image files
    int yPosition = 50;
    int fileCount = 0;
    
#if defined(ESP32)
    File root = SPIFFS.open("/");
    if (root) {
      File file = root.openNextFile();
      while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".pbm") || fileName.endsWith(".gif")) {
          Serial.println("Found image file: " + fileName);
          display.setCursor(20, yPosition);
          display.print(fileName.c_str());
          yPosition += 20;
          fileCount++;
        }
        file = root.openNextFile();
      }
      root.close();
    }
#elif defined(ESP8266)
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      if (fileName.endsWith(".pbm") || fileName.endsWith(".gif")) {
        Serial.println("Found image file: " + fileName);
        display.setCursor(20, yPosition);
        display.print(fileName.c_str());
        yPosition += 20;
        fileCount++;
      }
    }
#endif
    
    if (fileCount == 0) {
      Serial.println("No image files found in SPIFFS");
      display.setCursor(20, 50);
      display.print("No image files found");
    } else {
      Serial.println("Found " + String(fileCount) + " image files in SPIFFS");
    }
  }
  while (display.nextPage());
}

void displayHelloWorld() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
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

bool displayRandomImage() {
  Serial.println("Searching for image files to display...");
  // Count image files
  int fileCount = 0;
  
#if defined(ESP32)
  File root = SPIFFS.open("/");
  if (!root) {
    Serial.println("Failed to open SPIFFS root directory");
    return false;
  }
  
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (fileName.endsWith(".pbm") || fileName.endsWith(".gif")) {
      fileCount++;
    }
    file = root.openNextFile();
  }
  root.close();
#elif defined(ESP8266)
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".pbm") || fileName.endsWith(".gif")) {
      fileCount++;
    }
  }
#endif

  if (fileCount == 0) {
    Serial.println("No image files found for display");
    return false;
  }
  Serial.println("Found " + String(fileCount) + " image files");

  // Select a random file
  int randomIndex = random(fileCount);
  Serial.println("Selected random file index: " + String(randomIndex));
  
#if defined(ESP32)
  root = SPIFFS.open("/");
  if (!root) return false;
  
  file = root.openNextFile();
  int currentIndex = 0;
  String selectedFile;
  
  while (file) {
    String fileName = file.name();
    if (fileName.endsWith(".pbmx") || fileName.endsWith(".gif")) {
      if (currentIndex == randomIndex) {
        selectedFile = fileName;
        break;
      }
      currentIndex++;
    }
    file = root.openNextFile();
  }
  root.close();
  
  if (selectedFile.isEmpty()) {
    Serial.println("Failed to select an image file");
    return false;
  }
  Serial.println("Selected file: " + selectedFile);
  
  // Ensure the file path starts with "/"
  if (!selectedFile.startsWith("/")) {
    selectedFile = "/" + selectedFile;
  }
  
  // Check file extension and handle accordingly
  if (selectedFile.endsWith(".pbm")) {
    // Open the selected file
    File pbmFile = SPIFFS.open(selectedFile, "r");
    if (!pbmFile) {
      Serial.println("Failed to open file: " + selectedFile);
      return false;
    }
    Serial.println("Successfully opened file: " + selectedFile);
    
    // Parse PBM header to get image dimensions
    int width = 0, height = 0;
    if (!parsePBMHeader(&pbmFile, width, height)) {
      Serial.println("Failed to parse PBM header");
      pbmFile.close();
      return false;
    }
    Serial.println("Image dimensions: " + String(width) + "x" + String(height));
    
    // Calculate buffer size (1 bit per pixel)
    int bufferSize = (width * height + 7) / 8;
    Serial.println("Allocating buffer of " + String(bufferSize) + " bytes");
    uint8_t* buffer = (uint8_t*)malloc(bufferSize);
    
    if (!buffer) {
      Serial.println("Failed to allocate memory for image buffer");
      pbmFile.close();
      return false;
    }
    Serial.println("Successfully allocated image buffer");
    
    // Read image data
    Serial.println("Reading image data...");
    if (!readPBMData(&pbmFile, buffer, width, height)) {
      Serial.println("Failed to read PBM data");
      pbmFile.close();
      free(buffer);
      return false;
    }
    pbmFile.close();
    Serial.println("Successfully read image data");
    
    // Display the image
    Serial.println("Displaying image on e-paper...");
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(0, 0, buffer, width, height, GxEPD_BLACK);
    }
    while (display.nextPage());
    Serial.println("Image displayed successfully");
    
    free(buffer);
    return true;
  } else if (selectedFile.endsWith(".gif")) {
    // Handle GIF file
    Serial.println("Displaying GIF file: " + selectedFile);
    
    if (displayGIFFile(selectedFile.c_str())) {
      Serial.println("GIF displayed successfully");
      return true;
    } else {
      Serial.println("Failed to display GIF");
      return false;
    }
  }
  
#elif defined(ESP8266)
  dir = SPIFFS.openDir("/");
  int currentIndex = 0;
  String selectedFile;
  
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".pbm") || fileName.endsWith(".gif")) {
      if (currentIndex == randomIndex) {
        selectedFile = fileName;
        break;
      }
      currentIndex++;
    }
  }
  
  if (selectedFile.isEmpty()) {
    Serial.println("Failed to select an image file");
    return false;
  }
  Serial.println("Selected file: " + selectedFile);
  
  // Check file extension and handle accordingly
  if (selectedFile.endsWith(".pbm")) {
    // Open the selected file
    File pbmFile = SPIFFS.open(selectedFile, "r");
    if (!pbmFile) {
      Serial.println("Failed to open file: " + selectedFile);
      return false;
    }
    Serial.println("Successfully opened file: " + selectedFile);
    
    // Parse PBM header to get image dimensions
    int width = 0, height = 0;
    if (!parsePBMHeader(&pbmFile, width, height)) {
      Serial.println("Failed to parse PBM header");
      pbmFile.close();
      return false;
    }
    Serial.println("Image dimensions: " + String(width) + "x" + String(height));
    
    // Calculate buffer size (1 bit per pixel)
    int bufferSize = (width * height + 7) / 8;
    Serial.println("Allocating buffer of " + String(bufferSize) + " bytes");
    uint8_t* buffer = (uint8_t*)malloc(bufferSize);
    
    if (!buffer) {
      Serial.println("Failed to allocate memory for image buffer");
      pbmFile.close();
      return false;
    }
    Serial.println("Successfully allocated image buffer");
    
    // Read image data
    Serial.println("Reading image data...");
    if (!readPBMData(&pbmFile, buffer, width, height)) {
      Serial.println("Failed to read PBM data");
      pbmFile.close();
      free(buffer);
      return false;
    }
    pbmFile.close();
    Serial.println("Successfully read image data");
    
    // Display the image
    Serial.println("Displaying image on e-paper...");
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.drawBitmap(0, 0, buffer, width, height, GxEPD_RED);
    }
    while (display.nextPage());
    Serial.println("Image displayed successfully");
    
    free(buffer);
    return true;
  } else if (selectedFile.endsWith(".gif")) {
    // Handle GIF file
    Serial.println("Displaying GIF file: " + selectedFile);
    
    if (displayGIFFile(selectedFile.c_str())) {
      Serial.println("GIF displayed successfully");
      return true;
    } else {
      Serial.println("Failed to display GIF");
      return false;
    }
  }
#endif
  
  // Add a default return statement to avoid compiler warning
  return false;
}

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
  
  // List all files in SPIFFS
  Serial.println("SPIFFS content:");
  listSPIFFSContent();
  Serial.println("End of SPIFFS content");
  
  // Try to display a random image file, fallback to "Hello World" if none found
  Serial.println("Attempting to display a random image file...");
  if (!displayRandomImage()) {
    Serial.println("No image files found, trying to fetch image from server...");
    fetchAndDisplayImage();
  }
  display.hibernate();
  
#if defined(ESP8266)
  // Go to deep sleep
#if CONFIG_LOADED
  ESP.deepSleep(DEEP_SLEEP_DURATION); 
#else
  ESP.deepSleep(0); 
#endif
#endif
}

// Helper function to list SPIFFS content
void listSPIFFSContent() {
#if defined(ESP32)
  File root = SPIFFS.open("/");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      Serial.print("File: ");
      Serial.print(file.name());
      Serial.print(" Size: ");
      Serial.println(file.size());
      file = root.openNextFile();
    }
    root.close();
  }
#elif defined(ESP8266)
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    Serial.print("File: ");
    Serial.print(dir.fileName());
    Serial.print(" Size: ");
    File f = SPIFFS.open(dir.fileName(), "r");
    if (f) {
      Serial.println(f.size());
      f.close();
    } else {
      Serial.println("Unknown");
    }
  }
#endif
}

// Helper function to fetch and display image from server
void fetchAndDisplayImage() {
#if CONFIG_LOADED
  Serial.println("Fetching image from server...");
  
#if defined(ESP32)
  HTTPClient http;
  http.begin(SERVER_URL);
  int httpCode = http.GET();
#elif defined(ESP8266)
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification for simplicity
  
  HTTPClient http;
  http.begin(client, SERVER_URL);
  int httpCode = http.GET();
#endif

  if (httpCode == HTTP_CODE_OK) {
    Serial.println("Image downloaded successfully");
    WiFiClient *stream = http.getStreamPtr();
    
    // Parse PBM header to get image dimensions
    int width = 0, height = 0;
    if (!parsePBMHeader(stream, width, height)) {
      Serial.println("Failed to parse PBM header from server");
      http.end();
      return;
    }
    
    Serial.println("Image dimensions: " + String(width) + "x" + String(height));
    
    // Calculate buffer size (1 bit per pixel)
    int bufferSize = (width * height + 7) / 8;
    Serial.println("Allocating buffer of " + String(bufferSize) + " bytes");
    uint8_t* buffer = (uint8_t*)malloc(bufferSize);
    
    if (buffer) {
      // Read image data
      if (readPBMData(stream, buffer, width, height)) {
        // Display the image
        display.setRotation(1);
        display.setFullWindow();
        display.firstPage();
        do
        {
          display.fillScreen(GxEPD_WHITE);
          display.drawBitmap(0, 0, buffer, width, height, GxEPD_RED);
        }
        while (display.nextPage());
        
        Serial.println("Image displayed successfully");
      } else {
        Serial.println("Failed to read PBM data from server");
      }
      free(buffer);
    } else {
      Serial.println("Failed to allocate memory for image buffer");
    }
  }
  http.end();
#else // CONFIG_LOADED
  // If we couldn't fetch an image from the server, display instructions
  Serial.println("Displaying instructions for adding image files...");
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
#endif // CONFIG_LOADED
}

// Display GIF file using TJpg_Decoder
bool displayGIFFile(const char* filename) {
  // Initialize TJpg_Decoder
  TJpgDec.setSwapBytes(true);
  
  // Variable to store color depth information
  static uint8_t colorDepth = 16; // Default to 16-bit
  
  // Callback function for drawing pixels
  auto jpegDrawCallback = [](int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // The bitmap parameter contains 16-bit RGB565 color values
    // This indicates the image is being processed as 16-bit color
    colorDepth = 16;
    
    // Convert 16-bit color to display color
    uint16_t color = bitmap[0];
    uint8_t displayColor;
    
    // Simple color conversion (you might want to improve this)
    if ((color & 0xF800) > 0x8000 || (color & 0x07E0) > 0x0400 || (color & 0x001F) > 0x0010) {
      displayColor = GxEPD_BLACK;
    } else {
      displayColor = GxEPD_WHITE;
    }
    
    // Draw pixel by pixel (simplified approach)
    for (uint16_t i = 0; i < w; i++) {
      for (uint16_t j = 0; j < h; j++) {
        display.drawPixel(x + i, y + j, displayColor);
      }
    }
    return true;
  };
  
  TJpgDec.setCallback(jpegDrawCallback);
  
  // Display the GIF
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    TJpgDec.drawFsJpg(0, 0, filename, SPIFFS);
  }
  while (display.nextPage());
  
  // Print color depth information
  Serial.println("GIF color depth: " + String(colorDepth) + " bits");
  
  return true;
}

void loop() {};
