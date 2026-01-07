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

// Get weather data from Open-Meteo
bool getWeatherData(WeatherData& weather) {
  Serial.println("Fetching weather data...");
  
  // First get geolocation
  if (!getGeolocation(weather.latitude, weather.longitude, weather.location, weather.country)) {
    Serial.println("Failed to get geolocation");
    return false;
  }
  
  // Build Open-Meteo API URL
  String url = String(METEO_API_URL) + 
               "?latitude=" + String(weather.latitude, 6) + 
               "&longitude=" + String(weather.longitude, 6) + 
               "&daily=weather_code,temperature_2m_max,temperature_2m_min,precipitation_hours,precipitation_probability_max,wind_speed_10m_max,wind_gusts_10m_max,wind_direction_10m_dominant" +
               "&timezone=auto&forecast_days=1";
  
  WiFiClient client;
  HTTPClient http;
  http.begin(client, url);
  int httpCode = http.GET();
  
  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    DynamicJsonDocument doc(4096);
    DeserializationError error = deserializeJson(doc, payload);
    
    if (!error) {
      // Parse weather data
      JsonArray daily = doc["daily"];
      if (daily.size() > 0) {
        int weatherCode = daily[0]["weather_code"];
        weather.weatherDescription = getWeatherDescription(weatherCode);
        weather.temperature = daily[0]["temperature_2m_max"];
        weather.feelsLike = daily[0]["temperature_2m_min"];
        weather.humidity = 0; // Open-Meteo doesn't provide humidity in this endpoint
        weather.windSpeed = daily[0]["wind_speed_10m_max"];
        weather.windDirection = daily[0]["wind_direction_10m_dominant"];
        
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
        Serial.println("  Wind: " + String(weather.windSpeed) + " m/s");
        
        http.end();
        return true;
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

// Get weather description from WMO weather codes
String getWeatherDescription(int weatherCode) {
  switch (weatherCode) {
    case 0: return "Clear sky";
    case 1: return "Mostly cloudy";
    case 2: return "Partly cloudy";
    case 3: return "Overcast";
    case 45: return "Fog";
    case 48: return "Freezing fog";
    case 51: return "Light drizzle";
    case 53: return "Moderate drizzle";
    case 55: return "Heavy drizzle";
    case 56: return "Light freezing drizzle";
    case 57: return "Heavy freezing drizzle";
    case 61: return "Light rain";
    case 63: return "Moderate rain";
    case 65: return "Heavy rain";
    case 66: return "Light freezing rain";
    case 67: return "Heavy freezing rain";
    case 71: return "Light snow";
    case 73: return "Moderate snow";
    case 75: return "Heavy snow";
    case 77: return "Snow grains";
    case 80: return "Light rain showers";
    case 81: return "Moderate rain showers";
    case 82: return "Violent rain showers";
    case 85: return "Light snow showers";
    case 86: return "Heavy snow showers";
    case 95: return "Thunderstorm";
    case 96: return "Thunderstorm with light hail";
    case 99: return "Thunderstorm with heavy hail";
    default: return "Unknown weather";
  }
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
    } else if (weatherLower.indexOf("cloud") != -1 || weatherLower.indexOf("overcast") != -1) {
      drawCloudIcon(10, 75);
    } else if (weatherLower.indexOf("rain") != -1 || weatherLower.indexOf("drizzle") != -1) {
      drawRainIcon(10, 75);
    } else if (weatherLower.indexOf("thunder") != -1) {
      drawThunderIcon(10, 75);
    } else if (weatherLower.indexOf("snow") != -1) {
      drawSnowIcon(10, 75);
    } else if (weatherLower.indexOf("fog") != -1 || weatherLower.indexOf("mist") != -1) {
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
    
    // Display humidity (not available from Open-Meteo in this endpoint)
    display.setCursor(10, 140);
    display.print("Humidity: N/A");
    
    // Display wind information
    display.setCursor(10, 160);
    display.print("Wind: " + String(weather.windSpeed) + " m/s " + formatWindDirection(weather.windDirection));
    
    // Display coordinates
    display.setCursor(10, 180);
    display.print("Lat: " + String(weather.latitude, 4) + " Lon: " + String(weather.longitude, 4));
    
  } while (display.nextPage());
  
  Serial.println("Weather display completed");
}
