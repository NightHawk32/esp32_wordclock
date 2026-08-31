#include "display.h"
#include "board.h"
#include "time_led.h"
#include "digits.h"
#include "icons.h"
#include "settings.h"

Adafruit_NeoPixel strip(LED_COUNT, LED_BUS_PIN, NEO_GRBW + NEO_KHZ800);

// Custom LED color (R, G, B, W)
static uint8_t customLedColor[4] = {0, 0, 0, 255};
static uint8_t displayMode = 0; // 0 = word clock, 1 = digital

// Brightness limits are cached in RAM: they used to be re-read from NVS on
// every sensor tick, which is a flash read every 5 s for values that only
// change when the user saves them.
static BrightnessSettings brightnessCfg;
// Last brightness computed from ambient light, so an effect that takes the
// strip over (the connect animation) can hand it back unchanged.
static uint8_t currentBrightness = 255;

void reloadBrightnessSettings() {
  brightnessCfg = loadBrightnessSettings();
}

uint8_t getCurrentBrightness() {
  return currentBrightness;
}

// Hand the strip back to the clock after an effect (the connect animation) has
// been driving the brightness. Deliberately does not show() - the caller
// redraws, so the leftover animation frame is never displayed at full power.
void restoreBrightness() {
  strip.setBrightness(currentBrightness);
}

void initDisplay() {
  strip.begin();
  strip.show();
  reloadBrightnessSettings();
  currentBrightness = brightnessCfg.maxBrightness;
  strip.setBrightness(currentBrightness);
  Serial.printf("Display initialized - LED_COUNT: %d, LED_BUS_PIN: %d\n", LED_COUNT, LED_BUS_PIN);
}

int mapLed(int x, int y){
  int ret = 0;
  if(y%2==0){//even
    ret = x + 11*y;
  }else{
    ret = abs(x-10) + 11*y;
  }
  return ret;
}

int showDigit(int digit, int x, int y){
  const int (*digitArray)[3] = nullptr;
  
  switch (digit){
    case 0: digitArray = digit0; break;
    case 1: digitArray = digit1; break;
    case 2: digitArray = digit2; break;
    case 3: digitArray = digit3; break;
    case 4: digitArray = digit4; break;
    case 5: digitArray = digit5; break;
    case 6: digitArray = digit6; break;
    case 7: digitArray = digit7; break;
    case 8: digitArray = digit8; break;
    case 9: digitArray = digit9; break;
    default: return 0;
  }
  
  for(int i=0; i<5; i++){
    for(int j=0; j<3; j++){
      if(digitArray[i][j] == 1){
        strip.setPixelColor(mapLed(x+j, y+i), strip.Color(customLedColor[0], customLedColor[1], customLedColor[2], customLedColor[3]));
      }
    }
  }
  
  return 1;
}

void showHeart(uint32_t c){
  for(int i=0; i<11; i++) {
    for(int j=0; j<11; j++){
      if(heart[j][i] == 1){
        strip.setPixelColor(mapLed(i,j), c);
      }else{
        strip.setPixelColor(mapLed(i,j), strip.Color(0, 0, 0, 0));
      }
    }
  }
  strip.show();
}

void showWifi(uint32_t c){
  for(int i=0; i<11; i++) {
    for(int j=0; j<11; j++){
      if(wifi[j][i] == 1){
        strip.setPixelColor(mapLed(i,j), c);
      }else{
        strip.setPixelColor(mapLed(i,j), strip.Color(0, 0, 0, 0));
      }
    }
  }
  strip.show();
}

void testLed(){
  strip.show();
  while(true){
    for(int j=0; j<256; j++) {
      showHeart(strip.Color(0, strip.gamma8(j), 0, 0));
      delay(3);
    }

    for(int j=255; j>=0; j--) {
      showHeart(strip.Color(0, strip.gamma8(j), 0, 0));
      delay(3);
    }
  }
}

void setStime(uint hour, uint min)
{
  Serial.printf("setStime called: %02d:%02d, Color RGBW: %d,%d,%d,%d\n",
                hour, min, customLedColor[0], customLedColor[1], customLedColor[2], customLedColor[3]);
  
  if(min >=25){
    hour+=1;
  }
  if(hour >= 12 ){
    hour -= 12;
  }
  strip.fill(strip.Color(0, 0, 0, 0));

  uint32_t color = strip.Color(customLedColor[0], customLedColor[1], customLedColor[2], customLedColor[3]);

  for(int j=0; j<5;j++){
    strip.setPixelColor(time_it_is[j], color);
  }

  uint minTemp = min / 5;
  for(int j=0; j<12;j++){
    strip.setPixelColor(time_minutes[minTemp][j], color);
  }

  if(hour == 1 && minTemp != 0){
    for(int j=0; j<6;j++){
      strip.setPixelColor(time_hours[12][j], color);
    }
  }else{
    for(int j=0; j<6;j++){
      strip.setPixelColor(time_hours[hour][j], color);
    }
  }
  strip.show();
}

void setStimeDigital(uint hour, uint min)
{
  Serial.printf("setStimeDigital called: %02d:%02d, Color RGBW: %d,%d,%d,%d\n",
                hour, min, customLedColor[0], customLedColor[1], customLedColor[2], customLedColor[3]);
  
  for(int i=0; i<strip.numPixels(); i++) {
    strip.setPixelColor(i, strip.Color(0, 0, 0, 0));
  }
  int minDigit1 = min / 10;
  int minDigit2 = min % 10;

  int hourDigit1 = hour / 10;
  int hourDigit2 = hour % 10;

  showDigit(hourDigit1, 2, 0);
  showDigit(hourDigit2, 6, 0);

  showDigit(minDigit1, 2, 6);
  showDigit(minDigit2, 6, 6);
 
  strip.show();
}

void updateBrightness(float lux) {
  uint8_t newBrightness;
  if(lux < brightnessCfg.minLux){
    newBrightness = brightnessCfg.minBrightness;
  }else if(lux > brightnessCfg.maxLux || brightnessCfg.maxLux <= brightnessCfg.minLux){
    newBrightness = brightnessCfg.maxBrightness;
  }else{
    newBrightness = brightnessCfg.minBrightness +
                    (lux - brightnessCfg.minLux) *
                    (brightnessCfg.maxBrightness - brightnessCfg.minBrightness) /
                    (brightnessCfg.maxLux - brightnessCfg.minLux);
  }

  // Only touch the strip when the value actually moved: setBrightness()
  // rescales every pixel and show() blocks with interrupts off for ~3.6 ms at
  // 121 LEDs, which is time the WiFi stack would rather have.
  if (newBrightness == currentBrightness) return;

  currentBrightness = newBrightness;
  strip.setBrightness(currentBrightness);
  strip.show();
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  customLedColor[0] = r;
  customLedColor[1] = g;
  customLedColor[2] = b;
  customLedColor[3] = w;
}

void getLedColor(uint8_t &r, uint8_t &g, uint8_t &b, uint8_t &w) {
  r = customLedColor[0];
  g = customLedColor[1];
  b = customLedColor[2];
  w = customLedColor[3];
}

BrightnessSettings getBrightnessSettings() {
  return brightnessCfg;
}

void setDisplayMode(uint8_t mode) {
  displayMode = mode;
}

uint8_t getDisplayMode() {
  return displayMode;
}

void updateDisplay(uint hour, uint min) {
  if(displayMode == 0) {
    setStime(hour, min);
  } else {
    setStimeDigital(hour, min);
  }
}

void showConnectingAnimation() {
  static int16_t brightness = 0;
  static int8_t direction = 1;
  static uint32_t nextFrameMs = 0;

  // Self-paced so the caller can poll this from a non-blocking loop without
  // the animation speed depending on how often it happens to be called.
  uint32_t now = millis();
  if ((int32_t)(now - nextFrameMs) < 0) return;
  nextFrameMs = now + 30;

  uint32_t orangeColor = strip.Color(255, 165, 0, 0);

  strip.setBrightness((uint8_t)brightness);
  strip.fill(orangeColor);
  strip.show();

  brightness += direction * 5;
  if (brightness >= 150) {
    brightness = 150;
    direction = -1;
  } else if (brightness <= 0) {
    brightness = 0;
    direction = 1;
  }
}
