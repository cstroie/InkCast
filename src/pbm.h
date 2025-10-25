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

#ifndef _PBM_H_
#define _PBM_H_

#include <Arduino.h>
#include <FS.h>

// Function declarations for PBM handling
bool parsePBMHeader(Stream* stream, int& width, int& height);
bool readPBMData(Stream* stream, uint8_t* buffer, int width, int height);

// Parse PBM header to get image dimensions
bool parsePBMHeader(Stream* stream, int& width, int& height) {
  // Read PBM header (P4 format for binary)
  char header[2];
  if (stream->readBytes(header, 2) != 2) {
    return false;
  }
  
  // Check if it's a valid PBM binary format
  if (header[0] != 'P' || header[1] != '4') {
    return false;
  }
  
  // Skip comments and whitespace
  while (stream->available()) {
    char c = stream->peek();
    if (c == '#') {
      // Skip comment line
      stream->read(); // consume the '#'
      while (stream->available() && stream->read() != '\n');
    } else if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      stream->read(); // consume whitespace
    } else {
      break;
    }
  }
  
  // Parse width
  width = 0;
  while (stream->available()) {
    char c = stream->peek();
    if (c >= '0' && c <= '9') {
      width = width * 10 + (c - '0');
      stream->read(); // consume the digit
    } else {
      break;
    }
  }
  
  // Skip whitespace between width and height
  while (stream->available()) {
    char c = stream->peek();
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      stream->read(); // consume whitespace
    } else {
      break;
    }
  }
  
  // Parse height
  height = 0;
  while (stream->available()) {
    char c = stream->peek();
    if (c >= '0' && c <= '9') {
      height = height * 10 + (c - '0');
      stream->read(); // consume the digit
    } else {
      break;
    }
  }
  
  // Skip any remaining whitespace
  while (stream->available()) {
    char c = stream->peek();
    if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
      stream->read(); // consume whitespace
    } else {
      break;
    }
  }
  
  return (width > 0 && height > 0);
}

// Read PBM image data into buffer
bool readPBMData(Stream* stream, uint8_t* buffer, int width, int height) {
  // Calculate buffer size (1 bit per pixel)
  int bufferSize = (width * height + 7) / 8;
  
  // Read image data
  int bytesRead = stream->readBytes(buffer, bufferSize);
  
  return (bytesRead == bufferSize);
}

#endif // _PBM_H_
