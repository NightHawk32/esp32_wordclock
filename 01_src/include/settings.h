#ifndef SETTINGS_H
#define SETTINGS_H

#include <Arduino.h>

struct DisplaySettings {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t white;
  uint8_t mode; // 0 = word clock, 1 = digital
};

struct WiFiSettings {
  char ssid[32];
  char password[64];
  bool configured;
};

struct TimezoneSettings {
  char timezone[64];
  bool configured;
};

void initSettings();
void saveDisplaySettings(uint8_t r, uint8_t g, uint8_t b, uint8_t w, uint8_t mode);
DisplaySettings loadDisplaySettings();
void saveWiFiSettings(const char* ssid, const char* password);
WiFiSettings loadWiFiSettings();
void clearWiFiSettings();
void saveTimezoneSettings(const char* timezone);
TimezoneSettings loadTimezoneSettings();

#endif // SETTINGS_H
