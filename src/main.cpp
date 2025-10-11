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

// 20x20 pixel bitmap data (1-bit per pixel)
// This creates a simple pattern with a red border and black center
const uint8_t bitmap_data[] PROGMEM = {
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 1: 11111111 11111111 11111111 11111111 (white border)
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 2: 11111111 11111111 11111111 11111111 (white border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 3: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 4: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 5: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 6: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 7: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 8: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 9: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 10: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 11: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 12: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 13: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 14: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 15: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 16: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 17: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 18: 11111111 11111111 11111111 11111111 (white border)
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 19: 11111111 11111111 11111111 11111111 (white border)
  0xFF, 0xFF, 0xFF, 0xFF   // Row 20: 11111111 11111111 11111111 11111111 (white border)
};

// For 3-color displays, we need a color bitmap as well
// This defines which pixels should be red (1 = red, 0 = follow bitmap_data)
const uint8_t color_data[] PROGMEM = {
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 1: 11111111 11111111 11111111 11111111 (red border)
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 2: 11111111 11111111 11111111 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 3: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 4: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 5: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 6: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 7: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 8: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 9: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 10: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 11: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 12: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 13: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 14: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 15: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 16: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0x00, 0x00, 0xFF,  // Row 17: 11111111 00000000 00000000 11111111 (red border)
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 18: 11111111 11111111 11111111 11111111 (red border)
  0xFF, 0xFF, 0xFF, 0xFF,  // Row 19: 11111111 11111111 11111111 11111111 (red border)
  0xFF, 0xFF, 0xFF, 0xFF   // Row 20: 11111111 11111111 11111111 11111111 (red border)
};

void displayImage() {
  display.setRotation(1); // Set rotation to match display orientation
  display.setFullWindow();
  display.firstPage();
  do
  {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Draw the bitmap centered on the display
    int x = (display.width() - 20) / 2;
    int y = (display.height() - 20) / 2;
    
    // Draw the bitmap with black/white pixels
    display.drawBitmap(x, y, bitmap_data, 20, 20, GxEPD_BLACK);
    
    // Draw the color overlay for red pixels
    display.drawBitmap(x, y, color_data, 20, 20, GxEPD_RED);
  }
  while (display.nextPage());
}

void setup()
{
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  displayImage(); // Display the bitmap image
  display.hibernate();
}

void loop() {};
