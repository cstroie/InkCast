/*
 * SPDX-License-Identifier: GPL-3.0
 * Copyright (C) 2026 Costin Stroie <costinstroie@eridu.eu.org>
 */

#include "portal.h"
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

// ---------------------------------------------------------------------------
// HTML page (stored in flash)
// ---------------------------------------------------------------------------

static const char HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>ePaperFrame Setup</title>
<style>
*{box-sizing:border-box;margin:0;padding:0}
body{font-family:system-ui,sans-serif;background:#f0f2f5;color:#1a1a2e;padding:20px}
h1{font-size:1.4rem;margin:16px 0 20px}
section{background:#fff;border-radius:10px;padding:16px;margin-bottom:14px;
        box-shadow:0 1px 4px rgba(0,0,0,.08)}
h2{font-size:.8rem;text-transform:uppercase;letter-spacing:.08em;color:#999;margin-bottom:12px}
label{display:block;margin-bottom:10px}
span.lbl{display:block;font-size:.88rem;color:#444;margin-bottom:4px;font-weight:500}
small{display:block;font-size:.75rem;color:#aaa;margin-top:3px}
input,select{width:100%;padding:9px 11px;border:1px solid #ddd;border-radius:6px;
             font-size:.95rem;background:#fafafa;color:#1a1a2e}
input:focus,select:focus{outline:none;border-color:#4a90d9;background:#fff}
button{width:100%;padding:13px;background:#4a90d9;color:#fff;border:none;
       border-radius:8px;font-size:1rem;font-weight:600;cursor:pointer;margin-top:6px}
button:active{background:#357abd}
</style>
</head>
<body>
<h1>ePaperFrame Setup</h1>
<form method="POST" action="/save">

<section>
<h2>WiFi</h2>
<label>
  <span class="lbl">Network name (SSID)</span>
  <input name="ssid" value="%SSID%" required autocomplete="off">
</label>
<label>
  <span class="lbl">Password</span>
  <input name="pass" type="password" value="%PASS%" autocomplete="off">
</label>
</section>

<section>
<h2>Weather</h2>
<label>
  <span class="lbl">Temperature units</span>
  <select name="units">
    <option value="1"%SEL_C%>Celsius (&deg;C)</option>
    <option value="0"%SEL_F%>Fahrenheit (&deg;F)</option>
  </select>
</label>
<label>
  <span class="lbl">Forecast days (1&ndash;7)</span>
  <input name="fdays" type="number" min="1" max="7" value="%FDAYS%">
  <small>How many days of forecast to fetch (display shows today only for now)</small>
</label>
<label>
  <span class="lbl">Update interval (minutes)</span>
  <input name="interval" type="number" min="1" max="1440" value="%INTERVAL%">
</label>
</section>

<section>
<h2>Power</h2>
<label>
  <span class="lbl">Deep sleep duration (minutes)</span>
  <input name="sleep" type="number" min="-1" value="%SLEEP%">
  <small>Device refreshes then sleeps for this long. Use -1 to stay awake.</small>
</label>
</section>

<section>
<h2>Hardware pins (&minus;1 = not connected)</h2>
<label>
  <span class="lbl">Button GPIO</span>
  <input name="btn" type="number" min="-1" max="48" value="%BTN%">
  <small>Hold at boot to re-enter setup. Press anytime to force refresh.</small>
</label>
<label>
  <span class="lbl">LED GPIO</span>
  <input name="led" type="number" min="-1" max="48" value="%LED%">
</label>
</section>

<button type="submit">Save &amp; Restart</button>
</form>
</body>
</html>)html";

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static String buildPage(const Config& cfg) {
  String html;
  html.reserve(3000);
  html = FPSTR(HTML);
  html.replace("%SSID%",  String(cfg.wifiSsid));
  html.replace("%PASS%",  String(cfg.wifiPassword));
  html.replace("%SEL_C%", cfg.tempUnits == 1 ? " selected" : "");
  html.replace("%SEL_F%", cfg.tempUnits == 0 ? " selected" : "");
  html.replace("%FDAYS%",    String(cfg.forecastDays));
  html.replace("%INTERVAL%", String(cfg.updateInterval));
  html.replace("%SLEEP%",    String(cfg.deepSleepMins));
  html.replace("%BTN%",      String(cfg.buttonPin));
  html.replace("%LED%",      String(cfg.ledPin));
  return html;
}

// ---------------------------------------------------------------------------
// Portal
// ---------------------------------------------------------------------------

void runConfigPortal(Config& cfg, const char* apName) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("Config portal: SSID=%s  IP=%s\n", apName, ip.toString().c_str());

  // Captive-portal DNS: answer all queries with our IP
  DNSServer dns;
  dns.start(53, "*", ip);

  WebServer server(80);

  server.on("/", HTTP_GET, [&]() {
    server.send(200, "text/html", buildPage(cfg));
  });

  server.on("/save", HTTP_POST, [&]() {
    if (server.hasArg("ssid"))
      strlcpy(cfg.wifiSsid,     server.arg("ssid").c_str(), sizeof(cfg.wifiSsid));
    if (server.hasArg("pass"))
      strlcpy(cfg.wifiPassword, server.arg("pass").c_str(), sizeof(cfg.wifiPassword));
    cfg.tempUnits      = server.arg("units").toInt();
    cfg.forecastDays   = constrain(server.arg("fdays").toInt(), 1, 7);
    cfg.updateInterval = max(1, (int)server.arg("interval").toInt());
    cfg.deepSleepMins  = server.arg("sleep").toInt();
    cfg.buttonPin      = server.arg("btn").toInt();
    cfg.ledPin         = server.arg("led").toInt();

    ConfigManager::save(cfg);
    server.send(200, "text/html",
      "<!DOCTYPE html><html><body style='font-family:system-ui;padding:40px;text-align:center'>"
      "<h2>Saved!</h2><p>Rebooting&hellip;</p></body></html>");
    delay(1500);
    ESP.restart();
  });

  // Captive portal redirect for all unknown paths
  server.onNotFound([&]() {
    server.sendHeader("Location", String("http://") + ip.toString() + "/");
    server.send(302, "text/plain", "");
  });

  server.begin();

  while (true) {
    dns.processNextRequest();
    server.handleClient();
    delay(2);
  }
}
