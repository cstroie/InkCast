/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Weather Icons Font Implementation
 */

#include "weathericons.h"

// Use FreeMonoBold12pt7b as a placeholder font
const GFXfont *weathericons_font = &FreeMonoBold12pt7b;

// Icon mapping function
uint16_t getIconCodeForWeather(const String& iconName) {
    if (iconName == "wi-day-sunny") return WI_DAY_SUNNY;
    if (iconName == "wi-day-cloudy") return WI_DAY_CLOUDY;
    if (iconName == "wi-cloudy") return WI_CLOUDY;
    if (iconName == "wi-day-haze") return WI_DAY_HAZE;
    if (iconName == "wi-day-fog") return WI_DAY_FOG;
    if (iconName == "wi-fog") return WI_FOG;
    if (iconName == "wi-day-sprinkle") return WI_DAY_SPRINKLE;
    if (iconName == "wi-day-rain") return WI_DAY_RAIN;
    if (iconName == "wi-day-sleet") return WI_DAY_SLEET;
    if (iconName == "wi-day-snow") return WI_DAY_SNOW;
    return WI_NA; // Default icon
}
