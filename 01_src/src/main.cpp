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
#include "ota_manager.h"

struct tm timeinfo;

// All schedules use unsigned millis() deltas so they survive the 49-day
// rollover - a clock is expected to run for months without a reboot.
static uint32_t nextDisplayUpdate = 0;
static uint32_t nextSensorUpdate = 0;
static uint32_t nextStatusLed = 0;
static bool otaStarted = false;
static bool timeWasValid = false;

static inline bool due(uint32_t deadline) {
  return (int32_t)(millis() - deadline) >= 0;
}

// Forward declarations
void updateSensorData(float temp, float hum, float press, float lux);
void forceDisplayRefresh();

// Status LED: solid = online, slow blink = reconnecting, fast blink = setup AP.
static void updateStatusLed() {
  if (!due(nextStatusLed)) return;

  if (isWifiConnected()) {
    digitalWrite(PIN_LED, LED_ON);
    nextStatusLed = millis() + 500;
    return;
  }

  static bool on = false;
  on = !on;
  digitalWrite(PIN_LED, on ? LED_ON : LED_OFF);
  nextStatusLed = millis() + (isAPMode() ? 150 : 600);
}

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

  // Initialize display
  initDisplay();

  // Light every pixel so a dead LED can be spotted, then show the WiFi icon
  // while the connection comes up.
  ledSelfTest(5000);
  showWifi(strip.Color(0, 0, 0, 255));

  // Kick off the WiFi connection. This returns immediately - wifiLoop() drives
  // the retries, the AP fallback and the NTP sync from loop(), so boot is not
  // held up by an unreachable router.
  wifiInit();

  // Sensors
  printTSL();
  initBME688();

  // The web server (and with it the config portal) comes up right away, in
  // station mode or AP mode alike.
  initWebServer();
}

void loop()
{
  // Non-blocking WiFi state machine + captive portal DNS.
  wifiLoop();

  // ArduinoOTA can only be armed once we actually have an IP.
  if (isWifiConnected()) {
    if (!otaStarted) {
      initOTA();
      otaStarted = true;
    }
    handleOTA();
  }

  handleWebServer();
  updateStatusLed();

  // Log the first successful sync and force a redraw with the real time.
  if (!timeWasValid && isTimeValid()) {
    timeWasValid = true;
    Serial.println("Time synchronised");
    nextDisplayUpdate = millis();
  }

  // --- Display -------------------------------------------------------------
  // Driven by the RTC, not by the link state: once the time has been synced the
  // clock keeps showing it through a WiFi outage instead of falling back to a
  // placeholder.
  if (due(nextDisplayUpdate)) {
    if (isTimeValid() && getLocalTime(&timeinfo, 0)) {
      Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
      updateDisplay(timeinfo.tm_hour, timeinfo.tm_min);
      // Re-align to the top of the next minute.
      nextDisplayUpdate = millis() + (60 - timeinfo.tm_sec) * 1000UL;
    } else {
      // No time yet: show 12:00 as a placeholder and check back shortly.
      updateDisplay(12, 0);
      nextDisplayUpdate = millis() + 5000;
    }
  }

  // --- Sensors -------------------------------------------------------------
  if (due(nextSensorUpdate)) {
    uint32_t milliLux = printTSL();
    float lux = milliLux / 1000.0f;

    updateBrightness(lux);
    printBME688();

    updateSensorData(getTemperature(), getHumidity(), getPressure(), lux);

    nextSensorUpdate = millis() + 5000;
  }

  // Yield to the WiFi/TCP tasks.
  delay(5);
}

// Force an immediate display refresh (called from the web interface).
void forceDisplayRefresh() {
  if (isTimeValid() && getLocalTime(&timeinfo, 0)) {
    updateDisplay(timeinfo.tm_hour, timeinfo.tm_min);
  } else {
    updateDisplay(12, 0);
  }
}
