// GxEPD2_HelloWorld.ino by Jean-Marc Zingg
//
// Display Library example for SPI e-paper panels from Dalian Good Display and boards from Waveshare.
// Requires HW SPI and Adafruit_GFX. Caution: the e-paper panels require 3.3V supply AND data lines!
//
// Added functionality to fetch and display PBM images from web server
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
#include <WiFi.h>
#include <HTTPClient.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#endif

// select the display class and display driver class in the following file (new style):
#include "display.h"


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

bool fetchAndDisplayPBM(const char* url) {
  #if defined(ESP32) || defined(ESP8266)
  HTTPClient http;
  http.begin(url);
  int httpCode = http.GET();
  
  if (httpCode != HTTP_CODE_OK) {
    http.end();
    return false;
  }
  
  WiFiClient *stream = http.getStreamPtr();
  
  // Skip PBM header (P4 format for binary)
  char header[16];
  stream->readBytes(header, 2); // Read "P4"
  
  // Skip comments and whitespace
  while (stream->available()) {
    char c = stream->read();
    if (c == '#') {
      // Skip comment line
      while (stream->available() && stream->read() != '\n');
    } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      continue;
    } else {
      // Put back the character
      stream->peek(); // This doesn't actually put back, but we'll handle it differently
      break;
    }
  }
  
  // Read width and height
  int width = 0, height = 0;
  char numStr[10];
  int i = 0;
  
  // Read width
  while (stream->available() && i < sizeof(numStr)-1) {
    char c = stream->read();
    if (c == ' ' || c == '\n' || c == '\r') {
      numStr[i] = '\0';
      width = atoi(numStr);
      break;
    }
    numStr[i++] = c;
  }
  
  // Read height
  i = 0;
  while (stream->available() && i < sizeof(numStr)-1) {
    char c = stream->read();
    if (c == ' ' || c == '\n' || c == '\r') {
      numStr[i] = '\0';
      height = atoi(numStr);
      break;
    }
    numStr[i++] = c;
  }
  
  // Skip any remaining whitespace
  while (stream->available()) {
    char c = stream->read();
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      // Put back the character
      stream->peek();
      break;
    }
  }
  
  // Check if dimensions match our display
  if (width != 296 || height != 128) {
    http.end();
    return false;
  }
  
  // Calculate buffer size (1 bit per pixel)
  int bufferSize = (width * height + 7) / 8;
  uint8_t* buffer = (uint8_t*)malloc(bufferSize);
  
  if (!buffer) {
    http.end();
    return false;
  }
  
  // Read image data
  int bytesRead = stream->readBytes(buffer, bufferSize);
  
  if (bytesRead != bufferSize) {
    free(buffer);
    http.end();
    return false;
  }
  
  // Display the image
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, buffer, width, height, GxEPD_BLACK);
  }
  while (display.nextPage());
  
  free(buffer);
  http.end();
  return true;
  #else
  // Not supported on non-ESP platforms
  return false;
  #endif
}

void setup()
{
  //display.init(115200); // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  display.init(115200, true, 2, false); // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  
  #if defined(ESP32) || defined(ESP8266)
  // Connect to WiFi (you need to provide your own credentials)
  // WiFi.begin("your_ssid", "your_password");
  // while (WiFi.status() != WL_CONNECTED) {
  //   delay(1000);
  // }
  
  // Try to fetch and display the PBM image
  if (!fetchAndDisplayPBM("http://eridu.eu.org/test.pbm")) {
    displayImage(); // Fallback to the original bitmap if fetch fails
  }
  #else
  displayImage(); // Display the original bitmap on non-ESP platforms
  #endif
  
  display.hibernate();
}

void loop() {};
