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

#include "weather.h"
#include "display.h"
#include "icons.h"

// Get geolocation using ip-api.com
bool getGeolocation(float& latitude, float& longitude, String& location, String& country) {
  Serial.println("Fetching geolocation...");
  
  WiFiClient client;
  HTTPClient http;
  
  http.begin(client, IP_API_URL);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      if (doc["status"] == "success") {
        latitude = doc["lat"];
        longitude = doc["lon"];
        location = doc["city"].as<String>();
        country = doc["country"].as<String>();
        
        Serial.println("Geolocation successful:");
        Serial.println("  Location: " + location + ", " + country);
        Serial.println("  Coordinates: " + String(latitude, 6) + ", " + String(longitude, 6));
        
        http.end();
        return true;
      } else {
        Serial.println("Geolocation failed: " + String(doc["message"].as<String>()));
      }
    } else {
      Serial.println("JSON parsing failed: " + String(error.c_str()));
    }
  } else {
    Serial.println("HTTP request failed with code: " + String(httpCode));
  }
  
  http.end();
  return false;
}

// Get weather data from OpenWeatherMap
bool getWeatherData(WeatherData& weather) {
  Serial.println("Fetching weather data...");
  
  // First get geolocation
  if (!getGeolocation(weather.latitude, weather.longitude, weather.location, weather.country)) {
    Serial.println("Failed to get geolocation");
    return false;
  }
  
  // Build OpenWeatherMap API URL
  String url = String(OPENWEATHER_BASE_URL) + 
               "?lat=" + String(weather.latitude, 6) + 
               "&lon=" + String(weather.longitude, 6) + 
               "&units=metric&appid=" + String(OPENWEATHER_API_KEY);
  
  WiFiClientSecure client;
  client.setInsecure(); // Skip certificate verification for simplicity
  
  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(2048);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Parse weather data
      weather.weatherDescription = doc["weather"][0]["description"].as<String>();
      weather.weatherIcon = doc["weather"][0]["icon"].as<String>();
      weather.temperature = doc["main"]["temp"];
      weather.feelsLike = doc["main"]["feels_like"];
      weather.humidity = doc["main"]["humidity"];
      weather.windSpeed = doc["wind"]["speed"];
      weather.windDirection = doc["wind"]["deg"];
      
      // Get current date/time for forecast
      time_t now = time(nullptr);
      struct tm* timeinfo = localtime(&now);
      char dateStr[20];
      strftime(dateStr, sizeof(dateStr), "%Y-%m-%d %H:%M", timeinfo);
      weather.forecastDate = String(dateStr);
      weather.lastUpdated = String(dateStr);
      
      Serial.println("Weather data fetched successfully:");
      Serial.println("  Location: " + weather.location + ", " + weather.country);
      Serial.println("  Weather: " + weather.weatherDescription);
      Serial.println("  Temp: " + String(weather.temperature) + "°C");
      Serial.println("  Humidity: " + String(weather.humidity) + "%");
      
      http.end();
      return true;
    } else {
      Serial.println("JSON parsing failed: " + String(error.c_str()));
    }
  } else {
    Serial.println("HTTP request failed with code: " + String(httpCode));
  }
  
  http.end();
  return false;
}

// Get weather icon code for display
String getWeatherIconCode(const String& weatherMain) {
  String weatherLower = weatherMain;
  weatherLower.toLowerCase();
  
  if (weatherLower.indexOf("clear") != -1) return "01";
  if (weatherLower.indexOf("cloud") != -1) return "02";
  if (weatherLower.indexOf("rain") != -1) return "03";
  if (weatherLower.indexOf("thunder") != -1) return "04";
  if (weatherLower.indexOf("snow") != -1) return "05";
  if (weatherLower.indexOf("mist") != -1 || weatherLower.indexOf("fog") != -1) return "06";
  
  return "01"; // Default to clear
}

// Format temperature with degree symbol
String formatTemperature(float temp) {
  return String(temp, 1) + "°C";
}

// Format wind direction
String formatWindDirection(int degrees) {
  if (degrees >= 337.5 || degrees < 22.5) return "N";
  if (degrees >= 22.5 && degrees < 67.5) return "NE";
  if (degrees >= 67.5 && degrees < 112.5) return "E";
  if (degrees >= 112.5 && degrees < 157.5) return "SE";
  if (degrees >= 157.5 && degrees < 202.5) return "S";
  if (degrees >= 202.5 && degrees < 247.5) return "SW";
  if (degrees >= 247.5 && degrees < 292.5) return "W";
  if (degrees >= 292.5 && degrees < 337.5) return "NW";
  
  return "";
}

// Display weather information on e-paper
void displayWeather(const WeatherData& weather) {
  Serial.println("Displaying weather information...");
  
  display.setRotation(1); // Set rotation to match display orientation
  display.setPartialWindow(0, 0, display.width(), display.height());
  display.firstPage();
  do {
    // Fill background with white
    display.fillScreen(GxEPD_WHITE);
    
    // Display header
    display.setCursor(10, 20);
    display.setFont(&FreeMonoBold12pt7b);
    display.print("Weather Forecast");
    
    // Display location and date
    display.setCursor(10, 45);
    display.setFont(nullptr); // Use default font
    display.setTextSize(1);
    display.print(weather.location + ", " + weather.country);
    
    display.setCursor(10, 60);
    display.print(weather.forecastDate);
    
    // Display weather icon based on weather description
    String weatherLower = weather.weatherDescription;
    weatherLower.toLowerCase();
    
    if (weatherLower.indexOf("clear") != -1) {
      drawSunIcon(10, 75);
    } else if (weatherLower.indexOf("cloud") != -1) {
      drawCloudIcon(10, 75);
    } else if (weatherLower.indexOf("rain") != -1) {
      drawRainIcon(10, 75);
    } else if (weatherLower.indexOf("thunder") != -1) {
      drawThunderIcon(10, 75);
    } else if (weatherLower.indexOf("snow") != -1) {
      drawSnowIcon(10, 75);
    } else if (weatherLower.indexOf("mist") != -1 || weatherLower.indexOf("fog") != -1) {
      drawMistIcon(10, 75);
    } else {
      // Default to cloud icon for unknown weather
      drawCloudIcon(10, 75);
    }
    
    // Display weather description
    display.setCursor(40, 80);
    display.print(weather.weatherDescription);
    
    // Display temperature
    display.setCursor(10, 100);
    display.print("Temp: " + formatTemperature(weather.temperature));
    
    // Display feels like temperature
    display.setCursor(10, 120);
    display.print("Feels like: " + formatTemperature(weather.feelsLike));
    
    // Display humidity
    display.setCursor(10, 140);
    display.print("Humidity: " + String(weather.humidity) + "%");
    
    // Display wind information
    display.setCursor(10, 160);
    display.print("Wind: " + String(weather.windSpeed) + " m/s " + formatWindDirection(weather.windDirection));
    
    // Display coordinates
    display.setCursor(10, 180);
    display.print("Lat: " + String(weather.latitude, 4) + " Lon: " + String(weather.longitude, 4));
    
  } while (display.nextPage());
  
  Serial.println("Weather display completed");
}
