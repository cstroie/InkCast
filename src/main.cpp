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

// Include configuration file (rename config.tpl to config.h)
#include "config.h"

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

bool displayRandomPBM() {
  // Count PBM files
  int fileCount = 0;
  
#if defined(ESP32)
  File root = SPIFFS.open("/");
  if (!root) return false;
  
  File file = root.openNextFile();
  while (file) {
    String fileName = file.name();
    if (fileName.endsWith(".pbm")) {
      fileCount++;
    }
    file = root.openNextFile();
  }
  root.close();
#elif defined(ESP8266)
  Dir dir = SPIFFS.openDir("/");
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".pbm")) {
      fileCount++;
    }
  }
#endif

  if (fileCount == 0) return false;

  // Select a random file
  int randomIndex = random(fileCount);
  
#if defined(ESP32)
  root = SPIFFS.open("/");
  if (!root) return false;
  
  file = root.openNextFile();
  int currentIndex = 0;
  String selectedFile;
  
  while (file) {
    String fileName = file.name();
    if (fileName.endsWith(".pbm")) {
      if (currentIndex == randomIndex) {
        selectedFile = fileName;
        break;
      }
      currentIndex++;
    }
    file = root.openNextFile();
  }
  root.close();
  
  if (selectedFile.isEmpty()) return false;
  
  // Open the selected file
  File pbmFile = SPIFFS.open(selectedFile, "r");
  if (!pbmFile) return false;
  
  // Skip PBM header (P4 format for binary)
  char header[2];
  pbmFile.readBytes(header, 2); // Read "P4"
  
  // Skip comments and whitespace
  while (pbmFile.available()) {
    char c = pbmFile.read();
    if (c == '#') {
      // Skip comment line
      while (pbmFile.available() && pbmFile.read() != '\n');
    } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      continue;
    } else {
      // Put back the character by seeking back
      pbmFile.seek(pbmFile.position() - 1);
      break;
    }
  }
  
  // Skip width and height (we know it's 296x128)
  while (pbmFile.available()) {
    char c = pbmFile.read();
    if (c == ' ' || c == '\n' || c == '\r') {
      // Skip whitespace after numbers
      continue;
    } else if (c >= '0' && c <= '9') {
      // Skip digits
      continue;
    } else {
      // Put back the character
      pbmFile.seek(pbmFile.position() - 1);
      break;
    }
  }
  
  // Skip any remaining whitespace
  while (pbmFile.available()) {
    char c = pbmFile.read();
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      // Put back the character
      pbmFile.seek(pbmFile.position() - 1);
      break;
    }
  }
  
  // Calculate buffer size (1 bit per pixel for 296x128)
  int bufferSize = (296 * 128 + 7) / 8; // 4736 bytes
  uint8_t* buffer = (uint8_t*)malloc(bufferSize);
  
  if (!buffer) {
    pbmFile.close();
    return false;
  }
  
  // Read image data
  int bytesRead = pbmFile.readBytes((char*)buffer, bufferSize);
  pbmFile.close();
  
  if (bytesRead != bufferSize) {
    free(buffer);
    return false;
  }
  
  // Display the image
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, buffer, 296, 128, GxEPD_BLACK);
  }
  while (display.nextPage());
  
  free(buffer);
  return true;
  
#elif defined(ESP8266)
  dir = SPIFFS.openDir("/");
  int currentIndex = 0;
  String selectedFile;
  
  while (dir.next()) {
    String fileName = dir.fileName();
    if (fileName.endsWith(".pbm")) {
      if (currentIndex == randomIndex) {
        selectedFile = fileName;
        break;
      }
      currentIndex++;
    }
  }
  
  if (selectedFile.isEmpty()) return false;
  
  // Open the selected file
  File pbmFile = SPIFFS.open(selectedFile, "r");
  if (!pbmFile) return false;
  
  // Skip PBM header (P4 format for binary)
  char header[2];
  pbmFile.readBytes(header, 2); // Read "P4"
  
  // Skip comments and whitespace
  while (pbmFile.available()) {
    char c = pbmFile.read();
    if (c == '#') {
      // Skip comment line
      while (pbmFile.available() && pbmFile.read() != '\n');
    } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      continue;
    } else {
      // Put back the character by seeking back
      pbmFile.seek(pbmFile.position() - 1);
      break;
    }
  }
  
  // Skip width and height (we know it's 296x128)
  while (pbmFile.available()) {
    char c = pbmFile.read();
    if (c == ' ' || c == '\n' || c == '\r') {
      // Skip whitespace after numbers
      continue;
    } else if (c >= '0' && c <= '9') {
      // Skip digits
      continue;
    } else {
      // Put back the character
      pbmFile.seek(pbmFile.position() - 1);
      break;
    }
  }
  
  // Skip any remaining whitespace
  while (pbmFile.available()) {
    char c = pbmFile.read();
    if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
      // Put back the character
      pbmFile.seek(pbmFile.position() - 1);
      break;
    }
  }
  
  // Calculate buffer size (1 bit per pixel for 296x128)
  int bufferSize = (296 * 128 + 7) / 8; // 4736 bytes
  uint8_t* buffer = (uint8_t*)malloc(bufferSize);
  
  if (!buffer) {
    pbmFile.close();
    return false;
  }
  
  // Read image data
  int bytesRead = pbmFile.readBytes((char*)buffer, bufferSize);
  pbmFile.close();
  
  if (bytesRead != bufferSize) {
    free(buffer);
    return false;
  }
  
  // Display the image
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    display.drawBitmap(0, 0, buffer, 296, 128, GxEPD_BLACK);
  }
  while (display.nextPage());
  
  free(buffer);
  return true;
#endif
}

void setup() {
  Serial.begin(115200);
  // Initialize random seed
  randomSeed(analogRead(0)); 
  
  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to WiFi");
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
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
  
  // default 10ms reset pulse, e.g. for bare panels with DESPI-C02
  //display.init(115200); 
  // USE THIS for Waveshare boards with "clever" reset circuit, 2ms reset pulse
  display.init(115200, true, 2, false);
  
  // Initialize SPIFFS
#if defined(ESP32)
  if (!SPIFFS.begin(true)) {
#elif defined(ESP8266)
  if (!SPIFFS.begin()) {
#endif
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
  
  // Try to display a random PBM file, fallback to "Hello World" if none found
  if (!displayRandomPBM()) {
    // No PBM files found, try to fetch image from server
    bool imageFetched = false;
    
#if defined(ESP32)
    HTTPClient http;
    http.begin(SERVER_URL);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient *stream = http.getStreamPtr();
      
      // Skip PBM header (P4 format for binary)
      char header[2];
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
          stream->peek();
          break;
        }
      }
      
      // Skip width and height (we know it's 296x128)
      while (stream->available()) {
        char c = stream->read();
        if (c == ' ' || c == '\n' || c == '\r') {
          // Skip whitespace after numbers
          continue;
        } else if (c >= '0' && c <= '9') {
          // Skip digits
          continue;
        } else {
          // Put back the character
          stream->peek();
          break;
        }
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
      
      // Calculate buffer size (1 bit per pixel for 296x128)
      int bufferSize = (296 * 128 + 7) / 8; // 4736 bytes
      uint8_t* buffer = (uint8_t*)malloc(bufferSize);
      
      if (buffer) {
        // Read image data
        int bytesRead = stream->readBytes(buffer, bufferSize);
        
        if (bytesRead == bufferSize) {
          // Display the image
          display.setRotation(1);
          display.setFullWindow();
          display.firstPage();
          do
          {
            display.fillScreen(GxEPD_WHITE);
            display.drawBitmap(0, 0, buffer, 296, 128, GxEPD_BLACK);
          }
          while (display.nextPage());
          
          imageFetched = true;
        }
        free(buffer);
      }
    }
    http.end();
    
#elif defined(ESP8266)
    WiFiClientSecure client;
    client.setInsecure(); // Skip certificate verification for simplicity
    
    HTTPClient http;
    http.begin(client, SERVER_URL);
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
      WiFiClient *stream = http.getStreamPtr();
      
      // Skip PBM header (P4 format for binary)
      char header[2];
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
          stream->peek();
          break;
        }
      }
      
      // Skip width and height (we know it's 296x128)
      while (stream->available()) {
        char c = stream->read();
        if (c == ' ' || c == '\n' || c == '\r') {
          // Skip whitespace after numbers
          continue;
        } else if (c >= '0' && c <= '9') {
          // Skip digits
          continue;
        } else {
          // Put back the character
          stream->peek();
          break;
        }
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
      
      // Calculate buffer size (1 bit per pixel for 296x128)
      int bufferSize = (296 * 128 + 7) / 8; // 4736 bytes
      uint8_t* buffer = (uint8_t*)malloc(bufferSize);
      
      if (buffer) {
        // Read image data
        int bytesRead = stream->readBytes(buffer, bufferSize);
        
        if (bytesRead == bufferSize) {
          // Display the image
          display.setRotation(1);
          display.setFullWindow();
          display.firstPage();
          do
          {
            display.fillScreen(GxEPD_WHITE);
            display.drawBitmap(0, 0, buffer, 296, 128, GxEPD_BLACK);
          }
          while (display.nextPage());
          
          imageFetched = true;
        }
        free(buffer);
      }
    }
    http.end();
#endif
    
    // If we couldn't fetch an image from the server, display instructions
    if (!imageFetched) {
      display.setRotation(1);
      display.setFullWindow();
      display.firstPage();
      do
      {
        display.fillScreen(GxEPD_WHITE);
        display.setFont(&FreeMonoBold12pt7b);
        display.setCursor(20, 30);
        display.print("No PBM Files Found");
        display.setCursor(20, 60);
        display.print("Please add PBM files");
        display.setCursor(20, 90);
        display.print("to SPIFFS or check");
        display.setCursor(20, 120);
        display.print("server connectivity");
      }
      while (display.nextPage());
    }
  }
  display.hibernate();
  
#if defined(ESP8266)
  // Go to deep sleep
  ESP.deepSleep(DEEP_SLEEP_DURATION); 
#endif
}

void loop() {};
