#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>

void initWebServer();
void handleWebServer();
void updateSensorData(float temp, float hum, float press, float lux);

#endif // WEB_SERVER_H
