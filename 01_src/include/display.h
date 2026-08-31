#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include "settings.h"

// LED strip configuration
#define LED_COUNT  121
#define BRIGHTNESS 255
#define LED_COLOR 0, 0, 0, 255 //R G B W

extern Adafruit_NeoPixel strip;

// Display functions
void initDisplay();
int mapLed(int x, int y);
int showDigit(int digit, int x, int y);
void showHeart(uint32_t c);
void showWifi(uint32_t c);
void testLed();
void setStime(uint hour, uint min);
void setStimeDigital(uint hour, uint min);
void updateBrightness(float lux);
void reloadBrightnessSettings();
void restoreBrightness();
uint8_t getCurrentBrightness();
// Cached copy of the saved limits - avoids an NVS read on every status poll.
BrightnessSettings getBrightnessSettings();
void setLedColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void getLedColor(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w);
void setDisplayMode(uint8_t mode);
uint8_t getDisplayMode();
void updateDisplay(uint hour, uint min);
void forceDisplayRefresh();
void showConnectingAnimation();

#endif // DISPLAY_H
