/*
 * SPDX-License-Identifier: GPL-3.0
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 */

#pragma once
#include <Arduino.h>
#include <Preferences.h>

struct Config {
  char  wifiSsid[64];
  char  wifiPassword[64];
  char  city[64];         // manual city override; empty = auto via ip-api.com
  int   tempUnits;        // 0 = Fahrenheit, 1 = Celsius
  int   forecastDays;     // 1–7
  int   updateInterval;   // minutes between refreshes
  int   deepSleepMins;    // minutes per sleep cycle, -1 = disabled
  int   buttonPin;        // GPIO for manual refresh, -1 = disabled
  int   ledPin;           // GPIO for status LED,     -1 = disabled
};

class ConfigManager {
public:
  // Load config from NVS into cfg. Missing keys get defaults.
  static void load(Config& cfg);

  // Persist cfg to NVS.
  static void save(const Config& cfg);

  // True if a WiFi SSID has been saved.
  static bool isConfigured();

  // Erase all stored keys.
  static void reset();

private:
  static const char* NS;  // NVS namespace
};
