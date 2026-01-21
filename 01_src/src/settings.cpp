#include "settings.h"
#include <Preferences.h>

Preferences preferences;

void initSettings() {
  preferences.begin("wordclock", false);
  Serial.println("Settings initialized");
}

void saveDisplaySettings(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t mode) {
  preferences.begin("wordclock", false);
  preferences.putUChar("led_r", r);
  preferences.putUChar("led_g", g);
  preferences.putUChar("led_b", b);
  preferences.putUChar("led_w", w);
  preferences.putUChar("disp_mode", mode);
  preferences.end();
  Serial.println("Display settings saved to flash");
}

DisplaySettings loadDisplaySettings() {
  DisplaySettings settings;
  preferences.begin("wordclock", true);
  settings.red = preferences.getUChar("led_r", 0);
  settings.green = preferences.getUChar("led_g", 0);
  settings.blue = preferences.getUChar("led_b", 0);
  settings.white = preferences.getUChar("led_w", 255);
  settings.mode = preferences.getUChar("disp_mode", 0);
  preferences.end();
  
  Serial.printf("Loaded display settings - R:%d G:%d B:%d W:%d Mode:%d\n", 
                settings.red, settings.green, settings.blue, settings.white, settings.mode);
  return settings;
}

void saveWiFiSettings(const char* ssid, const char* password) {
  preferences.begin("wordclock", false);
  preferences.putString("wifi_ssid", ssid);
  preferences.putString("wifi_pass", password);
  preferences.putBool("wifi_cfg", true);
  preferences.end();
  Serial.println("WiFi settings saved to flash");
}

WiFiSettings loadWiFiSettings() {
  WiFiSettings settings;
  preferences.begin("wordclock", true);
  settings.configured = preferences.getBool("wifi_cfg", false);
  
  if (settings.configured) {
    String ssid = preferences.getString("wifi_ssid", "");
    String password = preferences.getString("wifi_pass", "");
    
    ssid.toCharArray(settings.ssid, sizeof(settings.ssid));
    password.toCharArray(settings.password, sizeof(settings.password));
    
    Serial.printf("Loaded WiFi settings - SSID: %s\n", settings.ssid);
  } else {
    settings.ssid[0] = '\0';
    settings.password[0] = '\0';
    Serial.println("No WiFi settings configured");
  }
  
  preferences.end();
  return settings;
}

void saveBrightnessSettings(uint8_t maxBright, uint8_t minBright, uint16_t maxLux, uint16_t minLux) {
  preferences.begin("wordclock", false);
  preferences.putUChar("max_bright", maxBright);
  preferences.putUChar("min_bright", minBright);
  preferences.putUShort("max_lux", maxLux);
  preferences.putUShort("min_lux", minLux);
  preferences.end();
  Serial.printf("Brightness settings saved - MaxBright:%d MinBright:%d MaxLux:%d MinLux:%d\n",
                maxBright, minBright, maxLux, minLux);
}

BrightnessSettings loadBrightnessSettings() {
  BrightnessSettings settings;
  preferences.begin("wordclock", true);
  settings.maxBrightness = preferences.getUChar("max_bright", 255);
  settings.minBrightness = preferences.getUChar("min_bright", 10);
  settings.maxLux = preferences.getUShort("max_lux", 300);
  settings.minLux = preferences.getUShort("min_lux", 10);
  preferences.end();
  
  Serial.printf("Loaded brightness settings - MaxBright:%d MinBright:%d MaxLux:%d MinLux:%d\n",
                settings.maxBrightness, settings.minBrightness, settings.maxLux, settings.minLux);
  return settings;
}

void clearWiFiSettings() {
  preferences.begin("wordclock", false);
  preferences.remove("wifi_ssid");
  preferences.remove("wifi_pass");
  preferences.putBool("wifi_cfg", false);
  preferences.end();
  Serial.println("WiFi settings cleared");
}

void saveTimezoneSettings(const char* timezone) {
  preferences.begin("wordclock", false);
  preferences.putString("timezone", timezone);
  preferences.putBool("tz_cfg", true);
  preferences.end();
  Serial.printf("Timezone settings saved: %s\n", timezone);
}

TimezoneSettings loadTimezoneSettings() {
  TimezoneSettings settings;
  preferences.begin("wordclock", true);
  settings.configured = preferences.getBool("tz_cfg", false);
  
  if (settings.configured) {
    String tz = preferences.getString("timezone", "CET-1CEST,M3.5.0/02,M10.5.0/3");
    tz.toCharArray(settings.timezone, sizeof(settings.timezone));
    Serial.printf("Loaded timezone: %s\n", settings.timezone);
  } else {
    // Default to Central European Time
    strcpy(settings.timezone, "CET-1CEST,M3.5.0/02,M10.5.0/3");
    Serial.println("No timezone configured, using default: CET");
  }
  
  preferences.end();
  return settings;
}
