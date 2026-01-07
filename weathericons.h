/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Weather Icons Font - Subset for ESP32
 * Based on weather-icons by Erik Flowers
 * https://github.com/erikflowers/weather-icons
 */

#ifndef WEATHERICONS_H
#define WEATHERICONS_H

#include <Adafruit_GFX.h>

// Weather icons font data - subset containing only the icons we need
// This is a simplified version - in practice you would generate this
// from the actual TTF file using font conversion tools

// Font structure
extern const uint8_t weathericons_font[];

// Icon Unicode mappings
#define WI_DAY_SUNNY 0xF00D
#define WI_DAY_CLOUDY 0xF002
#define WI_CLOUDY 0xF013
#define WI_DAY_HAZE 0xF0B6
#define WI_DAY_FOG 0xF001
#define WI_FOG 0xF014
#define WI_DAY_SPRINKLE 0xF003
#define WI_DAY_RAIN 0xF008
#define WI_DAY_SLEET 0xF0B2
#define WI_DAY_SNOW 0xF00A
#define WI_NA 0xF07B // Not available

// Function to get icon code from weather icon name
uint16_t getIconCodeForWeather(const String& iconName);

#endif // WEATHERICONS_H
