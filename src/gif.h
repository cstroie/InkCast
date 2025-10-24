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

#ifndef _GIF_H_
#define _GIF_H_

#include <Arduino.h>
#include <TJpg_Decoder.h>
#include "display.h"

// Function declarations for GIF handling
bool displayGIFFile(const char* filename);

// Display GIF file using TJpg_Decoder
bool displayGIFFile(const char* filename) {
  // Initialize TJpg_Decoder
  TJpgDec.setSwapBytes(true);
  
  // Callback function for drawing pixels
  auto jpegDrawCallback = [](int16_t x, int16_t y, uint16_t w, uint16_t h, uint16_t* bitmap) {
    // Convert 16-bit color to display color
    uint16_t color = bitmap[0];
    uint8_t displayColor;
    
    // Simple color conversion (you might want to improve this)
    if ((color & 0xF800) > 0x8000 || (color & 0x07E0) > 0x0400 || (color & 0x001F) > 0x0010) {
      displayColor = GxEPD_BLACK;
    } else {
      displayColor = GxEPD_WHITE;
    }
    
    // Draw pixel by pixel (simplified approach)
    for (uint16_t i = 0; i < w; i++) {
      for (uint16_t j = 0; j < h; j++) {
        display.drawPixel(x + i, y + j, displayColor);
      }
    }
    return true;
  };
  
  TJpgDec.setCallback(jpegDrawCallback);
  
  // Display the GIF
  display.setRotation(1);
  display.setFullWindow();
  display.firstPage();
  do
  {
    display.fillScreen(GxEPD_WHITE);
    TJpgDec.drawFsJpg(0, 0, filename, SPIFFS);
  }
  while (display.nextPage());
  
  return true;
}

#endif // _GIF_H_
