#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Wire.h>

#include "board.h"
#include "wifi_manager.h"
#include "time_utils.h"
#include "sensors.h"
#include "display.h"
#include "web_server.h"
#include "settings.h"

bool ledState = LED_OFF;
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 0;
const char* NTP_SERVER = "pool.ntp.org";

struct tm timeinfo;
long nextUpdate = 0;
long nextSensorUpdate = 0;

// Forward declaration
void updateSensorData(float temp, float hum, float press, float lux);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting ....");

  pinMode(PIN_LED, OUTPUT);
  digitalWrite(PIN_LED, LED_OFF);

  Wire.begin(PIN_SDA, PIN_SCL);
  
  // Initialize settings (flash storage)
  initSettings();
  
  // Load and apply saved display settings
  DisplaySettings displaySettings = loadDisplaySettings();
  setLedColor(displaySettings.red, displaySettings.green, displaySettings.blue, displaySettings.white);
  setDisplayMode(displaySettings.mode);
  Serial.printf("Loaded settings - Mode: %d, RGBW: %d,%d,%d,%d\n", 
                displaySettings.mode, displaySettings.red, displaySettings.green, 
                displaySettings.blue, displaySettings.white);
  
  Serial.println("Setup done ....");

  // Initialize display
  initDisplay();

  // Load WiFi settings and attempt connection
  WiFiSettings wifiSettings = loadWiFiSettings();
  bool wifiConnected = false;
  
  if (wifiSettings.configured) {
    Serial.println("Attempting to connect to saved WiFi...");
    wifiConnected = setup_wifi(wifiSettings.ssid, wifiSettings.password);
  }
  
  if (!wifiConnected) {
    Serial.println("No WiFi configured or connection failed, starting AP mode");
    startAPMode();
  } else {
    // Only init time if WiFi connected
    configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);
    
    // Load and apply timezone settings
    TimezoneSettings tzSettings = loadTimezoneSettings();
    initTime(tzSettings.timezone);
    Serial.printf("Using timezone: %s\n", tzSettings.timezone);
  }
  
  // Initialize sensors
  printTSL();
  initBME688();
  
  // Initialize web server
  initWebServer();
}

void loop()
{  
  // Handle web server requests
  handleWebServer();
  
  // Check WiFi connection (only in station mode)
  if (!isAPMode()) {
    if(WiFi.status() != WL_CONNECTED){
      digitalWrite(PIN_LED, LED_OFF);
      Serial.println("WiFi disconnected, attempting to reconnect...");
      WiFiSettings wifiSettings = loadWiFiSettings();
      if (wifiSettings.configured) {
        setup_wifi(wifiSettings.ssid, wifiSettings.password);
      }
    } else {
      digitalWrite(PIN_LED, LED_ON);
    }
  } else {
    digitalWrite(PIN_LED, LED_ON);
  }

  long currentTimestamp = millis();

  // Update time display (only if not in AP mode)
  if (!isAPMode() && currentTimestamp > nextUpdate){
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    nextUpdate = millis() + (60-timeinfo.tm_sec)*1000;

    updateDisplay(timeinfo.tm_hour, timeinfo.tm_min);
  }

  // Update brightness and read sensors every 5 seconds
  if(currentTimestamp > nextSensorUpdate){
    uint32_t milliLux = printTSL();
    float lux = milliLux / 1000.0f;
    Serial.printf("Lux: %.2f\n", lux);
    
    updateBrightness(lux);
    printBME688();
    
    // Update web interface with sensor data
    updateSensorData(getTemperature(), getHumidity(), getPressure(), lux);
    
    nextSensorUpdate = currentTimestamp + 5000; // Update every 5 seconds
  }
}
