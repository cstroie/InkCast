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

static const char FAVICON[] PROGMEM =
  "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 32 32'>"
  "<circle cx='12' cy='12' r='5' fill='#FFD700'/>"
  "<g stroke='#FFD700' stroke-width='2' stroke-linecap='round'>"
  "<line x1='12' y1='4' x2='12' y2='2'/>"
  "<line x1='12' y1='22' x2='12' y2='20'/>"
  "<line x1='4' y1='12' x2='2' y2='12'/>"
  "<line x1='22' y1='12' x2='20' y2='12'/>"
  "<line x1='6' y1='6' x2='4.5' y2='4.5'/>"
  "<line x1='18' y1='6' x2='19.5' y2='4.5'/>"
  "<line x1='6' y1='18' x2='4.5' y2='19.5'/>"
  "<line x1='18' y1='18' x2='19.5' y2='19.5'/>"
  "</g>"
  "<path d='M11 28 Q7 28 7 24.5 Q7 22 10 21.5 Q10.5 18.5 14 18.5 Q17 18.5 18 21 Q19.5 20.5 20.5 21.5 Q23 21.5 23 24.5 Q23 28 19 28 Z'"
  " fill='white' stroke='#bbb' stroke-width='1' stroke-linejoin='round'/>"
  "</svg>";

static const char HTML[] PROGMEM = R"html(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<link rel="icon" href="/favicon.svg" type="image/svg+xml">
<title>InkCast Setup</title>
<link rel="preconnect" href="https://fonts.googleapis.com">
<link href="https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;500;600&family=IBM+Plex+Sans:wght@400;500&display=swap" rel="stylesheet">
<style>
:root{--bg:#0d1117;--sf:#161b22;--sf2:#1c2128;--sf3:#21262d;--bd:#30363d;--bds:#21262d;--tx:#e6edf3;--tx2:#7d8590;--tx3:#484f58;--ac:#2f81f7;--acd:rgba(47,129,247,.13);--acr:rgba(47,129,247,.3);--dn:#f85149;--dnd:rgba(248,81,73,.13);--ok:#3fb950;--mn:'IBM Plex Mono',Consolas,monospace;--sn:'IBM Plex Sans',system-ui,sans-serif;--r:6px;--rs:4px;--rl:10px}
@media(prefers-color-scheme:light){:root{--bg:#f6f8fa;--sf:#fff;--sf2:#f6f8fa;--sf3:#eaeef2;--bd:#d0d7de;--bds:#eaeef2;--tx:#1f2328;--tx2:#59636e;--tx3:#818b98;--ac:#0969da;--acd:rgba(9,105,218,.09);--acr:rgba(9,105,218,.25);--dn:#d1242f;--dnd:rgba(209,36,47,.09);--ok:#1a7f37}}
*,*::before,*::after{box-sizing:border-box;margin:0;padding:0}
body{background:var(--bg);color:var(--tx);font-family:var(--sn);font-size:14px;line-height:1.5;min-height:100vh}
button{font-family:inherit;cursor:pointer}
.hdr{position:sticky;top:0;z-index:100;height:52px;background:var(--sf);border-bottom:1px solid var(--bd);display:flex;align-items:center;justify-content:space-between;padding:0 20px;gap:12px}
.hb{display:flex;align-items:center;gap:10px}
.ht{font-family:var(--mn);font-size:15px;font-weight:600;letter-spacing:-.01em}
.hbg{font-family:var(--mn);font-size:9.5px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;color:var(--tx3);background:var(--sf3);border:1px solid var(--bd);border-radius:var(--rs);padding:2px 7px}
.hs{display:flex;align-items:center;gap:7px}
.hd{width:7px;height:7px;border-radius:50%;background:var(--ok)}
.hi{font-family:var(--mn);font-size:11px;color:var(--tx3)}
.main{max-width:680px;margin:0 auto;padding:24px 16px 96px;display:flex;flex-direction:column;gap:14px}
.card{background:var(--sf);border:1px solid var(--bd);border-radius:var(--rl);overflow:hidden}
.ch{display:flex;align-items:center;justify-content:space-between;padding:11px 16px;background:var(--sf2);border-bottom:1px solid var(--bd)}
.cl{display:flex;align-items:center;gap:8px;font-family:var(--mn);font-size:10.5px;font-weight:600;letter-spacing:.1em;text-transform:uppercase;color:var(--tx2)}
.cl svg{color:var(--ac)}
.cs{font-family:var(--mn);font-size:11px;color:var(--tx3)}
.cb{padding:18px 16px;display:flex;flex-direction:column;gap:16px}
.fl{display:flex;flex-direction:column;gap:5px}
.flb{font-family:var(--mn);font-size:12px;font-weight:500;color:var(--tx2);letter-spacing:.02em}
.fh{font-size:11.5px;color:var(--tx3);line-height:1.45}
.fe{font-size:11.5px;color:var(--dn);margin-top:4px;display:none}
.g2{display:grid;grid-template-columns:1fr 1fr;gap:14px}
input[type=text],input[type=password],input[type=number],select{width:100%;padding:8px 11px;background:var(--sf2);border:1px solid var(--bd);border-radius:var(--r);color:var(--tx);font-family:var(--mn);font-size:13px;outline:none;appearance:none;-webkit-appearance:none;transition:border-color .1s,box-shadow .1s}
input[type=text]:focus,input[type=password]:focus,input[type=number]:focus,select:focus{border-color:var(--ac);box-shadow:0 0 0 3px var(--acr)}
input.err{border-color:var(--dn)!important}
input.err:focus{box-shadow:0 0 0 3px var(--dnd)!important}
input::placeholder{color:var(--tx3)}
select{background-image:url("data:image/svg+xml,%3Csvg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 16 16'%3E%3Cpath d='M4 6l4 4 4-4' fill='none' stroke='%23888' stroke-width='1.5' stroke-linecap='round' stroke-linejoin='round'/%3E%3C/svg%3E");background-repeat:no-repeat;background-position:right 10px center;background-size:16px;padding-right:30px;cursor:pointer}
.btn{display:inline-flex;align-items:center;justify-content:center;gap:6px;padding:7px 14px;border-radius:var(--r);font-family:var(--mn);font-size:12.5px;font-weight:500;border:1px solid transparent;white-space:nowrap;transition:filter .1s,background .1s}
.bp{background:var(--ac);color:#fff}
.bp:hover{filter:brightness(1.12)}
.bp:disabled{opacity:.45;cursor:not-allowed;filter:none}
.bg{background:transparent;color:var(--tx2);border-color:var(--bd)}
.bg:hover{background:var(--sf3);color:var(--tx)}
.bx{background:transparent;color:var(--tx3);border:none;padding:5px;border-radius:var(--rs)}
.bx:hover{background:var(--dnd);color:var(--dn)}
.nl{border:1px solid var(--bd);border-radius:var(--r);overflow:hidden}
.nr{display:flex;align-items:center;gap:10px;padding:8px 11px;border-bottom:1px solid var(--bds)}
.nr:last-child{border-bottom:none}
.ns{flex:1;font-family:var(--mn);font-size:13px;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.sp{border:1px solid var(--bd);border-radius:var(--r);overflow:hidden;margin-top:4px;background:var(--sf2);display:none}
.sr{display:flex;align-items:center;gap:10px;padding:8px 11px;cursor:pointer;border-bottom:1px solid var(--bds)}
.sr:last-child{border-bottom:none}
.sr:hover{background:var(--acd)}
.seg{display:flex;gap:2px;background:var(--sf2);border:1px solid var(--bd);border-radius:var(--r);padding:2px;width:fit-content}
.sb{padding:5px 18px;border:none;border-radius:var(--rs);font-family:var(--mn);font-size:13px;font-weight:500;color:var(--tx2);background:transparent}
.sb.active{background:var(--ac);color:#fff}
.sb:not(.active):hover{background:var(--sf3);color:var(--tx)}
.tr{display:flex;align-items:center;justify-content:space-between;gap:16px}
.ti{display:flex;flex-direction:column;gap:4px}
.tg{width:36px;height:20px;border-radius:10px;padding:0;border:1px solid var(--bd);background:var(--sf3);cursor:pointer;position:relative;flex-shrink:0;transition:background .15s,border-color .15s}
.tg[aria-checked=true]{background:var(--ac);border-color:var(--ac)}
.tg-k{position:absolute;top:2px;left:2px;width:14px;height:14px;border-radius:50%;background:#fff;box-shadow:0 1px 2px rgba(0,0,0,.3);transition:left .15s}
.tg[aria-checked=true] .tg-k{left:16px}
.pw{position:relative}
.pw input{padding-right:40px}
.pe{position:absolute;right:9px;top:50%;transform:translateY(-50%);background:none;border:none;color:var(--tx3);display:flex;padding:3px;border-radius:var(--rs)}
.pe:hover{color:var(--tx2)}
.sub{display:flex;align-items:center;gap:10px;font-family:var(--mn);font-size:10px;font-weight:600;letter-spacing:.08em;text-transform:uppercase;color:var(--tx3)}
.sub::after{content:'';flex:1;height:1px;background:var(--bd)}
.sbar{position:fixed;bottom:0;left:0;right:0;z-index:100;background:var(--sf);border-top:1px solid var(--bd);padding:11px 20px;display:flex;align-items:center;justify-content:space-between;gap:12px}
.sbn{font-family:var(--mn);font-size:11px;color:var(--tx3)}
</style>
</head>
<body>
<header class="hdr">
<div class="hb">
<svg width="26" height="26" viewBox="0 0 32 32" xmlns="http://www.w3.org/2000/svg">
<circle cx="12" cy="12" r="5" fill="#FFD700"/>
<g stroke="#FFD700" stroke-width="2" stroke-linecap="round">
<line x1="12" y1="4" x2="12" y2="2"/><line x1="12" y1="22" x2="12" y2="20"/>
<line x1="4" y1="12" x2="2" y2="12"/><line x1="22" y1="12" x2="20" y2="12"/>
<line x1="6" y1="6" x2="4.5" y2="4.5"/><line x1="18" y1="6" x2="19.5" y2="4.5"/>
<line x1="6" y1="18" x2="4.5" y2="19.5"/><line x1="18" y1="18" x2="19.5" y2="19.5"/>
</g>
<path d="M11 28 Q7 28 7 24.5 Q7 22 10 21.5 Q10.5 18.5 14 18.5 Q17 18.5 18 21 Q19.5 20.5 20.5 21.5 Q23 21.5 23 24.5 Q23 28 19 28 Z" fill="white" stroke="#bbb" stroke-width="1" stroke-linejoin="round"/>
</svg>
<span class="ht">InkCast</span>
<span class="hbg">Setup</span>
</div>
<div class="hs"><div class="hd"></div><span class="hi">%IP%</span></div>
</header>
<main class="main">
<div class="card">
<div class="ch">
<span class="cl"><svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><circle cx="12" cy="20" r=".5" fill="currentColor" stroke="none"/></svg>WiFi Networks</span>
<span class="cs">%WIFI_COUNT% / %WIFI_MAX% saved</span>
</div>
<div class="cb">
%WIFI_LIST%
%WIFI_ADD_FORM%
</div>
</div>
<form method="POST" action="/">
<div class="card">
<div class="ch">
<span class="cl"><svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M18 10h-1.26A8 8 0 1 0 9 20h9a5 5 0 0 0 0-10z"/></svg>Weather</span>
</div>
<div class="cb">
<div class="fl">
<label class="flb" for="city">City <span style="font-weight:400;opacity:.55">(optional)</span></label>
<input id="city" name="city" type="text" value="%CITY%" placeholder="e.g. Mangalia" autocomplete="off">
<p class="fh">Leave empty to detect location automatically from your IP address.</p>
</div>
<div class="fl">
<label class="flb">Temperature units</label>
<input type="hidden" name="units" id="units-val" value="%UNITS_VAL%">
<div class="seg">
<button type="button" class="sb%SEL_C%" onclick="setU(1,this)">&#176;C</button>
<button type="button" class="sb%SEL_F%" onclick="setU(0,this)">&#176;F</button>
</div>
</div>
<div class="g2">
<div class="fl">
<label class="flb" for="fdays">Forecast days (1&#8211;7)</label>
<input id="fdays" name="fdays" type="number" min="1" max="7" value="%FDAYS%">
<p class="fh">Display shows today&#39;s forecast only.</p>
</div>
<div class="fl">
<label class="flb" for="interval">Update interval</label>
<select id="interval" name="interval">%INTERVAL_OPTIONS%</select>
<p class="fh">How often to refresh weather data.</p>
</div>
</div>
<div class="fl">
<div class="tr">
<div class="ti">
<span class="flb">Deep sleep between refreshes</span>
<p class="fh">Device sleeps to save power. Sleep duration = update interval.</p>
</div>
<button type="button" role="switch" class="tg" aria-checked="%SLEEP_ARIA%" onclick="toggleSleep(this)"><div class="tg-k"></div></button>
<input type="hidden" name="sleep" id="sleep-val" value="%SLEEP_VAL%">
</div>
</div>
</div>
</div>
<div class="card">
<div class="ch">
<span class="cl"><svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><rect x="4" y="4" width="16" height="16" rx="2"/><rect x="9" y="9" width="6" height="6"/><line x1="9" y1="1" x2="9" y2="4"/><line x1="15" y1="1" x2="15" y2="4"/><line x1="9" y1="20" x2="9" y2="23"/><line x1="15" y1="20" x2="15" y2="23"/><line x1="20" y1="9" x2="23" y2="9"/><line x1="20" y1="14" x2="23" y2="14"/><line x1="1" y1="9" x2="4" y2="9"/><line x1="1" y1="14" x2="4" y2="14"/></svg>Hardware Pins</span>
<span class="cs">&#8722;1 = not connected</span>
</div>
<div class="cb">
<div class="g2">
<div class="fl">
<label class="flb" for="btn">Button GPIO</label>
<input id="btn" name="btn" type="number" min="-1" max="48" value="%BTN%">
<p class="fh">Hold at boot (&gt;1 s) to re-enter setup. Short press forces an immediate refresh.</p>
</div>
<div class="fl">
<label class="flb" for="led">LED GPIO</label>
<input id="led" name="led" type="number" min="-1" max="48" value="%LED%">
<p class="fh">Active-low. Steady = portal/fetch; slow = connecting; very fast = portal window; 2&#215; = ok; 3&#215; = error.</p>
</div>
</div>
</div>
</div>
<div class="sbar">
<span class="sbn">// restart required to apply changes</span>
<button type="submit" class="btn bp" style="padding:9px 28px;font-size:13px">Save &amp; Restart</button>
</div>
</form>
</main>
<script>
var WI='<svg width="13" height="13" viewBox="0 0 24 24" fill="none" stroke="var(--tx3)" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"><path d="M5 12.55a11 11 0 0 1 14.08 0"/><path d="M1.42 9a16 16 0 0 1 21.16 0"/><path d="M8.53 16.11a6 6 0 0 1 6.95 0"/><circle cx="12" cy="20" r=".5" fill="var(--tx3)" stroke="none"/></svg>';
var SAVED=%WIFI_SSIDS%;
function setU(v,btn){
  document.getElementById('units-val').value=v;
  btn.closest('.seg').querySelectorAll('.sb').forEach(function(b){b.classList.remove('active');});
  btn.classList.add('active');
}
function toggleSleep(btn){
  var c=btn.getAttribute('aria-checked')==='true';
  btn.setAttribute('aria-checked',String(!c));
  document.getElementById('sleep-val').value=c?'0':'1';
}
function doScan(){
  var btn=document.getElementById('sb'),sp=document.getElementById('sp');
  btn.disabled=true;btn.lastChild.nodeValue='Scanning…';
  fetch('/wifi/scan').then(function(r){return r.json();}).then(function(d){
    btn.disabled=false;btn.lastChild.nodeValue='Scan';
    sp.innerHTML='';
    if(!d.length){sp.style.display='none';return;}
    d.forEach(function(s){
      var row=document.createElement('div');row.className='sr';
      row.innerHTML=WI+'<span class="ns">'+s.replace(/&/g,'&amp;').replace(/</g,'&lt;')+'</span>';
      row.onclick=function(){document.getElementById('ns').value=s;sp.style.display='none';};
      sp.appendChild(row);
    });
    sp.style.display='block';
  }).catch(function(){btn.disabled=false;btn.lastChild.nodeValue='Scan';});
}
function doAdd(){
  var ns=document.getElementById('ns'),np=document.getElementById('np');
  var se=document.getElementById('ssid-err'),pe2=document.getElementById('pass-err');
  var s=ns.value.trim(),p=np.value,ok=true;
  se.style.display='none';pe2.style.display='none';
  ns.classList.remove('err');np.classList.remove('err');
  if(!s){se.textContent='SSID is required';se.style.display='block';ns.classList.add('err');ok=false;}
  else if(s.length>63){se.textContent='Max 63 characters';se.style.display='block';ns.classList.add('err');ok=false;}
  else if(SAVED.indexOf(s)!==-1){se.textContent='Already in list';se.style.display='block';ns.classList.add('err');ok=false;}
  if(p&&p.length<8){pe2.textContent='Min 8 characters for WPA2';pe2.style.display='block';np.classList.add('err');ok=false;}
  if(!ok)return;
  fetch('/wifi/add',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},
    body:'ssid='+encodeURIComponent(s)+'&pass='+encodeURIComponent(p)})
  .then(function(){location.reload();});
}
function togglePw(){
  var i=document.getElementById('np');
  var on=document.getElementById('eye-on'),off=document.getElementById('eye-off');
  if(i.type==='password'){i.type='text';on.style.display='none';off.style.display='';}
  else{i.type='password';on.style.display='';off.style.display='none';}
}
</script>
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

static const int  INTERVALS[]       = {15, 30, 45, 60, 120, 240, 360, 720, 1440};
static const char* INTERVAL_LABELS[] = {"15 min","30 min","45 min","1 h","2 h","4 h","6 h","12 h","24 h"};
static const int  INTERVALS_N       = sizeof(INTERVALS) / sizeof(INTERVALS[0]);

static String htmlEscape(const String& s) {
  String out;
  out.reserve(s.length());
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if      (c == '&') out += "&amp;";
    else if (c == '<') out += "&lt;";
    else if (c == '>') out += "&gt;";
    else if (c == '"') out += "&quot;";
    else               out += c;
  }
  return out;
}

static const char WIFI_ICON[] =
  "<svg width=\"13\" height=\"13\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"var(--tx3)\""
  " stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
  "<path d=\"M5 12.55a11 11 0 0 1 14.08 0\"/>"
  "<path d=\"M1.42 9a16 16 0 0 1 21.16 0\"/>"
  "<path d=\"M8.53 16.11a6 6 0 0 1 6.95 0\"/>"
  "<circle cx=\"12\" cy=\"20\" r=\".5\" fill=\"var(--tx3)\" stroke=\"none\"/>"
  "</svg>";

static const char X_ICON[] =
  "<svg width=\"12\" height=\"12\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\""
  " stroke-width=\"2.5\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
  "<line x1=\"18\" y1=\"6\" x2=\"6\" y2=\"18\"/>"
  "<line x1=\"6\" y1=\"6\" x2=\"18\" y2=\"18\"/>"
  "</svg>";

static String buildWifiList(const Config& cfg) {
  if (cfg.wifiCount == 0)
    return "<p class=\"fh\" style=\"color:var(--tx3)\">No networks saved yet.</p>";
  String s = "<div class=\"nl\">";
  for (int i = 0; i < cfg.wifiCount; i++) {
    s += "<div class=\"nr\">";
    s += WIFI_ICON;
    s += "<span class=\"ns\">";
    s += htmlEscape(String(cfg.wifi[i].ssid));
    s += "</span>"
         "<form method=\"POST\" action=\"/wifi/del\" style=\"display:inline\">"
         "<input type=\"hidden\" name=\"idx\" value=\"";
    s += String(i);
    s += "\"><button class=\"btn bx\" type=\"submit\">";
    s += X_ICON;
    s += "</button></form></div>";
  }
  s += "</div>";
  return s;
}

static String buildAddForm(const Config& cfg) {
  if (cfg.wifiCount >= WIFI_MAX)
    return "<p class=\"fh\" style=\"color:var(--tx3)\">Maximum "
           + String(WIFI_MAX) + " networks reached. Remove one to add another.</p>";
  return String(
    "<div class=\"sub\">Add network</div>"
    "<div class=\"fl\">"
    "<label class=\"flb\" for=\"ns\">Network (SSID)</label>"
    "<div style=\"display:flex;gap:8px\">"
    "<div style=\"flex:1\">"
    "<input id=\"ns\" type=\"text\" autocomplete=\"off\" placeholder=\"Type or select from scan\">"
    "<p class=\"fe\" id=\"ssid-err\"></p>"
    "</div>"
    "<button type=\"button\" class=\"btn bg\" id=\"sb\" onclick=\"doScan()\">"
    "<svg width=\"13\" height=\"13\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\""
    " stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
    "<polyline points=\"23 4 23 10 17 10\"/><polyline points=\"1 20 1 14 7 14\"/>"
    "<path d=\"M3.51 9a9 9 0 0 1 14.85-3.36L23 10M1 14l4.64 4.36A9 9 0 0 0 20.49 15\"/>"
    "</svg>Scan</button>"
    "</div>"
    "<div class=\"sp\" id=\"sp\"></div>"
    "</div>"
    "<div class=\"fl\">"
    "<label class=\"flb\" for=\"np\">Password</label>"
    "<div class=\"pw\">"
    "<input id=\"np\" type=\"password\" autocomplete=\"off\" placeholder=\"Leave blank for open network\">"
    "<button type=\"button\" class=\"pe\" onclick=\"togglePw()\" tabindex=\"-1\">"
    "<svg id=\"eye-on\" width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\""
    " stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
    "<path d=\"M1 12s4-8 11-8 11 8 11 8-4 8-11 8-11-8-11-8z\"/><circle cx=\"12\" cy=\"12\" r=\"3\"/>"
    "</svg>"
    "<svg id=\"eye-off\" width=\"14\" height=\"14\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\""
    " stroke-width=\"2\" stroke-linecap=\"round\" stroke-linejoin=\"round\" style=\"display:none\">"
    "<path d=\"M17.94 17.94A10.07 10.07 0 0 1 12 20c-7 0-11-8-11-8a18.45 18.45 0 0 1 5.06-5.94"
    "M9.9 4.24A9.12 9.12 0 0 1 12 4c7 0 11 8 11 8a18.5 18.5 0 0 1-2.16 3.19\"/>"
    "<line x1=\"1\" y1=\"1\" x2=\"23\" y2=\"23\"/>"
    "</svg>"
    "</button>"
    "</div>"
    "<p class=\"fe\" id=\"pass-err\"></p>"
    "</div>"
    "<div>"
    "<button type=\"button\" class=\"btn bp\" onclick=\"doAdd()\">"
    "<svg width=\"13\" height=\"13\" viewBox=\"0 0 24 24\" fill=\"none\" stroke=\"currentColor\""
    " stroke-width=\"2.5\" stroke-linecap=\"round\" stroke-linejoin=\"round\">"
    "<line x1=\"12\" y1=\"5\" x2=\"12\" y2=\"19\"/><line x1=\"5\" y1=\"12\" x2=\"19\" y2=\"12\"/>"
    "</svg>Add network</button>"
    "</div>"
  );
}

static String buildPage(const Config& cfg) {
  String html;
  html.reserve(8000);
  html = FPSTR(HTML);

  IPAddress ip = WiFi.localIP();
  if (ip == IPAddress(0, 0, 0, 0)) ip = WiFi.softAPIP();
  html.replace("%IP%",         ip.toString());

  String ssidJson = "[";
  for (int i = 0; i < cfg.wifiCount; i++) {
    if (i > 0) ssidJson += ",";
    String s = String(cfg.wifi[i].ssid);
    s.replace("\\", "\\\\"); s.replace("\"", "\\\"");
    ssidJson += "\"" + s + "\"";
  }
  ssidJson += "]";
  html.replace("%WIFI_SSIDS%",  ssidJson);
  html.replace("%WIFI_COUNT%", String(cfg.wifiCount));
  html.replace("%WIFI_MAX%",   String(WIFI_MAX));
  html.replace("%WIFI_LIST%",  buildWifiList(cfg));
  html.replace("%WIFI_ADD_FORM%", buildAddForm(cfg));

  html.replace("%CITY%",       htmlEscape(String(cfg.city)));
  html.replace("%UNITS_VAL%",  String(cfg.tempUnits));
  html.replace("%SEL_C%",      cfg.tempUnits == 1 ? " active" : "");
  html.replace("%SEL_F%",      cfg.tempUnits == 0 ? " active" : "");
  html.replace("%FDAYS%",      String(cfg.forecastDays));

  String opts;
  for (int i = 0; i < INTERVALS_N; i++) {
    int v = INTERVALS[i];
    opts += "<option value=\"" + String(v) + "\"";
    if (v == cfg.updateInterval) opts += " selected";
    opts += ">" + String(INTERVAL_LABELS[i]) + "</option>";
  }
  html.replace("%INTERVAL_OPTIONS%", opts);

  bool sleeping = cfg.deepSleepMins > 0;
  html.replace("%SLEEP_ARIA%", sleeping ? "true" : "false");
  html.replace("%SLEEP_VAL%",  sleeping ? "1"    : "0");

  html.replace("%BTN%",        String(cfg.buttonPin));
  html.replace("%LED%",        String(cfg.ledPin));
  return html;
}

static void applyFormArgs(WebServer& server, Config& cfg) {
  strlcpy(cfg.city, server.arg("city").c_str(), sizeof(cfg.city));
  cfg.tempUnits    = server.arg("units").toInt();
  cfg.forecastDays = constrain(server.arg("fdays").toInt(), 1, 7);
  int iv = server.arg("interval").toInt();
  bool valid = false;
  for (int i = 0; i < INTERVALS_N; i++) if (INTERVALS[i] == iv) { valid = true; break; }
  cfg.updateInterval = valid ? iv : 30;
  cfg.deepSleepMins  = (server.arg("sleep") == "1") ? cfg.updateInterval : -1;
  cfg.buttonPin      = server.arg("btn").toInt();
  cfg.ledPin         = server.arg("led").toInt();
}

// Handle POST /wifi/add — add a network to cfg and save; redirect to /
static void handleWifiAdd(WebServer& server, Config& cfg) {
  String ssid = server.arg("ssid");
  ssid.trim();
  if (ssid.length() == 0 || cfg.wifiCount >= WIFI_MAX) {
    server.sendHeader("Location", "/");
    server.send(303, "text/plain", " ");
    return;
  }
  for (int i = 0; i < cfg.wifiCount; i++) {
    if (ssid == cfg.wifi[i].ssid) {
      server.sendHeader("Location", "/");
      server.send(303, "text/plain", " ");
      return;
    }
  }
  strlcpy(cfg.wifi[cfg.wifiCount].ssid, ssid.c_str(), sizeof(cfg.wifi[0].ssid));
  strlcpy(cfg.wifi[cfg.wifiCount].pass, server.arg("pass").c_str(), sizeof(cfg.wifi[0].pass));
  cfg.wifiCount++;
  ConfigManager::save(cfg);
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", " ");
}

// Handle POST /wifi/del — remove network by idx, compact, save; redirect to /
static void handleWifiDel(WebServer& server, Config& cfg) {
  int idx = server.arg("idx").toInt();
  if (idx >= 0 && idx < cfg.wifiCount) {
    for (int i = idx; i < cfg.wifiCount - 1; i++)
      cfg.wifi[i] = cfg.wifi[i + 1];
    cfg.wifiCount--;
  }
  ConfigManager::save(cfg);
  server.sendHeader("Location", "/");
  server.send(303, "text/plain", " ");
}

// Handle GET /wifi/scan — return JSON array of visible SSIDs
static void handleWifiScan(WebServer& server) {
  int n = WiFi.scanNetworks(/*async=*/false, /*show_hidden=*/false);
  String json = "[";
  for (int i = 0; i < n && n > 0; i++) {
    if (i > 0) json += ",";
    String s = WiFi.SSID(i);
    s.replace("\\", "\\\\"); s.replace("\"", "\\\"");
    json += "\"" + s + "\"";
  }
  json += "]";
  WiFi.scanDelete();
  server.send(200, "application/json", json);
}

static const char SAVED_HTML[] =
  "<!DOCTYPE html><html lang=\"en\"><head>"
  "<meta charset=\"utf-8\">"
  "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
  "<meta http-equiv=\"refresh\" content=\"30;url=/\">"
  "<link rel=\"icon\" href=\"/favicon.svg\" type=\"image/svg+xml\">"
  "<title>InkCast Setup</title>"
  "<link rel=\"preconnect\" href=\"https://fonts.googleapis.com\">"
  "<link href=\"https://fonts.googleapis.com/css2?family=IBM+Plex+Mono:wght@400;600&family=IBM+Plex+Sans:wght@400&display=swap\" rel=\"stylesheet\">"
  "<style>"
  ":root{--bg:#0d1117;--bd:#30363d;--tx:#e6edf3;--tx2:#7d8590;--ac:#2f81f7;"
  "--mn:'IBM Plex Mono',Consolas,monospace;--sn:'IBM Plex Sans',system-ui,sans-serif}"
  "@media(prefers-color-scheme:light){:root{--bg:#f6f8fa;--bd:#d0d7de;--tx:#1f2328;--tx2:#59636e;--ac:#0969da}}"
  "*{box-sizing:border-box;margin:0;padding:0}"
  "body{background:var(--bg);color:var(--tx);font-family:var(--sn);min-height:100vh;"
  "display:flex;flex-direction:column;align-items:center;justify-content:center;gap:14px}"
  ".t{font-family:var(--mn);font-size:18px;font-weight:600}"
  ".s{font-size:13px;color:var(--tx2)}"
  ".bar{width:220px;height:2px;background:var(--bd);border-radius:1px;overflow:hidden;margin-top:4px}"
  ".prog{height:100%;background:var(--ac);border-radius:1px;animation:p 2.5s ease-in-out forwards}"
  "@keyframes p{from{width:0}to{width:100%}}"
  "</style></head><body>"
  "<svg width=\"48\" height=\"48\" viewBox=\"0 0 32 32\" xmlns=\"http://www.w3.org/2000/svg\" style=\"opacity:.6\">"
  "<circle cx=\"12\" cy=\"12\" r=\"5\" fill=\"#FFD700\"/>"
  "<g stroke=\"#FFD700\" stroke-width=\"2\" stroke-linecap=\"round\">"
  "<line x1=\"12\" y1=\"4\" x2=\"12\" y2=\"2\"/><line x1=\"12\" y1=\"22\" x2=\"12\" y2=\"20\"/>"
  "<line x1=\"4\" y1=\"12\" x2=\"2\" y2=\"12\"/><line x1=\"22\" y1=\"12\" x2=\"20\" y2=\"12\"/>"
  "<line x1=\"6\" y1=\"6\" x2=\"4.5\" y2=\"4.5\"/><line x1=\"18\" y1=\"6\" x2=\"19.5\" y2=\"4.5\"/>"
  "<line x1=\"6\" y1=\"18\" x2=\"4.5\" y2=\"19.5\"/><line x1=\"18\" y1=\"18\" x2=\"19.5\" y2=\"19.5\"/>"
  "</g>"
  "<path d=\"M11 28 Q7 28 7 24.5 Q7 22 10 21.5 Q10.5 18.5 14 18.5 Q17 18.5 18 21 Q19.5 20.5 20.5 21.5 Q23 21.5 23 24.5 Q23 28 19 28 Z\""
  " fill=\"white\" stroke=\"#bbb\" stroke-width=\"1\" stroke-linejoin=\"round\"/>"
  "</svg>"
  "<span class=\"t\">Configuration saved</span>"
  "<span class=\"s\">Device is rebooting&hellip;</span>"
  "<div class=\"bar\"><div class=\"prog\"></div></div>"
  "</body></html>";

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

  server.on("/wifi/add", HTTP_POST, [&]() { handleWifiAdd(server, cfg); });
  server.on("/wifi/del", HTTP_POST, [&]() { handleWifiDel(server, cfg); });
  server.on("/wifi/scan", HTTP_GET, [&]() { handleWifiScan(server); });

  auto serveFavicon = [&]() { server.send_P(200, "image/svg+xml", FAVICON); };
  server.on("/favicon.svg", HTTP_GET, serveFavicon);
  server.on("/favicon.ico", HTTP_GET, serveFavicon);

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

  bgServer->on("/", HTTP_POST, []() {
    applyFormArgs(*bgServer, *bgCfg);
    ConfigManager::save(*bgCfg);
    bgServer->send(200, "text/html", SAVED_HTML);
    delay(1500);
    ESP.restart();
  });

  bgServer->on("/wifi/add",  HTTP_POST, []() { handleWifiAdd(*bgServer, *bgCfg); });
  bgServer->on("/wifi/del",  HTTP_POST, []() { handleWifiDel(*bgServer, *bgCfg); });
  bgServer->on("/wifi/scan", HTTP_GET,  []() { handleWifiScan(*bgServer); });

  bgServer->on("/favicon.svg", HTTP_GET, []() { bgServer->send_P(200, "image/svg+xml", FAVICON); });
  bgServer->on("/favicon.ico", HTTP_GET, []() { bgServer->send_P(200, "image/svg+xml", FAVICON); });

  bgServer->onNotFound([]() {
    IPAddress ip = WiFi.localIP();
    if (ip == IPAddress(0, 0, 0, 0)) ip = WiFi.softAPIP();
    bgServer->sendHeader("Location", String("http://") + ip.toString() + "/");
    bgServer->send(302, "text/plain", "");
  });

  bgServer->begin();
  Serial.printf("Config server started at http://%s/\n", WiFi.localIP().toString().c_str());
}

void handleConfigServer() {
  if (bgServer) bgServer->handleClient();
}
