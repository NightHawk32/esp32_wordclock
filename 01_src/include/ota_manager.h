#ifndef OTA_MANAGER_H
#define OTA_MANAGER_H

#include <Arduino.h>

// Initialize both ArduinoOTA and ElegantOTA
void initOTA();

// Handle ArduinoOTA (call in loop)
void handleOTA();

#endif // OTA_MANAGER_H
