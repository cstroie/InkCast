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
<title>InkCast Setup</title>
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
<h1>InkCast Setup</h1>
<form method="POST" action="/">

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
  <small>How many days of forecast to fetch. The display shows today&rsquo;s forecast only.</small>
</label>
<label>
  <span class="lbl">Update interval</span>
  <select name="interval">%INTERVAL_OPTIONS%</select>
  <small>How often to refresh weather data.</small>
</label>
<label>
  <span class="lbl">Deep sleep between refreshes</span>
  <input name="sleep" type="checkbox" value="1" %SLEEP_CHK% style="width:auto;margin-top:6px">
  <small>When enabled, the device sleeps between refreshes to save power.</small>
</label>
</section>

<section>
<h2>Hardware pins (&minus;1&nbsp;=&nbsp;not connected)</h2>
<label>
  <span class="lbl">Button GPIO</span>
  <input name="btn" type="number" min="-1" max="48" value="%BTN%">
  <small>Hold at boot (&gt;1 s) to re-enter setup. Short press during normal operation forces an immediate weather refresh.</small>
</label>
<label>
  <span class="lbl">LED GPIO</span>
  <input name="led" type="number" min="-1" max="48" value="%LED%">
  <small>Active-low status LED. Steady = portal or network busy; fast blink = NTP sync; slow blink = WiFi connecting; 2 short = success; 3 long = error.</small>
</label>
</section>

<button type="submit">Save &amp; Restart</button>
</form>
</body>
</html>)html";

// ---------------------------------------------------------------------------
// Shared helpers
// ---------------------------------------------------------------------------

static bool pageServedFlag = false;

bool configServerPageServed() {
  bool v = pageServedFlag;
  pageServedFlag = false;
  return v;
}

static const int INTERVALS[] = {15, 30, 45, 60, 120, 240, 360, 720, 1440};
static const int INTERVALS_N = sizeof(INTERVALS) / sizeof(INTERVALS[0]);

static String buildPage(const Config& cfg) {
  String html;
  html.reserve(3500);
  html = FPSTR(HTML);
  html.replace("%SSID%",   String(cfg.wifiSsid));
  html.replace("%PASS%",   String(cfg.wifiPassword));
  html.replace("%SEL_C%",  cfg.tempUnits == 1 ? " selected" : "");
  html.replace("%SEL_F%",  cfg.tempUnits == 0 ? " selected" : "");
  html.replace("%FDAYS%",  String(cfg.forecastDays));
  String opts;
  for (int i = 0; i < INTERVALS_N; i++) {
    int v = INTERVALS[i];
    opts += "<option value=\"" + String(v) + "\"";
    if (v == cfg.updateInterval) opts += " selected";
    opts += ">" + String(v) + " min</option>";
  }
  html.replace("%INTERVAL_OPTIONS%", opts);
  html.replace("%SLEEP_CHK%", cfg.deepSleepMins > 0 ? "checked" : "");
  html.replace("%BTN%",  String(cfg.buttonPin));
  html.replace("%LED%",  String(cfg.ledPin));
  return html;
}

static void applyFormArgs(WebServer& server, Config& cfg) {
  if (server.hasArg("ssid"))
    strlcpy(cfg.wifiSsid,     server.arg("ssid").c_str(), sizeof(cfg.wifiSsid));
  if (server.hasArg("pass"))
    strlcpy(cfg.wifiPassword, server.arg("pass").c_str(), sizeof(cfg.wifiPassword));
  cfg.tempUnits    = server.arg("units").toInt();
  cfg.forecastDays = constrain(server.arg("fdays").toInt(), 1, 7);
  int iv = server.arg("interval").toInt();
  bool valid = false;
  for (int i = 0; i < INTERVALS_N; i++) if (INTERVALS[i] == iv) { valid = true; break; }
  cfg.updateInterval = valid ? iv : 30;
  cfg.deepSleepMins  = server.hasArg("sleep") ? cfg.updateInterval : -1;
  cfg.buttonPin      = server.arg("btn").toInt();
  cfg.ledPin         = server.arg("led").toInt();
}

static const char SAVED_HTML[] =
  "<!DOCTYPE html><html><body style='font-family:system-ui;padding:40px;text-align:center'>"
  "<h2>Saved!</h2><p>Rebooting&hellip;</p></body></html>";

// ---------------------------------------------------------------------------
// Blocking AP-mode portal (first boot / forced setup)
// ---------------------------------------------------------------------------

void runConfigPortal(Config& cfg, const char* apName) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(apName);
  IPAddress ip = WiFi.softAPIP();
  Serial.printf("Config portal: SSID=%s  IP=%s\n", apName, ip.toString().c_str());

  DNSServer dns;
  dns.start(53, "*", ip);

  WebServer server(80);

  server.on("/", HTTP_GET, [&]() {
    pageServedFlag = true;
    server.send(200, "text/html", buildPage(cfg));
  });

  server.on("/", HTTP_POST, [&]() {
    applyFormArgs(server, cfg);
    ConfigManager::save(cfg);
    server.send(200, "text/html", SAVED_HTML);
    delay(1500);
    ESP.restart();
  });

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

// ---------------------------------------------------------------------------
// Non-blocking STA-mode config server (runs alongside normal operation)
// ---------------------------------------------------------------------------

static WebServer* bgServer = nullptr;
static Config*    bgCfg    = nullptr;

void startConfigServer(Config& cfg) {
  if (bgServer) {
    bgServer->stop();
    delete bgServer;
  }
  bgCfg    = &cfg;
  bgServer = new WebServer(80);

  bgServer->on("/", HTTP_GET, []() {
    pageServedFlag = true;
    bgServer->send(200, "text/html", buildPage(*bgCfg));
  });

  bgServer->on("/save", HTTP_POST, []() {
    applyFormArgs(*bgServer, *bgCfg);
    ConfigManager::save(*bgCfg);
    bgServer->send(200, "text/html", SAVED_HTML);
    delay(1500);
    ESP.restart();
  });

  bgServer->onNotFound([]() {
    IPAddress ip = WiFi.localIP();
    bgServer->sendHeader("Location", String("http://") + ip.toString() + "/");
    bgServer->send(302, "text/plain", "");
  });

  bgServer->begin();
  Serial.printf("Config server started at http://%s/\n", WiFi.localIP().toString().c_str());
}

void handleConfigServer() {
  if (bgServer) bgServer->handleClient();
}
