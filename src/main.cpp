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

// select the display class and display driver class in the following file (new style):
#include "display.h"

// Create a simple 296x128 test image with black, white, and red pixels
void displayImage() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
  display.firstPage();
  do
  {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Draw some colored rectangles to demonstrate all three colors
    // Left third - Black
    display.fillRect(0, 0, 98, 128, GxEPD_BLACK);
    
    // Middle third - Red
    display.fillRect(98, 0, 98, 128, GxEPD_RED);
    
    // Right third - White (already filled, but showing for clarity)
    display.fillRect(196, 0, 100, 128, GxEPD_WHITE);
    
    // Draw some patterns to make it more interesting
    for (int i = 0; i < 128; i += 8) {
      display.fillRect(30, i, 4, 4, GxEPD_RED);
      display.fillRect(130, i, 4, 4, GxEPD_BLACK);
      display.fillRect(230, i, 4, 4, GxEPD_BLACK);
    }
  }
  while (display.nextPage());
}

void setup()
{
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  displayImage(); // Display the image instead of text
  display.hibernate();
}

void loop() {};
