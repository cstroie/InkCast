/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2025 Costin Stroie <costinstroie@eridu.eu.org>
 *
 * Weather Icons - WMO code to text label mapping
 */

#include "weathericons.h"

struct WmoEntry {
  int       code;
  const char* label;
};

static const WmoEntry wmoTable[] = {
  {  0, "Clear"      },
  {  1, "Mostly Clr" },
  {  2, "Part.Cloudy" },
  {  3, "Overcast"   },
  { 45, "Fog"        },
  { 48, "Frz.Fog"    },
  { 51, "Lt.Drizzle" },
  { 53, "Drizzle"    },
  { 55, "Hvy Drzl"   },
  { 56, "Frz.Drzl"   },
  { 57, "Hvy FDrzl"  },
  { 61, "Lt.Rain"    },
  { 63, "Rain"       },
  { 65, "Hvy Rain"   },
  { 66, "Frz.Rain"   },
  { 67, "Hvy FRain"  },
  { 71, "Lt.Snow"    },
  { 73, "Snow"       },
  { 75, "Hvy Snow"   },
  { 77, "Snow Grns"  },
  { 80, "Showers"    },
  { 81, "Rain Shwrs" },
  { 82, "Hvy Shwrs"  },
  { 85, "Snow Shwrs" },
  { 86, "HvySnwShwr" },
  { 95, "Tstorm"     },
  { 96, "Tstorm+Hail"},
  { 99, "Tstorm+Hail"},
};

const char* getWeatherLabel(int wmoCode) {
  for (size_t i = 0; i < sizeof(wmoTable) / sizeof(wmoTable[0]); i++) {
    if (wmoTable[i].code == wmoCode)
      return wmoTable[i].label;
  }
  return nullptr;
}
