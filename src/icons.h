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

#ifndef _ICONS_H_
#define _ICONS_H_

#include "display.h"

#include <GxEPD2_BW.h>

// Simple weather icon drawing functions
template<typename DisplayType>
void drawSunIcon(DisplayType& display, int x, int y);
template<typename DisplayType>
void drawCloudIcon(DisplayType& display, int x, int y);
template<typename DisplayType>
void drawRainIcon(DisplayType& display, int x, int y);
template<typename DisplayType>
void drawSnowIcon(DisplayType& display, int x, int y);
template<typename DisplayType>
void drawThunderIcon(DisplayType& display, int x, int y);
template<typename DisplayType>
void drawMistIcon(DisplayType& display, int x, int y);

#endif // _ICONS_H_
