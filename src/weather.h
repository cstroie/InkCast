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

#ifndef _WEATHER_H_
#define _WEATHER_H_

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClientSecure.h>

// Weather data structure
struct WeatherData {
  String location;
  String country;
  float latitude;
  float longitude;
  String weatherDescription;
  String weatherIcon;
  float temperature;
  float feelsLike;
  float humidity;
  float windSpeed;
  int windDirection;
  String forecastDate;
  String lastUpdated;
};

// Weather API configuration
#define OPENWEATHER_API_KEY "your_openweather_api_key"  // Replace with your API key
#define OPENWEATHER_BASE_URL "https://api.openweathermap.org/data/2.5/weather"
#define IP_API_URL "http://ip-api.com/json/"

// Function declarations
bool getGeolocation(float& latitude, float& longitude, String& location, String& country);
bool getWeatherData(WeatherData& weather);
String getWeatherIconCode(const String& weatherMain);
void displayWeather(const WeatherData& weather);
String formatTemperature(float temp);
String formatWindDirection(int degrees);

#endif // _WEATHER_H_
