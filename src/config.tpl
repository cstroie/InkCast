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

// Configuration template file
// Rename this file to config.h and update with your settings

// WiFi credentials
#define WIFI_SSID "your_wifi_ssid"
#define WIFI_PASSWORD "your_wifi_password"

// Deep sleep duration in microseconds
//  0 = forever, wake up by reset
// -1 = disable deep sleep
// Example: 300e6 = 300 seconds = 5 minutes
#define DEEP_SLEEP_DURATION -1

// Built-in button pin (set to -1 to disable button functionality)
#define BUTTON_PIN -1

// Server URL for random pictures
#define SERVER_URL "https://eridu.eu.org/test.pbm"

// Uncomment to enable debug output
// #define DEBUG_OUTPUT

