#include "web_server.h"
#include <WebServer.h>
#include <ESPmDNS.h>
#include <ArduinoJson.h>
#include <ElegantOTA.h>
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
  uint32_t gasResistance = 0;
} sensorData;

const char HTML_PAGE[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 WordClock Control</title>
  <style>
    * { margin: 0; padding: 0; box-sizing: border-box; }
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      padding: 20px;
    }
    .container {
      max-width: 800px;
      margin: 0 auto;
      background: white;
      border-radius: 20px;
      padding: 30px;
      box-shadow: 0 20px 60px rgba(0,0,0,0.3);
    }
    h1 {
      color: #333;
      text-align: center;
      margin-bottom: 30px;
      font-size: 2em;
    }
    .section {
      margin-bottom: 30px;
      padding: 20px;
      background: #f8f9fa;
      border-radius: 10px;
    }
    .section h2 {
      color: #667eea;
      margin-bottom: 15px;
      font-size: 1.3em;
    }
    .color-picker-container {
      display: flex;
      align-items: center;
      gap: 15px;
      flex-wrap: wrap;
    }
    .color-input {
      width: 80px;
      height: 80px;
      border: none;
      border-radius: 10px;
      cursor: pointer;
      box-shadow: 0 4px 10px rgba(0,0,0,0.2);
    }
    .color-channels {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(100px, 1fr));
      gap: 15px;
      flex: 1;
    }
    .channel {
      display: flex;
      flex-direction: column;
      gap: 5px;
    }
    .channel label {
      font-weight: 600;
      color: #555;
      font-size: 0.9em;
    }
    .channel input[type="number"] {
      padding: 8px;
      border: 2px solid #ddd;
      border-radius: 5px;
      font-size: 1em;
    }
    .sensor-grid {
      display: grid;
      grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
      gap: 15px;
    }
    .sensor-card {
      background: white;
      padding: 15px;
      border-radius: 10px;
      box-shadow: 0 2px 5px rgba(0,0,0,0.1);
      text-align: center;
    }
    .sensor-label {
      font-size: 0.9em;
      color: #666;
      margin-bottom: 5px;
    }
    .sensor-value {
      font-size: 1.8em;
      font-weight: bold;
      color: #667eea;
    }
    .sensor-unit {
      font-size: 0.9em;
      color: #999;
    }
    .mode-toggle {
      display: flex;
      gap: 10px;
      justify-content: center;
    }
    .mode-btn {
      flex: 1;
      padding: 15px 30px;
      border: none;
      border-radius: 10px;
      font-size: 1.1em;
      font-weight: 600;
      cursor: pointer;
      transition: all 0.3s;
      background: #e0e0e0;
      color: #666;
    }
    .mode-btn.active {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      box-shadow: 0 4px 15px rgba(102, 126, 234, 0.4);
    }
    .mode-btn:hover {
      transform: translateY(-2px);
    }
    .preset-colors {
      display: flex;
      gap: 10px;
      flex-wrap: wrap;
      margin-top: 10px;
    }
    .preset-btn {
      width: 50px;
      height: 50px;
      border: 3px solid white;
      border-radius: 8px;
      cursor: pointer;
      box-shadow: 0 2px 5px rgba(0,0,0,0.2);
      transition: transform 0.2s;
    }
    .preset-btn:hover {
      transform: scale(1.1);
    }
    .wifi-form {
      display: flex;
      flex-direction: column;
      gap: 15px;
    }
    .wifi-form input[type="text"],
    .wifi-form input[type="password"] {
      padding: 12px;
      border: 2px solid #ddd;
      border-radius: 8px;
      font-size: 1em;
    }
    .wifi-form button {
      padding: 12px;
      border: none;
      border-radius: 8px;
      font-size: 1em;
      font-weight: 600;
      cursor: pointer;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      transition: transform 0.2s;
    }
    .wifi-form button:hover {
      transform: translateY(-2px);
    }
    .status-message {
      padding: 10px;
      border-radius: 5px;
      margin-top: 10px;
      display: none;
    }
    .status-message.success {
      background: #d4edda;
      color: #155724;
      display: block;
    }
    .status-message.error {
      background: #f8d7da;
      color: #721c24;
      display: block;
    }
    .ap-mode-banner {
      background: #fff3cd;
      color: #856404;
      padding: 15px;
      border-radius: 10px;
      margin-bottom: 20px;
      text-align: center;
      font-weight: 600;
    }
  </style>
</head>
<body>
  <div class="container">
    <div id="apBanner" class="ap-mode-banner" style="display:none;">
      ⚠️ Access Point Mode - Configure WiFi to connect to your network
    </div>
    
    <h1>🕐 WordClock Control</h1>
    
    <div id="wifiSection" class="section" style="display:none;">
      <h2>WiFi Configuration</h2>
      <div class="wifi-form">
        <input type="text" id="wifiSsid" placeholder="WiFi Network Name (SSID)" maxlength="31">
        <input type="password" id="wifiPassword" placeholder="WiFi Password" maxlength="63">
        <button onclick="saveWiFi()">Connect to WiFi</button>
        <div id="wifiStatus" class="status-message"></div>
      </div>
    </div>
    
    <div class="section">
      <h2>Timezone Configuration</h2>
      <div class="wifi-form">
        <select id="timezoneSelect" style="padding: 12px; border: 2px solid #ddd; border-radius: 8px; font-size: 1em;">
          <option value="CET-1CEST,M3.5.0/02,M10.5.0/3">Europe/Berlin (CET/CEST)</option>
          <option value="GMT0BST,M3.5.0/1,M10.5.0">Europe/London (GMT/BST)</option>
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
        <button onclick="saveTimezone()">Save Timezone</button>
        <div id="timezoneStatus" class="status-message"></div>
      </div>
    </div>
    
    <div class="section">
      <h2>Brightness & Lux Settings</h2>
      <div class="wifi-form">
        <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 20px;">
          <div>
            <label style="font-weight: 600; color: #555; display: block; margin-bottom: 10px;">
              Max Brightness: <span id="maxBrightValue">255</span>
            </label>
            <input type="range" id="maxBrightness" min="0" max="255" value="255"
                   style="width: 100%;" oninput="updateBrightnessValue('max')">
          </div>
          <div>
            <label style="font-weight: 600; color: #555; display: block; margin-bottom: 10px;">
              Min Brightness: <span id="minBrightValue">10</span>
            </label>
            <input type="range" id="minBrightness" min="0" max="255" value="10"
                   style="width: 100%;" oninput="updateBrightnessValue('min')">
          </div>
          <div>
            <label style="font-weight: 600; color: #555; display: block; margin-bottom: 10px;">
              Max Lux: <span id="maxLuxValue">300</span>
            </label>
            <input type="range" id="maxLux" min="100" max="500" value="300" step="10"
                   style="width: 100%;" oninput="updateBrightnessValue('maxLux')">
          </div>
          <div>
            <label style="font-weight: 600; color: #555; display: block; margin-bottom: 10px;">
              Min Lux: <span id="minLuxValue">10</span>
            </label>
            <input type="range" id="minLux" min="1" max="100" value="10" step="1"
                   style="width: 100%;" oninput="updateBrightnessValue('minLux')">
          </div>
        </div>
        <button onclick="saveBrightnessSettings()" style="margin-top: 15px;">Save Brightness Settings</button>
        <div id="brightnessStatus" class="status-message"></div>
      </div>
    </div>
    
    <div class="section">
      <h2>Display Mode</h2>
      <div class="mode-toggle">
        <button class="mode-btn active" id="wordMode" onclick="setMode('word')">Word Clock</button>
        <button class="mode-btn" id="digitalMode" onclick="setMode('digital')">Digital Clock</button>
      </div>
    </div>

    <div class="section">
      <h2>LED Color</h2>
      <div class="color-picker-container">
        <input type="color" id="colorPicker" class="color-input" value="#ffffff" onchange="updateFromPicker()">
        <div class="color-channels">
          <div class="channel">
            <label>Red</label>
            <input type="number" id="red" min="0" max="255" value="0" onchange="updateColor()">
          </div>
          <div class="channel">
            <label>Green</label>
            <input type="number" id="green" min="0" max="255" value="0" onchange="updateColor()">
          </div>
          <div class="channel">
            <label>Blue</label>
            <input type="number" id="blue" min="0" max="255" value="0" onchange="updateColor()">
          </div>
          <div class="channel">
            <label>White</label>
            <input type="number" id="white" min="0" max="255" value="255" onchange="updateColor()">
          </div>
        </div>
      </div>
      <div class="preset-colors">
        <div class="preset-btn" style="background: white;" onclick="setPreset(0,0,0,255)" title="Pure White"></div>
        <div class="preset-btn" style="background: #ffccaa;" onclick="setPreset(0,0,0,200)" title="Warm White"></div>
        <div class="preset-btn" style="background: red;" onclick="setPreset(255,0,0,0)" title="Red"></div>
        <div class="preset-btn" style="background: orange;" onclick="setPreset(255,50,0,0)" title="Orange"></div>
        <div class="preset-btn" style="background: yellow;" onclick="setPreset(200,150,0,0)" title="Yellow"></div>
        <div class="preset-btn" style="background: lime;" onclick="setPreset(0,255,0,0)" title="Green"></div>
        <div class="preset-btn" style="background: cyan;" onclick="setPreset(0,255,255,0)" title="Cyan"></div>
        <div class="preset-btn" style="background: blue;" onclick="setPreset(0,0,255,0)" title="Blue"></div>
        <div class="preset-btn" style="background: magenta;" onclick="setPreset(255,0,255,0)" title="Magenta"></div>
        <div class="preset-btn" style="background: pink;" onclick="setPreset(255,100,150,0)" title="Pink"></div>
      </div>
    </div>

    <div class="section">
      <h2>Sensor Data</h2>
      <div class="sensor-grid">
        <div class="sensor-card">
          <div class="sensor-label">Temperature</div>
          <div class="sensor-value" id="temp">--</div>
          <div class="sensor-unit">°C</div>
        </div>
        <div class="sensor-card">
          <div class="sensor-label">Humidity</div>
          <div class="sensor-value" id="humidity">--</div>
          <div class="sensor-unit">%</div>
        </div>
        <div class="sensor-card">
          <div class="sensor-label">Pressure</div>
          <div class="sensor-value" id="pressure">--</div>
          <div class="sensor-unit">hPa</div>
        </div>
        <div class="sensor-card">
          <div class="sensor-label">Brightness</div>
          <div class="sensor-value" id="lux">--</div>
          <div class="sensor-unit">lux</div>
        </div>
      </div>
    </div>
    
    <div class="section">
      <h2>Firmware Update</h2>
      <p style="color: #666; margin-bottom: 15px;">Update your WordClock firmware over WiFi</p>
      <button onclick="window.location.href='/update'" style="width: 100%; padding: 15px; border: none; border-radius: 8px; font-size: 1.1em; font-weight: 600; cursor: pointer; background: linear-gradient(135deg, #667eea 0%, #764ba2 100%); color: white; transition: transform 0.2s;">
        🔄 Go to Firmware Update Page
      </button>
    </div>
  </div>

  <script>
    let currentMode = 'word';

    function checkAPMode() {
      fetch('/getStatus')
        .then(response => response.json())
        .then(data => {
          if (data.apMode) {
            document.getElementById('apBanner').style.display = 'block';
            document.getElementById('wifiSection').style.display = 'block';
          }
          // Load current timezone
          if (data.timezone) {
            document.getElementById('timezoneSelect').value = data.timezone;
          }
        });
    }

    function saveWiFi() {
      const ssid = document.getElementById('wifiSsid').value;
      const password = document.getElementById('wifiPassword').value;
      const statusDiv = document.getElementById('wifiStatus');
      
      if (!ssid) {
        statusDiv.className = 'status-message error';
        statusDiv.textContent = 'Please enter a WiFi network name';
        return;
      }
      
      fetch('/saveWiFi', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `ssid=${encodeURIComponent(ssid)}&password=${encodeURIComponent(password)}`
      })
      .then(response => response.text())
      .then(data => {
        statusDiv.className = 'status-message success';
        statusDiv.textContent = 'WiFi settings saved! Restarting...';
        setTimeout(() => {
          window.location.href = 'http://wordclock.local';
        }, 3000);
      })
      .catch(error => {
        statusDiv.className = 'status-message error';
        statusDiv.textContent = 'Failed to save WiFi settings';
      });
    }

    function saveTimezone() {
      const timezone = document.getElementById('timezoneSelect').value;
      const statusDiv = document.getElementById('timezoneStatus');
      
      fetch('/saveTimezone', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `timezone=${encodeURIComponent(timezone)}`
      })
      .then(response => response.text())
      .then(data => {
        statusDiv.className = 'status-message success';
        statusDiv.textContent = 'Timezone saved successfully!';
        setTimeout(() => {
          statusDiv.style.display = 'none';
        }, 3000);
      })
      .catch(error => {
        statusDiv.className = 'status-message error';
        statusDiv.textContent = 'Failed to save timezone';
      });
    }

    function updateBrightnessValue(type) {
      if (type === 'max') {
        const value = document.getElementById('maxBrightness').value;
        document.getElementById('maxBrightValue').textContent = value;
      } else if (type === 'min') {
        const value = document.getElementById('minBrightness').value;
        document.getElementById('minBrightValue').textContent = value;
      } else if (type === 'maxLux') {
        const value = document.getElementById('maxLux').value;
        document.getElementById('maxLuxValue').textContent = value;
      } else if (type === 'minLux') {
        const value = document.getElementById('minLux').value;
        document.getElementById('minLuxValue').textContent = value;
      }
    }

    function saveBrightnessSettings() {
      const maxBright = document.getElementById('maxBrightness').value;
      const minBright = document.getElementById('minBrightness').value;
      const maxLux = document.getElementById('maxLux').value;
      const minLux = document.getElementById('minLux').value;
      const statusDiv = document.getElementById('brightnessStatus');
      
      fetch('/saveBrightness', {
        method: 'POST',
        headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
        body: `maxBright=${maxBright}&minBright=${minBright}&maxLux=${maxLux}&minLux=${minLux}`
      })
      .then(response => response.text())
      .then(data => {
        statusDiv.className = 'status-message success';
        statusDiv.textContent = 'Brightness settings saved successfully!';
        setTimeout(() => {
          statusDiv.style.display = 'none';
        }, 3000);
      })
      .catch(error => {
        statusDiv.className = 'status-message error';
        statusDiv.textContent = 'Failed to save brightness settings';
      });
    }

    function loadBrightnessSettings() {
      fetch('/getBrightness')
        .then(response => response.json())
        .then(data => {
          document.getElementById('maxBrightness').value = data.maxBrightness;
          document.getElementById('minBrightness').value = data.minBrightness;
          document.getElementById('maxLux').value = data.maxLux;
          document.getElementById('minLux').value = data.minLux;
          document.getElementById('maxBrightValue').textContent = data.maxBrightness;
          document.getElementById('minBrightValue').textContent = data.minBrightness;
          document.getElementById('maxLuxValue').textContent = data.maxLux;
          document.getElementById('minLuxValue').textContent = data.minLux;
        })
        .catch(error => console.error('Error loading brightness settings:', error));
    }

    function setMode(mode) {
      currentMode = mode;
      document.getElementById('wordMode').classList.toggle('active', mode === 'word');
      document.getElementById('digitalMode').classList.toggle('active', mode === 'digital');
      
      fetch('/setMode?mode=' + mode)
        .then(response => response.text())
        .then(data => console.log('Mode set:', data));
    }

    function updateColor() {
      const r = document.getElementById('red').value;
      const g = document.getElementById('green').value;
      const b = document.getElementById('blue').value;
      const w = document.getElementById('white').value;
      
      fetch(`/setColor?r=${r}&g=${g}&b=${b}&w=${w}`)
        .then(response => response.text())
        .then(data => console.log('Color updated:', data));
    }

    function updateFromPicker() {
      const hex = document.getElementById('colorPicker').value;
      const r = parseInt(hex.substr(1,2), 16);
      const g = parseInt(hex.substr(3,2), 16);
      const b = parseInt(hex.substr(5,2), 16);
      
      document.getElementById('red').value = r;
      document.getElementById('green').value = g;
      document.getElementById('blue').value = b;
      document.getElementById('white').value = 0;
      
      updateColor();
    }

    function setPreset(r, g, b, w) {
      document.getElementById('red').value = r;
      document.getElementById('green').value = g;
      document.getElementById('blue').value = b;
      document.getElementById('white').value = w;
      updateColor();
    }

    function updateSensorData() {
      fetch('/getSensors')
        .then(response => response.json())
        .then(data => {
          document.getElementById('temp').textContent = data.temperature.toFixed(1);
          document.getElementById('humidity').textContent = data.humidity.toFixed(1);
          document.getElementById('pressure').textContent = data.pressure.toFixed(1);
          document.getElementById('lux').textContent = data.lux.toFixed(1);
        })
        .catch(error => console.error('Error:', error));
    }

    checkAPMode();
    loadBrightnessSettings();

    // Update sensor data every 2 seconds
    setInterval(updateSensorData, 2000);
    updateSensorData();
  </script>
</body>
</html>
)rawliteral";

void handleRoot() {
  server.send(200, "text/html", HTML_PAGE);
}

void handleSetColor() {
  if (server.hasArg("r") && server.hasArg("g") && server.hasArg("b") && server.hasArg("w")) {
    uint8_t r = server.arg("r").toInt();
    uint8_t g = server.arg("g").toInt();
    uint8_t b = server.arg("b").toInt();
    uint8_t w = server.arg("w").toInt();
    
    setLedColor(r, g, b, w);
    saveDisplaySettings(r, g, b, w, getDisplayMode());
    
    // Immediately refresh the display with new color
    forceDisplayRefresh();
    
    server.send(200, "text/plain", "Color updated");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleSetMode() {
  if (server.hasArg("mode")) {
    String mode = server.arg("mode");
    uint8_t modeVal;
    if (mode == "word") {
      modeVal = 0;
      setDisplayMode(0);
      server.send(200, "text/plain", "Mode set to word clock");
    } else if (mode == "digital") {
      modeVal = 1;
      setDisplayMode(1);
      server.send(200, "text/plain", "Mode set to digital clock");
    } else {
      server.send(400, "text/plain", "Invalid mode");
      return;
    }
    
    DisplaySettings settings = loadDisplaySettings();
    saveDisplaySettings(settings.red, settings.green, settings.blue, settings.white, modeVal);
    
    // Immediately refresh the display with new mode
    forceDisplayRefresh();
  } else {
    server.send(400, "text/plain", "Missing mode parameter");
  }
}

void handleGetSensors() {
  JsonDocument doc;
  doc["temperature"] = sensorData.temperature;
  doc["humidity"] = sensorData.humidity;
  doc["pressure"] = sensorData.pressure;
  doc["lux"] = sensorData.lux;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleGetStatus() {
  JsonDocument doc;
  doc["apMode"] = isAPMode();
  
  TimezoneSettings tzSettings = loadTimezoneSettings();
  doc["timezone"] = tzSettings.timezone;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void handleSaveWiFi() {
  if (server.hasArg("ssid")) {
    String ssid = server.arg("ssid");
    String password = server.arg("password");
    
    saveWiFiSettings(ssid.c_str(), password.c_str());
    server.send(200, "text/plain", "WiFi settings saved. Restarting...");
    
    delay(1000);
    ESP.restart();
  } else {
    server.send(400, "text/plain", "Missing SSID");
  }
}

void handleSaveTimezone() {
  if (server.hasArg("timezone")) {
    String timezone = server.arg("timezone");
    
    saveTimezoneSettings(timezone.c_str());
    setTimezone(timezone);
    server.send(200, "text/plain", "Timezone saved");
  } else {
    server.send(400, "text/plain", "Missing timezone");
  }
}

void handleSaveBrightness() {
  if (server.hasArg("maxBright") && server.hasArg("minBright") &&
      server.hasArg("maxLux") && server.hasArg("minLux")) {
    uint8_t maxBright = server.arg("maxBright").toInt();
    uint8_t minBright = server.arg("minBright").toInt();
    uint16_t maxLux = server.arg("maxLux").toInt();
    uint16_t minLux = server.arg("minLux").toInt();
    
    saveBrightnessSettings(maxBright, minBright, maxLux, minLux);
    server.send(200, "text/plain", "Brightness settings saved");
  } else {
    server.send(400, "text/plain", "Missing parameters");
  }
}

void handleGetBrightness() {
  BrightnessSettings settings = loadBrightnessSettings();
  
  JsonDocument doc;
  doc["maxBrightness"] = settings.maxBrightness;
  doc["minBrightness"] = settings.minBrightness;
  doc["maxLux"] = settings.maxLux;
  doc["minLux"] = settings.minLux;
  
  String json;
  serializeJson(doc, json);
  server.send(200, "application/json", json);
}

void initWebServer() {
  // Initialize mDNS
  if (!MDNS.begin("wordclock")) {
    Serial.println("Error setting up mDNS responder!");
  } else {
    Serial.println("mDNS responder started");
    MDNS.addService("http", "tcp", 80);
  }
  
  server.on("/", handleRoot);
  server.on("/setColor", handleSetColor);
  server.on("/setMode", handleSetMode);
  server.on("/getSensors", handleGetSensors);
  server.on("/getStatus", handleGetStatus);
  server.on("/saveWiFi", HTTP_POST, handleSaveWiFi);
  server.on("/saveTimezone", HTTP_POST, handleSaveTimezone);
  server.on("/saveBrightness", HTTP_POST, handleSaveBrightness);
  server.on("/getBrightness", handleGetBrightness);
  
  // Initialize ElegantOTA (only in station mode)
  if (!isAPMode()) {
    ElegantOTA.begin(&server);    // Start ElegantOTA
    Serial.println("ElegantOTA initialized");
    Serial.println("Access OTA update page at: http://wordclock.local/update");
  }
  
  server.begin();
  Serial.println("Web server started");
  
  if (isAPMode()) {
    Serial.print("Access at: http://");
    Serial.println(getAPIP());
  } else {
    Serial.print("Access at: http://");
    Serial.println(WiFi.localIP());
    Serial.println("Or access at: http://wordclock.local");
  }
}

void handleWebServer() {
  server.handleClient();
  // ElegantOTA is automatically handled by the server
}

void updateSensorData(float temp, float hum, float press, float lux) {
  sensorData.temperature = temp;
  sensorData.humidity = hum;
  sensorData.pressure = press;
  sensorData.lux = lux;
}
