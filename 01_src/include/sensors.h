#ifndef SENSORS_H
#define SENSORS_H

#include <Arduino.h>
#include <Wire.h>
#include <Tsl2561Util.h>
#include <Adafruit_BME680.h>

extern Tsl2561 Tsl;
extern Adafruit_BME680 bme;

void initBME688();
void printBME688();
uint32_t printTSL();
float getTemperature();
float getHumidity();
float getPressure();
uint32_t getGasResistance();

#endif // SENSORS_H
