/*
 * SPDX-License-Identifier: GPL-3.0
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 */

#include "config_manager.h"

const char* ConfigManager::NS = "epaper";

// Build NVS key for network index: "ssid0".."ssid4" / "pass0".."pass4"
static void wifiKey(char* buf, const char* prefix, int idx) {
  buf[0] = prefix[0]; buf[1] = prefix[1]; buf[2] = prefix[2];
  buf[3] = prefix[3]; buf[4] = '0' + idx; buf[5] = '\0';
}

void ConfigManager::load(Config& cfg) {
  Preferences p;
  p.begin(NS, /*readOnly=*/true);

  // One-time migration from single-SSID layout (keys "ssid" / "pass")
  String oldSsid = p.getString("ssid", "");
  int storedN = p.getInt("wifin", -1);
  p.end();

  if (storedN < 0) {
    // First run with new layout — migrate old credentials if present
    cfg.wifiCount = 0;
    if (oldSsid.length() > 0) {
      strlcpy(cfg.wifi[0].ssid, oldSsid.c_str(), sizeof(cfg.wifi[0].ssid));
      String oldPass = "";
      Preferences p2;
      p2.begin(NS, /*readOnly=*/true);
      oldPass = p2.getString("pass", "");
      p2.end();
      strlcpy(cfg.wifi[0].pass, oldPass.c_str(), sizeof(cfg.wifi[0].pass));
      cfg.wifiCount = 1;
    }
    // Persist new layout and erase old keys
    Preferences pw;
    pw.begin(NS, /*readOnly=*/false);
    pw.putInt("wifin", cfg.wifiCount);
    if (cfg.wifiCount > 0) {
      char key[6];
      wifiKey(key, "ssid", 0); pw.putString(key, cfg.wifi[0].ssid);
      wifiKey(key, "pass", 0); pw.putString(key, cfg.wifi[0].pass);
    }
    pw.remove("ssid");
    pw.remove("pass");
    pw.end();
  } else {
    cfg.wifiCount = constrain(storedN, 0, WIFI_MAX);
    Preferences p2;
    p2.begin(NS, /*readOnly=*/true);
    char key[6];
    for (int i = 0; i < cfg.wifiCount; i++) {
      wifiKey(key, "ssid", i); strlcpy(cfg.wifi[i].ssid, p2.getString(key, "").c_str(), sizeof(cfg.wifi[i].ssid));
      wifiKey(key, "pass", i); strlcpy(cfg.wifi[i].pass, p2.getString(key, "").c_str(), sizeof(cfg.wifi[i].pass));
    }
    p2.end();
  }

  Preferences p3;
  p3.begin(NS, /*readOnly=*/true);
  strlcpy(cfg.city, p3.getString("city", "").c_str(), sizeof(cfg.city));
  cfg.tempUnits      = p3.getInt("units",    1);
  cfg.forecastDays   = p3.getInt("fdays",    1);
  cfg.updateInterval = p3.getInt("interval", 30);
  cfg.deepSleepMins  = p3.getInt("sleep",   -1);
  cfg.buttonPin      = p3.getInt("btn",      9);
  cfg.ledPin         = p3.getInt("led",      8);
  p3.end();
}

void ConfigManager::save(const Config& cfg) {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.putInt("wifin", cfg.wifiCount);
  char key[6];
  for (int i = 0; i < cfg.wifiCount; i++) {
    wifiKey(key, "ssid", i); p.putString(key, cfg.wifi[i].ssid);
    wifiKey(key, "pass", i); p.putString(key, cfg.wifi[i].pass);
  }
  // Erase any slots beyond current count (handles deletions)
  for (int i = cfg.wifiCount; i < WIFI_MAX; i++) {
    wifiKey(key, "ssid", i); if (p.isKey(key)) p.remove(key);
    wifiKey(key, "pass", i); if (p.isKey(key)) p.remove(key);
  }
  p.putString("city",     cfg.city);
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
  int n = p.getInt("wifin", -1);
  bool ok = false;
  if (n > 0) {
    ok = true;
  } else if (n < 0) {
    // Legacy layout
    ok = p.getString("ssid", "").length() > 0;
  }
  p.end();
  return ok;
}

void ConfigManager::reset() {
  Preferences p;
  p.begin(NS, /*readOnly=*/false);
  p.clear();
  p.end();
}
