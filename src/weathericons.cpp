/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 */

#include "weathericons.h"

struct WmoIcon {
  int      code;
  uint16_t day;
  uint16_t night;
};

static const WmoIcon wmoTable[] = {
  {  0, WI_DAY_SUNNY,         WI_NIGHT_CLEAR            },  // Clear sky
  {  1, WI_DAY_SUNNY_OVERCAST, WI_NIGHT_ALT_PARTLY_CLOUDY},  // Mainly clear
  {  2, WI_DAY_CLOUDY,        WI_NIGHT_ALT_CLOUDY       },  // Partly cloudy
  {  3, WI_CLOUDY,            WI_CLOUDY                 },  // Overcast
  { 45, WI_DAY_FOG,           WI_NIGHT_FOG              },  // Fog
  { 48, WI_FOG,               WI_FOG                    },  // Depositing rime fog
  { 51, WI_DAY_SPRINKLE,      WI_NIGHT_ALT_SPRINKLE     },  // Drizzle: light
  { 53, WI_DAY_SPRINKLE,      WI_NIGHT_ALT_SPRINKLE     },  // Drizzle: moderate
  { 55, WI_DAY_RAIN,          WI_NIGHT_ALT_RAIN         },  // Drizzle: dense
  { 56, WI_DAY_SLEET,         WI_NIGHT_ALT_SLEET        },  // Freezing drizzle: light
  { 57, WI_DAY_SLEET,         WI_NIGHT_ALT_SLEET        },  // Freezing drizzle: dense
  { 61, WI_DAY_SPRINKLE,      WI_NIGHT_ALT_SPRINKLE     },  // Rain: slight
  { 63, WI_DAY_RAIN,          WI_NIGHT_ALT_RAIN         },  // Rain: moderate
  { 65, WI_RAIN,              WI_NIGHT_ALT_RAIN         },  // Rain: heavy
  { 66, WI_DAY_SLEET,         WI_NIGHT_ALT_SLEET        },  // Freezing rain: light
  { 67, WI_DAY_SLEET,         WI_NIGHT_ALT_SLEET        },  // Freezing rain: heavy
  { 71, WI_DAY_SNOW,          WI_NIGHT_ALT_SNOW         },  // Snow: slight
  { 73, WI_DAY_SNOW,          WI_NIGHT_ALT_SNOW         },  // Snow: moderate
  { 75, WI_SNOW,              WI_SNOW                   },  // Snow: heavy
  { 77, WI_HAIL,              WI_HAIL                   },  // Snow grains
  { 80, WI_DAY_SHOWERS,       WI_NIGHT_ALT_SHOWERS      },  // Rain showers: slight
  { 81, WI_SHOWERS,           WI_NIGHT_ALT_SHOWERS      },  // Rain showers: moderate
  { 82, WI_STORM_SHOWERS,     WI_NIGHT_ALT_STORM_SHOWERS},  // Rain showers: violent
  { 85, WI_DAY_SNOW,          WI_NIGHT_ALT_SNOW         },  // Snow showers: slight
  { 86, WI_SNOW,              WI_SNOW                   },  // Snow showers: heavy
  { 95, WI_DAY_THUNDERSTORM,  WI_NIGHT_ALT_THUNDERSTORM },  // Thunderstorm
  { 96, WI_DAY_THUNDERSTORM,  WI_NIGHT_ALT_THUNDERSTORM },  // Thunderstorm with slight hail
  { 99, WI_THUNDERSTORM,      WI_NIGHT_ALT_THUNDERSTORM },  // Thunderstorm with heavy hail
};

uint16_t getIconCode(int wmoCode, bool isDay) {
  for (size_t i = 0; i < sizeof(wmoTable) / sizeof(wmoTable[0]); i++) {
    if (wmoTable[i].code == wmoCode)
      return isDay ? wmoTable[i].day : wmoTable[i].night;
  }
  return WI_NA;
}
