/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2025 Costin Stroie <costinstroie@eridu.eu.org>
 */

#include "weathericons.h"

struct WmoIcon {
  int      code;
  uint16_t glyph;
};

static const WmoIcon wmoTable[] = {
  {  0, WI_DAY_SUNNY    },  // Clear sky
  {  1, WI_DAY_CLOUDY   },  // Mainly clear
  {  2, WI_DAY_CLOUDY   },  // Partly cloudy
  {  3, WI_CLOUDY       },  // Overcast
  { 45, WI_DAY_FOG      },  // Fog
  { 48, WI_FOG          },  // Depositing rime fog
  { 51, WI_DAY_SPRINKLE },  // Drizzle: light
  { 53, WI_DAY_SPRINKLE },  // Drizzle: moderate
  { 55, WI_DAY_RAIN     },  // Drizzle: dense
  { 56, WI_DAY_SLEET    },  // Freezing drizzle: light
  { 57, WI_DAY_SLEET    },  // Freezing drizzle: dense
  { 61, WI_DAY_RAIN     },  // Rain: slight
  { 63, WI_DAY_RAIN     },  // Rain: moderate
  { 65, WI_DAY_RAIN     },  // Rain: heavy
  { 66, WI_DAY_SLEET    },  // Freezing rain: light
  { 67, WI_DAY_SLEET    },  // Freezing rain: heavy
  { 71, WI_DAY_SNOW     },  // Snow: slight
  { 73, WI_DAY_SNOW     },  // Snow: moderate
  { 75, WI_SNOW         },  // Snow: heavy
  { 77, WI_HAIL         },  // Snow grains
  { 80, WI_SHOWERS      },  // Rain showers: slight
  { 81, WI_SHOWERS      },  // Rain showers: moderate
  { 82, WI_SHOWERS      },  // Rain showers: violent
  { 85, WI_DAY_SNOW     },  // Snow showers: slight
  { 86, WI_SNOW         },  // Snow showers: heavy
  { 95, WI_THUNDERSTORM },  // Thunderstorm
  { 96, WI_THUNDERSTORM },  // Thunderstorm with slight hail
  { 99, WI_THUNDERSTORM },  // Thunderstorm with heavy hail
};

uint16_t getIconCode(int wmoCode) {
  for (size_t i = 0; i < sizeof(wmoTable) / sizeof(wmoTable[0]); i++) {
    if (wmoTable[i].code == wmoCode)
      return wmoTable[i].glyph;
  }
  return WI_NA;
}
