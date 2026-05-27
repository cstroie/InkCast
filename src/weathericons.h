/*
 * SPDX-License-Identifier: GPL-3.0
 *
 * Copyright (C) 2025 Costin Stroie <costinstroie@eridu.eu.org>
 *
 * Weather Icons - WMO code to text label mapping
 * https://open-meteo.com/en/docs#weathervariables
 */

#ifndef WEATHERICONS_H
#define WEATHERICONS_H

#include <Arduino.h>

// Returns a short text label for a WMO weather code, or nullptr if unknown.
const char* getWeatherLabel(int wmoCode);

#endif // WEATHERICONS_H
