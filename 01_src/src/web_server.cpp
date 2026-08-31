#include "web_server.h"
#include <WebServer.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
#include <time.h>
#include "display.h"
#include "sensors.h"
#include "settings.h"
#include "wifi_manager.h"
#include "time_utils.h"

// External function from main.cpp
extern void forceDisplayRefresh();

WebServer server(80);

// Global variables for sensor data
struct SensorData {
  float temperature = 0;
  float humidity = 0;
  float pressure = 0;
  float lux = 0;
} sensorData;

// Dragging a colour slider fires a request per frame. The colour is applied to
// the strip immediately, but the NVS write is deferred until the user stops
// moving - otherwise a few seconds of fiddling costs hundreds of flash writes.
static bool     colorSavePending = false;
static uint32_t colorSaveAtMs = 0;
static const uint32_t COLOR_SAVE_DELAY_MS = 1500;

// Reboots requested from the browser are deferred so the HTTP response is
// actually flushed to the client first.
static bool     restartPending = false;
static uint32_t restartAtMs = 0;

// Read once at startup and kept in sync on save, so the status poll never
// touches flash.
static String cachedTimezone;

// WiFi changes are applied after the HTTP reply has gone out, because they
// drop the association the browser is talking to.
enum PendingWifiAction { WIFI_ACTION_NONE, WIFI_ACTION_APPLY, WIFI_ACTION_FORGET, WIFI_ACTION_RECONNECT };
static PendingWifiAction pendingWifiAction = WIFI_ACTION_NONE;
static uint32_t pendingWifiAtMs = 0;
static String pendingSsid;
static String pendingPass;

static inline bool due(uint32_t deadline) {
  return (int32_t)(millis() - deadline) >= 0;
}

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<meta name="viewport" content="width=device-width,initial-scale=1,viewport-fit=cover">
<meta name="theme-color" content="#0f1117">
<title>WordClock</title>
<style>
:root{
  --bg:#0f1117; --panel:#171a23; --panel-2:#1e222d; --line:#2a2f3d;
  --text:#e6e9f0; --muted:#8b93a7; --accent:#6d8bff; --accent-2:#a06dff;
  --ok:#3ddc97; --warn:#ffb454; --err:#ff6b6b; --radius:14px;
}
*{margin:0;padding:0;box-sizing:border-box}
body{
  font:15px/1.5 -apple-system,BlinkMacSystemFont,"Segoe UI",Roboto,Helvetica,Arial,sans-serif;
  background:var(--bg);color:var(--text);
  padding:16px 16px calc(24px + env(safe-area-inset-bottom));
  -webkit-font-smoothing:antialiased;
}
.wrap{max-width:760px;margin:0 auto;display:flex;flex-direction:column;gap:14px}

header{display:flex;align-items:center;gap:12px;padding:4px 2px 2px}
header h1{font-size:1.25rem;font-weight:600;letter-spacing:.2px}
header .clock{margin-left:auto;font-variant-numeric:tabular-nums;font-size:1.5rem;font-weight:600;color:var(--accent)}

.pill{display:inline-flex;align-items:center;gap:6px;font-size:.78rem;font-weight:600;
  padding:4px 10px;border-radius:999px;background:var(--panel-2);color:var(--muted);white-space:nowrap}
.pill .dot{width:8px;height:8px;border-radius:50%;background:var(--muted);flex:none}
.pill.ok .dot{background:var(--ok);box-shadow:0 0 8px var(--ok)}
.pill.warn .dot{background:var(--warn);box-shadow:0 0 8px var(--warn)}
.pill.err .dot{background:var(--err);box-shadow:0 0 8px var(--err)}
.pill.ok{color:var(--ok)} .pill.warn{color:var(--warn)} .pill.err{color:var(--err)}

.banner{background:rgba(255,180,84,.12);border:1px solid rgba(255,180,84,.35);color:var(--warn);
  padding:12px 14px;border-radius:var(--radius);font-size:.9rem}
.banner b{color:#ffd9a0}

.card{background:var(--panel);border:1px solid var(--line);border-radius:var(--radius);padding:16px}
.card>h2{font-size:.78rem;font-weight:700;letter-spacing:.09em;text-transform:uppercase;
  color:var(--muted);margin-bottom:14px;display:flex;align-items:center;gap:8px}
.card>h2 .right{margin-left:auto;font-weight:500;letter-spacing:0;text-transform:none;font-size:.8rem}

.grid{display:grid;gap:10px}
.grid.g2{grid-template-columns:repeat(auto-fit,minmax(150px,1fr))}
.grid.g4{grid-template-columns:repeat(auto-fit,minmax(110px,1fr))}

.stat{background:var(--panel-2);border-radius:10px;padding:12px;text-align:center}
.stat .k{font-size:.72rem;color:var(--muted);text-transform:uppercase;letter-spacing:.06em}
.stat .v{font-size:1.45rem;font-weight:600;margin-top:4px;font-variant-numeric:tabular-nums}
.stat .u{font-size:.75rem;color:var(--muted)}

.kv{display:flex;justify-content:space-between;gap:12px;padding:7px 0;border-bottom:1px solid var(--line);font-size:.88rem}
.kv:last-child{border-bottom:0}
.kv span:first-child{color:var(--muted)}
.kv span:last-child{font-variant-numeric:tabular-nums;text-align:right;word-break:break-all}

input,select,button{font:inherit;color:inherit}
input[type=text],input[type=password],select{
  width:100%;padding:11px 12px;background:var(--panel-2);border:1px solid var(--line);
  border-radius:10px;outline:none}
input[type=text]:focus,input[type=password]:focus,select:focus{border-color:var(--accent)}
select{appearance:none;background-image:linear-gradient(45deg,transparent 50%,var(--muted) 50%),linear-gradient(135deg,var(--muted) 50%,transparent 50%);
  background-position:calc(100% - 18px) 51%,calc(100% - 13px) 51%;background-size:5px 5px;background-repeat:no-repeat;padding-right:36px}
label.fl{display:block;font-size:.8rem;color:var(--muted);margin-bottom:6px}

button{cursor:pointer;border:none;border-radius:10px;padding:11px 16px;font-weight:600;
  background:var(--panel-2);border:1px solid var(--line);transition:filter .15s,transform .05s}
button:hover{filter:brightness(1.25)}
button:active{transform:translateY(1px)}
button.primary{background:linear-gradient(135deg,var(--accent),var(--accent-2));border-color:transparent;color:#fff}
button.danger{color:var(--err);border-color:rgba(255,107,107,.35)}
button:disabled{opacity:.5;cursor:not-allowed}
.row{display:flex;gap:10px;flex-wrap:wrap}
.row>*{flex:1;min-width:130px}

.seg{display:flex;background:var(--panel-2);border:1px solid var(--line);border-radius:10px;padding:4px;gap:4px}
.seg button{flex:1;background:transparent;border:none;padding:9px;color:var(--muted)}
.seg button.on{background:linear-gradient(135deg,var(--accent),var(--accent-2));color:#fff}

input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:6px;border-radius:3px;
  background:var(--panel-2);outline:none;margin:10px 0}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;width:20px;height:20px;border-radius:50%;
  background:var(--accent);cursor:pointer;border:3px solid var(--panel)}
input[type=range]::-moz-range-thumb{width:20px;height:20px;border-radius:50%;background:var(--accent);
  cursor:pointer;border:3px solid var(--panel)}
.slider .hd{display:flex;justify-content:space-between;font-size:.82rem}
.slider .hd b{color:var(--accent);font-variant-numeric:tabular-nums}

.swatches{display:flex;flex-wrap:wrap;gap:9px;margin-top:12px}
.sw{width:38px;height:38px;border-radius:9px;cursor:pointer;border:2px solid transparent;
  box-shadow:inset 0 0 0 1px rgba(255,255,255,.15)}
.sw:hover{border-color:var(--text)}
input[type=color]{width:64px;height:64px;padding:0;border:none;border-radius:12px;background:none;cursor:pointer;flex:none}
input[type=color]::-webkit-color-swatch-wrapper{padding:0}
input[type=color]::-webkit-color-swatch{border:1px solid var(--line);border-radius:12px}

.nets{display:flex;flex-direction:column;gap:6px;max-height:210px;overflow-y:auto;margin-bottom:10px}
.net{display:flex;align-items:center;gap:10px;padding:9px 11px;background:var(--panel-2);
  border:1px solid var(--line);border-radius:9px;cursor:pointer;font-size:.88rem}
.net:hover{border-color:var(--accent)}
.net .ss{flex:1;overflow:hidden;text-overflow:ellipsis;white-space:nowrap}
.net .rs{color:var(--muted);font-size:.78rem;font-variant-numeric:tabular-nums}
.bars{display:flex;align-items:flex-end;gap:2px;height:14px;flex:none}
.bars i{width:3px;background:var(--line);border-radius:1px}
.bars i.on{background:var(--ok)}
.bars i:nth-child(1){height:25%}.bars i:nth-child(2){height:50%}
.bars i:nth-child(3){height:75%}.bars i:nth-child(4){height:100%}

#toast{position:fixed;left:50%;bottom:24px;transform:translate(-50%,120%);
  background:var(--panel-2);border:1px solid var(--line);padding:11px 18px;border-radius:999px;
  font-size:.88rem;font-weight:500;box-shadow:0 10px 30px rgba(0,0,0,.5);
  transition:transform .25s ease;z-index:50;max-width:90vw;text-align:center}
#toast.show{transform:translate(-50%,0)}
#toast.ok{border-color:var(--ok);color:var(--ok)}
#toast.err{border-color:var(--err);color:var(--err)}
.hide{display:none!important}
.muted{color:var(--muted);font-size:.83rem}
</style>
</head>
<body>
<div class="wrap">

  <header>
    <h1>&#128347; WordClock</h1>
    <div class="clock" id="clock">--:--</div>
  </header>

  <div id="banner" class="banner hide">
    <b>Setup mode.</b> The clock is not on your network yet. Pick your WiFi below to connect it.
  </div>

  <div class="card">
    <h2>Status <span class="right" id="netPill"></span></h2>
    <div class="grid g4" style="margin-bottom:12px">
      <div class="stat"><div class="k">Signal</div><div class="v" id="sRssi">--</div><div class="u">dBm</div></div>
      <div class="stat"><div class="k">Uptime</div><div class="v" id="sUp">--</div><div class="u">since boot</div></div>
      <div class="stat"><div class="k">Free RAM</div><div class="v" id="sHeap">--</div><div class="u">KB</div></div>
      <div class="stat"><div class="k">Drop-outs</div><div class="v" id="sDrops">--</div><div class="u">since boot</div></div>
    </div>
    <div class="kv"><span>Network</span><span id="sSsid">--</span></div>
    <div class="kv"><span>IP address</span><span id="sIp">--</span></div>
    <div class="kv"><span>Time source</span><span id="sTime">--</span></div>
  </div>

  <div class="card">
    <h2>Sensors</h2>
    <div class="grid g4">
      <div class="stat"><div class="k">Temp</div><div class="v" id="temp">--</div><div class="u">&deg;C</div></div>
      <div class="stat"><div class="k">Humidity</div><div class="v" id="hum">--</div><div class="u">%</div></div>
      <div class="stat"><div class="k">Pressure</div><div class="v" id="pres">--</div><div class="u">hPa</div></div>
      <div class="stat"><div class="k">Light</div><div class="v" id="lux">--</div><div class="u">lux</div></div>
    </div>
  </div>

  <div class="card">
    <h2>Display mode</h2>
    <div class="seg">
      <button id="mWord" onclick="setMode('word')">Word clock</button>
      <button id="mDigital" onclick="setMode('digital')">Digital</button>
    </div>
  </div>

  <div class="card">
    <h2>Colour</h2>
    <div style="display:flex;gap:16px;align-items:center;flex-wrap:wrap">
      <input type="color" id="pick" value="#ffffff" oninput="fromPicker(false)" onchange="fromPicker(true)">
      <div style="flex:1;min-width:200px">
        <div class="slider">
          <div class="hd"><span>White channel</span><b id="wVal">255</b></div>
          <input type="range" id="w" min="0" max="255" value="255"
                 oninput="onW(false)" onchange="onW(true)">
        </div>
        <div class="muted">The RGB picker drives the colour LEDs, the slider adds the dedicated warm-white LED.</div>
      </div>
    </div>
    <div class="swatches" id="swatches"></div>
  </div>

  <div class="card">
    <h2>Brightness</h2>
    <div class="grid g2">
      <div class="slider"><div class="hd"><span>Max brightness</span><b id="vMaxB">255</b></div>
        <input type="range" id="maxB" min="1" max="255" value="255" oninput="lbl()"></div>
      <div class="slider"><div class="hd"><span>Min brightness</span><b id="vMinB">10</b></div>
        <input type="range" id="minB" min="1" max="255" value="10" oninput="lbl()"></div>
      <div class="slider"><div class="hd"><span>Max lux (full bright)</span><b id="vMaxL">300</b></div>
        <input type="range" id="maxL" min="20" max="1000" step="10" value="300" oninput="lbl()"></div>
      <div class="slider"><div class="hd"><span>Min lux (dimmest)</span><b id="vMinL">10</b></div>
        <input type="range" id="minL" min="1" max="200" value="10" oninput="lbl()"></div>
    </div>
    <button class="primary" style="width:100%;margin-top:14px" onclick="saveBright()">Save brightness</button>
  </div>

  <div class="card">
    <h2>WiFi <span class="right"><button id="scanBtn" style="padding:5px 12px;font-size:.8rem" onclick="scan()">Scan</button></span></h2>
    <div class="nets hide" id="nets"></div>
    <div class="grid" style="gap:10px">
      <div><label class="fl" for="ssid">Network name</label>
        <input type="text" id="ssid" maxlength="31" autocomplete="off" placeholder="SSID"></div>
      <div><label class="fl" for="pass">Password</label>
        <input type="password" id="pass" maxlength="63" autocomplete="off" placeholder="Leave empty for open networks"></div>
      <div class="row">
        <button class="primary" onclick="saveWifi()">Save &amp; connect</button>
        <button onclick="post('/api/wifi/reconnect',null,'Reconnecting...')">Reconnect</button>
        <button class="danger" onclick="forget()">Forget</button>
      </div>
    </div>
    <div class="muted" style="margin-top:10px">The clock applies new credentials without rebooting. If they are wrong it
      falls back to the <b>WordClock-Setup</b> access point automatically.</div>
  </div>

  <div class="card">
    <h2>Timezone</h2>
    <select id="tz">
      <option value="CET-1CEST,M3.5.0/02,M10.5.0/3">Europe/Berlin (CET/CEST)</option>
      <option value="GMT0BST,M3.5.0/1,M10.5.0">Europe/London (GMT/BST)</option>
      <option value="EET-2EEST,M3.5.0/3,M10.5.0/4">Europe/Helsinki (EET/EEST)</option>
      <option value="EST5EDT,M3.2.0,M11.1.0">America/New_York (EST/EDT)</option>
      <option value="CST6CDT,M3.2.0,M11.1.0">America/Chicago (CST/CDT)</option>
      <option value="MST7MDT,M3.2.0,M11.1.0">America/Denver (MST/MDT)</option>
      <option value="PST8PDT,M3.2.0,M11.1.0">America/Los_Angeles (PST/PDT)</option>
      <option value="AEST-10AEDT,M10.1.0,M4.1.0/3">Australia/Sydney (AEST/AEDT)</option>
      <option value="JST-9">Asia/Tokyo (JST)</option>
      <option value="CST-8">Asia/Shanghai (CST)</option>
      <option value="IST-5:30">Asia/Kolkata (IST)</option>
      <option value="UTC0">UTC</option>
    </select>
    <button class="primary" style="width:100%;margin-top:12px" onclick="saveTz()">Save timezone</button>
  </div>

  <div class="card">
    <h2>Maintenance</h2>
    <div class="row">
      <button onclick="location.href='/update'">&#128260; Firmware update</button>
      <button class="danger" onclick="restart()">Restart clock</button>
    </div>
  </div>
</div>

<div id="toast"></div>

<script>
var PRESETS=[[0,0,0,255,'#ffffff','Pure white'],[0,0,0,180,'#ffe9c9','Warm white'],
 [255,0,0,0,'#ff2d2d','Red'],[255,60,0,0,'#ff7a1a','Orange'],[210,150,0,0,'#ffd400','Yellow'],
 [0,255,0,0,'#28e04a','Green'],[0,255,190,0,'#20e6c8','Teal'],[0,180,255,0,'#28b6ff','Sky'],
 [40,0,255,0,'#4b3bff','Blue'],[190,0,255,0,'#c13bff','Violet'],[255,0,150,0,'#ff2db4','Pink']];

var st={r:0,g:0,b:0,w:255,mode:0};
var suppress=0;           // pause polling right after a local edit
var timer=null, tick=null, clockSecs=null;

function $(i){return document.getElementById(i)}
function toast(msg,cls){
  var t=$('toast'); t.textContent=msg; t.className='show '+(cls||'');
  clearTimeout(t._h); t._h=setTimeout(function(){t.className=''},2600);
}
function hex(r,g,b){
  return '#'+[r,g,b].map(function(v){return ('0'+(v|0).toString(16)).slice(-2)}).join('');
}
function fmtUp(s){
  var d=Math.floor(s/86400), h=Math.floor(s%86400/3600), m=Math.floor(s%3600/60);
  if(d)return d+'d '+h+'h';
  if(h)return h+'h '+m+'m';
  return m+'m';
}

/* ---- status polling ---------------------------------------------------- */
function render(d){
  var w=d.wifi;
  var cls = w.state=='connected' ? 'ok' : (d.ap ? 'warn' : 'err');
  $('netPill').innerHTML='<span class="pill '+cls+'"><span class="dot"></span>'+w.state+'</span>';
  $('banner').classList.toggle('hide', !d.ap || w.state=='connected');

  $('sRssi').textContent = w.rssi ? w.rssi : '--';
  $('sUp').textContent   = fmtUp(d.uptime);
  $('sHeap').textContent = Math.round(d.heap/1024);
  $('sDrops').textContent= w.drops;
  $('sSsid').textContent = w.ssid || 'not configured';
  $('sIp').textContent   = w.ip;
  $('sTime').textContent = d.timeValid ? 'NTP synced' : 'waiting for NTP';

  if(d.timeValid){ clockSecs = d.secs; }

  $('temp').textContent = d.temp.toFixed(1);
  $('hum').textContent  = d.hum.toFixed(1);
  $('pres').textContent = d.pres.toFixed(0);
  $('lux').textContent  = d.lux.toFixed(1);

  if(!suppress){
    st.r=d.color.r; st.g=d.color.g; st.b=d.color.b; st.w=d.color.w; st.mode=d.mode;
    $('pick').value=hex(st.r,st.g,st.b);
    $('w').value=st.w; $('wVal').textContent=st.w;
    $('mWord').classList.toggle('on', st.mode==0);
    $('mDigital').classList.toggle('on', st.mode==1);
    $('maxB').value=d.bright.maxB; $('minB').value=d.bright.minB;
    $('maxL').value=d.bright.maxL; $('minL').value=d.bright.minL; lbl();
    if(d.tz) $('tz').value=d.tz;
    if(!$('ssid').value && w.ssid) $('ssid').placeholder=w.ssid;
  }
}
function poll(){
  if(document.hidden) return;                       // do not wake the ESP32 for a hidden tab
  fetch('/api/status').then(function(r){return r.json()}).then(render).catch(function(){
    $('netPill').innerHTML='<span class="pill err"><span class="dot"></span>offline</span>';
  });
}
function showClock(){
  if(clockSecs===null){$('clock').textContent='--:--';return}
  var s=clockSecs%86400;
  $('clock').textContent=('0'+Math.floor(s/3600)).slice(-2)+':'+('0'+Math.floor(s%3600/60)).slice(-2);
  clockSecs++;
}

/* ---- actions ----------------------------------------------------------- */
function hold(){ suppress++; clearTimeout(hold._t); hold._t=setTimeout(function(){suppress=0},2500) }

function get(url,msg){
  return fetch(url).then(function(r){
    if(!r.ok) throw 0;
    if(msg) toast(msg,'ok');
    return r;
  }).catch(function(){ toast('Request failed','err') });
}
function post(url,body,msg){
  return fetch(url,{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:body||''})
    .then(function(r){ if(!r.ok) throw 0; if(msg) toast(msg,'ok'); setTimeout(poll,400); return r })
    .catch(function(){ toast('Request failed','err') });
}

var colorTimer=null;
function pushColor(persist){
  hold();
  clearTimeout(colorTimer);
  // Throttle while dragging; only the settling request is asked to persist.
  colorTimer=setTimeout(function(){
    fetch('/api/color?r='+st.r+'&g='+st.g+'&b='+st.b+'&w='+st.w+'&save='+(persist?1:0));
  }, persist?0:60);
}
function fromPicker(persist){
  var h=$('pick').value;
  st.r=parseInt(h.substr(1,2),16); st.g=parseInt(h.substr(3,2),16); st.b=parseInt(h.substr(5,2),16);
  pushColor(persist);
}
function onW(persist){
  st.w=parseInt($('w').value,10); $('wVal').textContent=st.w; pushColor(persist);
}
function preset(p){
  st.r=p[0];st.g=p[1];st.b=p[2];st.w=p[3];
  $('pick').value=hex(st.r,st.g,st.b); $('w').value=st.w; $('wVal').textContent=st.w;
  pushColor(true);
}
function setMode(m){
  hold(); st.mode = m=='word'?0:1;
  $('mWord').classList.toggle('on',st.mode==0);
  $('mDigital').classList.toggle('on',st.mode==1);
  get('/api/mode?mode='+m);
}
function lbl(){
  $('vMaxB').textContent=$('maxB').value; $('vMinB').textContent=$('minB').value;
  $('vMaxL').textContent=$('maxL').value; $('vMinL').textContent=$('minL').value;
}
function saveBright(){
  hold();
  post('/api/brightness','maxBright='+$('maxB').value+'&minBright='+$('minB').value+
       '&maxLux='+$('maxL').value+'&minLux='+$('minL').value,'Brightness saved');
}
function saveTz(){ hold(); post('/api/timezone','timezone='+encodeURIComponent($('tz').value),'Timezone saved') }

function saveWifi(){
  var s=$('ssid').value.trim();
  if(!s){ toast('Enter a network name','err'); return }
  toast('Connecting to '+s+'...');
  post('/api/wifi','ssid='+encodeURIComponent(s)+'&password='+encodeURIComponent($('pass').value));
  $('pass').value='';
  // The AP drops ~2 min after the station joins, so keep checking for a while.
  var n=0, iv=setInterval(function(){ poll(); if(++n>30) clearInterval(iv) },2000);
}
function forget(){
  if(!confirm('Erase the stored WiFi credentials and reopen the setup access point?')) return;
  post('/api/wifi/forget',null,'Credentials erased');
}
function restart(){
  if(!confirm('Restart the clock now?')) return;
  post('/api/restart',null,'Restarting...');
}

/* ---- WiFi scan --------------------------------------------------------- */
function bars(rssi){
  var n = rssi>=-55?4 : rssi>=-65?3 : rssi>=-75?2 : 1, h='';
  for(var i=1;i<=4;i++) h+='<i class="'+(i<=n?'on':'')+'"></i>';
  return '<span class="bars">'+h+'</span>';
}
function scan(){
  var btn=$('scanBtn'); btn.disabled=true; btn.textContent='Scanning';
  fetch('/api/scan?start=1');
  var tries=0;
  var iv=setInterval(function(){
    fetch('/api/scan').then(function(r){return r.json()}).then(function(d){
      if(d.running && ++tries<15) return;
      clearInterval(iv); btn.disabled=false; btn.textContent='Scan';
      var box=$('nets');
      if(!d.nets || !d.nets.length){ toast('No networks found','err'); box.classList.add('hide'); return }
      box.classList.remove('hide');
      box.innerHTML=d.nets.map(function(n){
        return '<div class="net" data-ssid="'+n.ssid.replace(/"/g,'&quot;')+'">'+bars(n.rssi)+
               '<span class="ss">'+n.ssid.replace(/</g,'&lt;')+'</span>'+
               '<span class="rs">'+n.rssi+' dBm'+(n.open?' &middot; open':'')+'</span></div>';
      }).join('');
      Array.prototype.forEach.call(box.children,function(el){
        el.onclick=function(){ $('ssid').value=el.dataset.ssid; $('pass').focus() };
      });
    }).catch(function(){ clearInterval(iv); btn.disabled=false; btn.textContent='Scan' });
  },900);
}

/* ---- boot -------------------------------------------------------------- */
$('swatches').innerHTML=PRESETS.map(function(p,i){
  return '<div class="sw" data-i="'+i+'" title="'+p[5]+'" style="background:'+p[4]+'"></div>';
}).join('');
Array.prototype.forEach.call($('swatches').children,function(el){
  el.onclick=function(){ preset(PRESETS[+el.dataset.i]) };
});

poll();
timer=setInterval(poll,3000);
tick=setInterval(showClock,1000);
document.addEventListener('visibilitychange',function(){ if(!document.hidden) poll() });
</script>
</body>
</html>
)rawliteral";

// --- Handlers ---------------------------------------------------------------
static void handleRoot() {
  // send_P streams straight out of flash - server.send() with a PROGMEM
  // pointer first copies the whole ~16 KB page into a heap String.
  server.sendHeader("Cache-Control", "no-cache");
  server.send_P(200, "text/html", HTML_PAGE, sizeof(HTML_PAGE) - 1);
}

static void handleStatus() {
  JsonDocument doc;

  JsonObject w = doc["wifi"].to<JsonObject>();
  w["state"]  = getWifiStateName();
  w["ssid"]   = getWifiSSID();
  w["ip"]     = getWifiIP();
  w["rssi"]   = getWifiRSSI();
  w["quality"]= getWifiQuality();
  w["drops"]  = getWifiDisconnectCount();
  w["reason"] = getLastDisconnectReason();

  doc["ap"]     = isAPMode();
  doc["uptime"] = (uint32_t)(millis() / 1000);
  doc["heap"]   = ESP.getFreeHeap();

  doc["timeValid"] = isTimeValid();
  struct tm t;
  // Seconds since local midnight - lets the browser run the clock itself
  // instead of polling for it.
  doc["secs"] = getLocalTime(&t, 0) ? (t.tm_hour * 3600 + t.tm_min * 60 + t.tm_sec) : 0;

  doc["temp"] = sensorData.temperature;
  doc["hum"]  = sensorData.humidity;
  doc["pres"] = sensorData.pressure;
  doc["lux"]  = sensorData.lux;

  // Everything below comes from RAM: the page polls a few times a minute and
  // reading NVS on each poll was a pointless flash hit.
  uint8_t r, g, b8, w8;
  getLedColor(r, g, b8, w8);
  JsonObject c = doc["color"].to<JsonObject>();
  c["r"] = r; c["g"] = g; c["b"] = b8; c["w"] = w8;
  doc["mode"] = getDisplayMode();
  doc["brightness"] = getCurrentBrightness();

  BrightnessSettings bs = getBrightnessSettings();
  JsonObject b = doc["bright"].to<JsonObject>();
  b["maxB"] = bs.maxBrightness; b["minB"] = bs.minBrightness;
  b["maxL"] = bs.maxLux;        b["minL"] = bs.minLux;

  doc["tz"] = cachedTimezone;

  String json;
  serializeJson(doc, json);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", json);
}

static void handleSetColor() {
  if (!(server.hasArg("r") && server.hasArg("g") && server.hasArg("b") && server.hasArg("w"))) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }
  uint8_t r = constrain(server.arg("r").toInt(), 0, 255);
  uint8_t g = constrain(server.arg("g").toInt(), 0, 255);
  uint8_t b = constrain(server.arg("b").toInt(), 0, 255);
  uint8_t w = constrain(server.arg("w").toInt(), 0, 255);

  setLedColor(r, g, b, w);
  forceDisplayRefresh();

  // Persist either right away (slider released) or once the drag settles.
  if (server.arg("save") == "1") {
    saveDisplaySettings(r, g, b, w, getDisplayMode());
    colorSavePending = false;
  } else {
    colorSavePending = true;
    colorSaveAtMs = millis() + COLOR_SAVE_DELAY_MS;
  }
  server.send(200, "text/plain", "ok");
}

static void handleSetMode() {
  if (!server.hasArg("mode")) {
    server.send(400, "text/plain", "Missing mode parameter");
    return;
  }
  String mode = server.arg("mode");
  uint8_t modeVal;
  if (mode == "word")         modeVal = 0;
  else if (mode == "digital") modeVal = 1;
  else { server.send(400, "text/plain", "Invalid mode"); return; }

  setDisplayMode(modeVal);
  DisplaySettings s = loadDisplaySettings();
  saveDisplaySettings(s.red, s.green, s.blue, s.white, modeVal);
  forceDisplayRefresh();
  server.send(200, "text/plain", "ok");
}

static void handleSaveWiFi() {
  if (!server.hasArg("ssid") || server.arg("ssid").length() == 0) {
    server.send(400, "text/plain", "Missing SSID");
    return;
  }
  // Applying credentials tears down the current association, which would kill
  // this very connection mid-reply. Answer now, act from handleWebServer().
  pendingSsid = server.arg("ssid");
  pendingPass = server.arg("password");
  pendingWifiAction = WIFI_ACTION_APPLY;
  pendingWifiAtMs = millis() + 300;
  server.send(200, "text/plain", "ok");
}

static void handleForgetWiFi() {
  pendingWifiAction = WIFI_ACTION_FORGET;
  pendingWifiAtMs = millis() + 300;
  server.send(200, "text/plain", "ok");
}

static void handleReconnect() {
  pendingWifiAction = WIFI_ACTION_RECONNECT;
  pendingWifiAtMs = millis() + 300;
  server.send(200, "text/plain", "ok");
}

static void handleSaveTimezone() {
  if (!server.hasArg("timezone")) {
    server.send(400, "text/plain", "Missing timezone");
    return;
  }
  String tz = server.arg("timezone");
  saveTimezoneSettings(tz.c_str());
  setTimezone(tz);
  cachedTimezone = tz;
  forceDisplayRefresh();
  server.send(200, "text/plain", "ok");
}

static void handleSaveBrightness() {
  if (!(server.hasArg("maxBright") && server.hasArg("minBright") &&
        server.hasArg("maxLux") && server.hasArg("minLux"))) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }
  uint8_t  maxBright = constrain(server.arg("maxBright").toInt(), 1, 255);
  uint8_t  minBright = constrain(server.arg("minBright").toInt(), 1, 255);
  uint16_t maxLux    = constrain(server.arg("maxLux").toInt(), 1, 5000);
  uint16_t minLux    = constrain(server.arg("minLux").toInt(), 1, 5000);
  // Keep the mapping monotonic even if the user crosses the sliders over.
  if (minBright > maxBright) { uint8_t t = minBright; minBright = maxBright; maxBright = t; }
  if (minLux > maxLux)       { uint16_t t = minLux; minLux = maxLux; maxLux = t; }

  saveBrightnessSettings(maxBright, minBright, maxLux, minLux);
  reloadBrightnessSettings();
  server.send(200, "text/plain", "ok");
}

static void handleScan() {
  if (server.hasArg("start")) {
    wifiStartScan();
    server.send(200, "application/json", "{\"running\":true,\"nets\":[]}");
    return;
  }
  int n = wifiScanStatus();
  if (n == WIFI_SCAN_RUNNING) {
    server.send(200, "application/json", "{\"running\":true,\"nets\":[]}");
    return;
  }
  server.send(200, "application/json", "{\"running\":false,\"nets\":" + wifiScanResultsJson() + "}");
}

static void handleRestart() {
  server.send(200, "text/plain", "ok");
  restartPending = true;
  restartAtMs = millis() + 500;
}

static void handleNotFound() {
  // In setup mode every stray request (including the OS connectivity checks)
  // is bounced to the config page, so the captive portal pops up by itself.
  if (isAPMode()) {
    server.sendHeader("Location", "http://" + getAPIP() + "/", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

// --- Lifecycle --------------------------------------------------------------
void initWebServer() {
  cachedTimezone = loadTimezoneSettings().timezone;

  server.on("/", handleRoot);
  server.on("/api/status", handleStatus);
  server.on("/api/color", handleSetColor);
  server.on("/api/mode", handleSetMode);
  server.on("/api/scan", handleScan);
  server.on("/api/wifi", HTTP_POST, handleSaveWiFi);
  server.on("/api/wifi/forget", HTTP_POST, handleForgetWiFi);
  server.on("/api/wifi/reconnect", HTTP_POST, handleReconnect);
  server.on("/api/timezone", HTTP_POST, handleSaveTimezone);
  server.on("/api/brightness", HTTP_POST, handleSaveBrightness);
  server.on("/api/restart", HTTP_POST, handleRestart);

  // Captive-portal probes used by iOS, Android and Windows.
  server.on("/generate_204", handleNotFound);
  server.on("/hotspot-detect.html", handleNotFound);
  server.on("/ncsi.txt", handleNotFound);
  server.onNotFound(handleNotFound);

  // ElegantOTA works in AP mode too, which is exactly when you may need it.
  ElegantOTA.begin(&server);
  ElegantOTA.setAutoReboot(true);

  server.begin();
  Serial.printf("[web] server started at http://%s (http://%s.local)\n",
                getWifiIP().c_str(), WIFI_HOSTNAME);
}

void handleWebServer() {
  server.handleClient();
  ElegantOTA.loop();

  if (colorSavePending && due(colorSaveAtMs)) {
    colorSavePending = false;
    // The live colour is authoritative - persist whatever the strip ended on.
    uint8_t r, g, b, w;
    getLedColor(r, g, b, w);
    saveDisplaySettings(r, g, b, w, getDisplayMode());
  }

  if (pendingWifiAction != WIFI_ACTION_NONE && due(pendingWifiAtMs)) {
    PendingWifiAction action = pendingWifiAction;
    pendingWifiAction = WIFI_ACTION_NONE;
    switch (action) {
      case WIFI_ACTION_APPLY:
        wifiApplyCredentials(pendingSsid.c_str(), pendingPass.c_str());
        pendingSsid = ""; pendingPass = "";
        break;
      case WIFI_ACTION_FORGET:
        clearWiFiSettings();
        wifiForceReconnect();
        break;
      case WIFI_ACTION_RECONNECT:
        wifiForceReconnect();
        break;
      default:
        break;
    }
  }

  if (restartPending && due(restartAtMs)) {
    Serial.println("[web] restart requested from web UI");
    ESP.restart();
  }
}

void updateSensorData(float temp, float hum, float press, float lux) {
  sensorData.temperature = temp;
  sensorData.humidity = hum;
  sensorData.pressure = press;
  sensorData.lux = lux;
}
