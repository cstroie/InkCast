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

// Codepoints used
#define WI_DAY_SUNNY     ((uint16_t)0xF00D)
#define WI_DAY_CLOUDY    ((uint16_t)0xF002)
#define WI_CLOUDY        ((uint16_t)0xF013)
#define WI_DAY_FOG       ((uint16_t)0xF001)
#define WI_FOG           ((uint16_t)0xF014)
#define WI_DAY_SPRINKLE  ((uint16_t)0xF003)
#define WI_DAY_RAIN      ((uint16_t)0xF008)
#define WI_SHOWERS       ((uint16_t)0xF009)
#define WI_DAY_SLEET     ((uint16_t)0xF0B2)
#define WI_DAY_SNOW      ((uint16_t)0xF00A)
#define WI_SNOW          ((uint16_t)0xF01B)
#define WI_HAIL          ((uint16_t)0xF015)
#define WI_THUNDERSTORM  ((uint16_t)0xF01E)
#define WI_NA            ((uint16_t)0xF07B)
#define WI_UMBRELLA      ((uint16_t)0xF084)

// Returns the Weather Icons codepoint for a WMO weather code.
// Returns WI_NA if the code is unknown.
uint16_t getIconCode(int wmoCode);

#endif // WEATHERICONS_H
