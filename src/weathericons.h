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
#include "WeatherIcons32pt7b.h"
#include "WeatherIcons48pt7b.h"

#define WI_FONT (&weathericons48pt8b)

// Codepoints used
#define WI_DAY_SUNNY     0xF00D
#define WI_DAY_CLOUDY    0xF002
#define WI_CLOUDY        0xF013
#define WI_DAY_FOG       0xF001
#define WI_FOG           0xF014
#define WI_DAY_SPRINKLE  0xF003
#define WI_DAY_RAIN      0xF008
#define WI_SHOWERS       0xF009
#define WI_DAY_SLEET     0xF0B2
#define WI_DAY_SNOW      0xF00A
#define WI_SNOW          0xF01B
#define WI_HAIL          0xF015
#define WI_THUNDERSTORM  0xF01E
#define WI_NA            0xF07B

// Returns the Weather Icons codepoint for a WMO weather code.
// Returns WI_NA if the code is unknown.
uint16_t getIconCode(int wmoCode);

#endif // WEATHERICONS_H
