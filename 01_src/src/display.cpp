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

void initDisplay() {
  strip.begin();
  strip.show();
  BrightnessSettings brightnessSettings = loadBrightnessSettings();
  strip.setBrightness(brightnessSettings.maxBrightness);
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
  BrightnessSettings brightnessSettings = loadBrightnessSettings();
  
  uint8_t newBrightness;
  if(lux < brightnessSettings.minLux){
    newBrightness = brightnessSettings.minBrightness;
  }else if(lux > brightnessSettings.maxLux){
    newBrightness = brightnessSettings.maxBrightness;
  }else{
    newBrightness = brightnessSettings.minBrightness +
                    (lux - brightnessSettings.minLux) *
                    (brightnessSettings.maxBrightness - brightnessSettings.minBrightness) /
                    (brightnessSettings.maxLux - brightnessSettings.minLux);
  }
  strip.setBrightness(newBrightness);
  Serial.printf("Brightness updated: lux=%.2f, brightness=%d (min=%d, max=%d, minLux=%d, maxLux=%d)\n",
                lux, newBrightness, brightnessSettings.minBrightness, brightnessSettings.maxBrightness,
                brightnessSettings.minLux, brightnessSettings.maxLux);
  strip.show();
}

void setLedColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  customLedColor[0] = r;
  customLedColor[1] = g;
  customLedColor[2] = b;
  customLedColor[3] = w;
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
  static uint8_t brightness = 0;
  static int8_t direction = 1;
  
  // Orange color (R=255, G=165, B=0)
  uint32_t orangeColor = strip.Color(255, 165, 0, 0);
  
  // Fill all LEDs with orange at current brightness
  strip.setBrightness(brightness);
  strip.fill(orangeColor);
  strip.show();
  
  // Update brightness for pulsing effect
  brightness += direction * 5;
  
  // Reverse direction at limits (0 to 150)
  if (brightness >= 150) {
    brightness = 150;
    direction = -1;
  } else if (brightness <= 0) {
    brightness = 0;
    direction = 1;
  }
}
