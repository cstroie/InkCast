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

#include "icons.h"
#include "display.h"

// Draw a simple sun icon
void drawSunIcon(int x, int y) {
  // Draw sun circle
  display.fillCircle(x + 10, y + 10, 8, GxEPD_BLACK);
  
  // Draw sun rays
  display.drawLine(x + 10, y, x + 10, y + 4, GxEPD_BLACK);      // Top
  display.drawLine(x + 10, y + 16, x + 10, y + 20, GxEPD_BLACK); // Bottom
  display.drawLine(x, y + 10, x + 4, y + 10, GxEPD_BLACK);      // Left
  display.drawLine(x + 16, y + 10, x + 20, y + 10, GxEPD_BLACK); // Right
  display.drawLine(x + 3, y + 3, x + 7, y + 7, GxEPD_BLACK);    // Top-left
  display.drawLine(x + 13, y + 13, x + 17, y + 17, GxEPD_BLACK); // Bottom-right
  display.drawLine(x + 13, y + 3, x + 17, y + 7, GxEPD_BLACK);  // Top-right
  display.drawLine(x + 3, y + 13, x + 7, y + 17, GxEPD_BLACK);  // Bottom-left
}

// Draw a simple cloud icon
void drawCloudIcon(int x, int y) {
  // Draw cloud body
  display.fillCircle(x + 8, y + 12, 6, GxEPD_BLACK);
  display.fillCircle(x + 16, y + 12, 6, GxEPD_BLACK);
  display.fillCircle(x + 12, y + 8, 6, GxEPD_BLACK);
  display.fillRect(x + 6, y + 12, 12, 8, GxEPD_BLACK);
}

// Draw a simple rain icon
void drawRainIcon(int x, int y) {
  drawCloudIcon(x, y);
  // Draw rain drops
  display.drawLine(x + 8, y + 20, x + 8, y + 26, GxEPD_BLACK);
  display.drawLine(x + 16, y + 20, x + 16, y + 26, GxEPD_BLACK);
  display.drawLine(x + 12, y + 22, x + 12, y + 28, GxEPD_BLACK);
}

// Draw a simple snow icon
void drawSnowIcon(int x, int y) {
  drawCloudIcon(x, y);
  // Draw snowflakes
  display.drawLine(x + 8, y + 20, x + 12, y + 24, GxEPD_BLACK);
  display.drawLine(x + 12, y + 20, x + 8, y + 24, GxEPD_BLACK);
  display.drawLine(x + 16, y + 20, x + 20, y + 24, GxEPD_BLACK);
  display.drawLine(x + 20, y + 20, x + 16, y + 24, GxEPD_BLACK);
}

// Draw a simple thunder icon
void drawThunderIcon(int x, int y) {
  drawCloudIcon(x, y);
  // Draw lightning bolt
  display.drawLine(x + 12, y + 20, x + 12, y + 28, GxEPD_BLACK);
  display.drawLine(x + 12, y + 28, x + 8, y + 24, GxEPD_BLACK);
  display.drawLine(x + 8, y + 24, x + 16, y + 24, GxEPD_BLACK);
}

// Draw a simple mist/fog icon
void drawMistIcon(int x, int y) {
  drawCloudIcon(x, y);
  // Draw mist lines
  display.drawLine(x + 4, y + 22, x + 8, y + 22, GxEPD_BLACK);
  display.drawLine(x + 10, y + 24, x + 16, y + 24, GxEPD_BLACK);
  display.drawLine(x + 18, y + 22, x + 22, y + 22, GxEPD_BLACK);
}
