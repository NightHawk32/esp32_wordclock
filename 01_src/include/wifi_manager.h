#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>

void scanNetworks();
bool setup_wifi(const char* ssid, const char* password);
void startAPMode();
bool isAPMode();
String getAPIP();

#endif // WIFI_MANAGER_H
