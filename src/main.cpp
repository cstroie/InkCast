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

#if defined(ESP32)
#include "SPIFFS.h"
#elif defined(ESP8266)
#include "FS.h"
#endif

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#endif

// select the display class and display driver class in the following file (new style):
#include "display.h"


void listPBMFiles() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
  display.firstPage();
  do
  {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Display title
    display.setCursor(20, 30);
    display.print("PBM Files in SPIFFS:");
    
    // List PBM files
    int yPosition = 50;
    
#if defined(ESP32)
    File root = SPIFFS.open("/");
    if (root) {
      File file = root.openNextFile();
      while (file) {
        String fileName = file.name();
        if (fileName.endsWith(".pbm")) {
          display.setCursor(20, yPosition);
          display.print(fileName.c_str());
          yPosition += 20;
        }
        file = root.openNextFile();
      }
      root.close();
    }
#elif defined(ESP8266)
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      if (fileName.endsWith(".pbm")) {
        display.setCursor(20, yPosition);
        display.print(fileName.c_str());
        yPosition += 20;
      }
    }
#endif
    
    if (yPosition == 50) {
      display.setCursor(20, 50);
      display.print("No PBM files found");
    }
  }
  while (display.nextPage());
}

void displayImage() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
  display.firstPage();
  do
  {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Display a simple message instead
    display.setCursor(20, 50);
    display.print("PBM Image Loader");
  }
  while (display.nextPage());
}

void setup()
{
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  
  // Initialize SPIFFS
#if defined(ESP32)
  if (!SPIFFS.begin(true)) {
#elif defined(ESP8266)
  if (!SPIFFS.begin()) {
#endif
    display.setRotation(1);
    display.setFullWindow();
    display.firstPage();
    do
    {
      display.fillScreen(GxEPD_WHITE);
      display.setCursor(20, 50);
      display.print("SPIFFS Mount Failed");
    }
    while (display.nextPage());
    display.hibernate();
    return;
  }
  
  listPBMFiles(); // List all PBM files in SPIFFS
  display.hibernate();
}

void loop() {};
