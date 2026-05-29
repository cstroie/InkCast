/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 *
 * WMO weather code → Weather Icons glyph mapping
 * Font codepoints from https://erikflowers.github.io/weather-icons/
 */

#ifndef WEATHERICONS_H
#define WEATHERICONS_H

#include <Arduino.h>
#include <Adafruit_GFX.h>
#include "WeatherIcons44pt7b.h"
#include "WeatherIcons10pt7b.h"

#define WI_FONT        (&weathericons44pt8b)
#define WI_SMALL_FONT  (&weathericons10pt8b)

// Codepoints used — day variants
#define WI_DAY_SUNNY     ((uint16_t)0xF00D)  // wi-day-sunny
#define WI_DAY_CLOUDY    ((uint16_t)0xF002)  // wi-day-cloudy
#define WI_CLOUDY        ((uint16_t)0xF013)  // wi-cloudy
#define WI_DAY_FOG       ((uint16_t)0xF003)  // wi-day-fog
#define WI_FOG           ((uint16_t)0xF014)  // wi-fog
#define WI_DAY_SPRINKLE  ((uint16_t)0xF00B)  // wi-day-sprinkle
#define WI_DAY_RAIN      ((uint16_t)0xF008)  // wi-day-rain
#define WI_SHOWERS       ((uint16_t)0xF009)  // wi-day-showers
#define WI_DAY_SLEET     ((uint16_t)0xF0B2)  // wi-day-sleet
#define WI_DAY_SNOW      ((uint16_t)0xF00A)  // wi-day-snow
#define WI_SNOW          ((uint16_t)0xF01B)  // wi-snow
#define WI_HAIL          ((uint16_t)0xF015)  // wi-hail
#define WI_THUNDERSTORM  ((uint16_t)0xF01E)  // wi-thunderstorm

// Night variants
#define WI_NIGHT_CLEAR        ((uint16_t)0xF02E)  // wi-night-clear
#define WI_NIGHT_CLOUDY       ((uint16_t)0xF086)  // wi-night-alt-cloudy
#define WI_NIGHT_FOG          ((uint16_t)0xF04A)  // wi-night-fog
#define WI_NIGHT_SPRINKLE     ((uint16_t)0xF02B)  // wi-night-alt-sprinkle
#define WI_NIGHT_RAIN         ((uint16_t)0xF028)  // wi-night-alt-rain
#define WI_NIGHT_SHOWERS      ((uint16_t)0xF029)  // wi-night-alt-showers
#define WI_NIGHT_SLEET        ((uint16_t)0xF0B4)  // wi-night-alt-sleet
#define WI_NIGHT_SNOW         ((uint16_t)0xF02A)  // wi-night-alt-snow
#define WI_NIGHT_THUNDERSTORM ((uint16_t)0xF02D)  // wi-night-alt-thunderstorm

// Neutral / special
#define WI_NA            ((uint16_t)0xF07B)  // wi-na
#define WI_CLOUD_REFRESH ((uint16_t)0xF03E)  // wi-cloud-refresh
#define WI_REFRESH       ((uint16_t)0xF04C)  // wi-refresh
#define WI_UMBRELLA      ((uint16_t)0xF084)  // wi-umbrella

// Returns the Weather Icons codepoint for a WMO weather code.
// isDay=true selects day icons, false selects night icons where available.
// Returns WI_NA if the code is unknown.
uint16_t getIconCode(int wmoCode, bool isDay = true);

#endif // WEATHERICONS_H
