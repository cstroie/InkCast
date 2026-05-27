/*
 * SPDX-License-Identifier: GPL-3.0
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 */

#include "config_manager.h"

const char* ConfigManager::NS = "epaper";

void ConfigManager::load(Config& cfg) {
  Preferences p;
  p.begin(NS, /*readOnly=*/true);
  strlcpy(cfg.wifiSsid,     p.getString("ssid",     "").c_str(), sizeof(cfg.wifiSsid));
  strlcpy(cfg.wifiPassword, p.getString("pass",     "").c_str(), sizeof(cfg.wifiPassword));
  cfg.tempUnits      = p.getInt("units",    1);
  cfg.forecastDays   = p.getInt("fdays",    1);
  cfg.updateInterval = p.getInt("interval", 30);
  cfg.deepSleepMins  = p.getInt("sleep",   -1);
  cfg.buttonPin      = p.getInt("btn",     -1);
  cfg.ledPin         = p.getInt("led",     -1);
  p.end();
}

void ConfigManager::save(const Config& cfg) {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.putString("ssid",     cfg.wifiSsid);
  p.putString("pass",     cfg.wifiPassword);
  p.putInt("units",       cfg.tempUnits);
  p.putInt("fdays",       cfg.forecastDays);
  p.putInt("interval",    cfg.updateInterval);
  p.putInt("sleep",       cfg.deepSleepMins);
  p.putInt("btn",         cfg.buttonPin);
  p.putInt("led",         cfg.ledPin);
  p.end();
}

bool ConfigManager::isConfigured() {
  Preferences p;
  p.begin(NS, /*readOnly=*/true);
  bool ok = p.getString("ssid", "").length() > 0;
  p.end();
  return ok;
}

void ConfigManager::reset() {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.clear();
  p.end();
}
