#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>
#include <Wire.h>
#include <Tsl2561Util.h>

#include "board.h"
#include "myWifi.h"

#include "time_led.h"
#include "digits.h"
#include "icons.h"

bool ledState = LED_OFF;
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  121

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 255
#define LED_COLOR 0, 0, 0, 255 //R G B W
#define MIN_BRIGHTNESS 5
#define MAX_BRIGHTNESS 255

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_BUS_PIN, NEO_GRBW + NEO_KHZ800);
Tsl2561 Tsl(Wire);

//https://www.pixilart.com/draw/96x64-bitmap-7012c32cc9


// create an array digits which contains pointers to the arrays digit0 and digit1


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
  switch (digit){
    case 0:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit0[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 1:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit1[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 2:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit2[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 3:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit3[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 4:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit4[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 5:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit5[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 6:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit6[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 7:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit7[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 8:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit8[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
    case 9:
      for(int i=0; i<5; i++){
        for(int j=0; j<3; j++){
          if(digit9[i][j] == 1){
            strip.setPixelColor(mapLed(x+j, y+i), strip.Color(LED_COLOR));
          }
        }
      }
      break;
  }
  
  return 1;
}

int i = 0;

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

void testLed(){
    strip.show();
  while(true){
    for(int j=0; j<256; j++) { // Ramp up from 0 to 255
    // Fill entire strip with white at gamma-corrected brightness level 'j':
    showHeart(strip.Color(0, strip.gamma8(j), 0, 0));
    delay(3);
  }

  for(int j=255; j>=0; j--) { // Ramp down from 255 to 0
    showHeart(strip.Color(0, strip.gamma8(j), 0, 0));
    delay(3);
  }
    
    /*i++;
    if(i>9) i=0;
    for(int i=0; i<11; i++) {
      for(int j=0; j<11; j++){
        strip.setPixelColor(mapLed(i,j), strip.Color(0, 0, 0, 255));
        strip.show();
        delay(10);
      }
    }
    showDigit(i, 2, 0);
    showDigit(i, 6, 0);

    showDigit(i, 2, 6);
    showDigit(i, 6, 6);
    strip.show();
    delay(1000);
    for(int i=0; i<strip.numPixels(); i++) {
      strip.setPixelColor(i, strip.Color(0, 0, 0, 0));
      strip.show();
    }*/
  }
}

void setStime(uint hour, uint min)
{
  if(min >=25){
    hour+=1;
  }
  if(hour >= 12 ){
    hour -= 12;
  }  
  strip.fill(strip.Color(0, 0, 0, 0)); 

  for(int j=0; j<5;j++){
    strip.setPixelColor(time_it_is[j], strip.Color(LED_COLOR));
  }

  uint minTemp = min / 5;
  for(int j=0; j<12;j++){
    strip.setPixelColor(time_minutes[minTemp][j], strip.Color(LED_COLOR));
  }

  if(hour == 1 && minTemp == 0){
    for(int j=0; j<6;j++){
      strip.setPixelColor(time_hours[12][j], strip.Color(LED_COLOR));
    }    
  }else{
    for(int j=0; j<6;j++){
      strip.setPixelColor(time_hours[hour][j], strip.Color(LED_COLOR));
    }
  }  
  strip.show();
}

void setStimeDigital(uint hour, uint min)
{
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

void printLocalTime(){
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setTimezone(String timezone){
  Serial.printf("  Setting Timezone to %s\n",timezone.c_str());
  setenv("TZ",timezone.c_str(),1);  //  Now adjust the TZ.  Clock settings are adjusted to show the new local time
  tzset();
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;   // Set date
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;      // Set time
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;  // 1 or 0
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void initTime(String timezone){
  struct tm timeinfo;

  Serial.println("Setting up time");
  configTime(0, 0, "pool.ntp.org");    // First connect to NTP server, with 0 TZ offset
  if(!getLocalTime(&timeinfo)){
    Serial.println("  Failed to obtain time");
    return;
  }
  Serial.println("  Got the time from NTP");
  // Now we can set the real timezone
  setTimezone(timezone);
}

void scanNetworks() {
  Serial.println("Scan start");

  // WiFi.scanNetworks will return the number of networks found.
  int n = WiFi.scanNetworks();
  Serial.println("Scan done");
  if (n == 0) {
      Serial.println("no networks found");
  } else {
      Serial.print(n);
      Serial.println(" networks found");
      Serial.println("Nr | SSID                             | RSSI | CH | Encryption");
      for (int i = 0; i < n; ++i) {
          // Print SSID and RSSI for each network found
          Serial.printf("%2d",i + 1);
          Serial.print(" | ");
          Serial.printf("%-32.32s", WiFi.SSID(i).c_str());
          Serial.print(" | ");
          Serial.printf("%4ld", WiFi.RSSI(i));
          Serial.print(" | ");
          Serial.printf("%2ld", WiFi.channel(i));
          Serial.print(" | ");
          switch (WiFi.encryptionType(i))
          {
          case WIFI_AUTH_OPEN:
              Serial.print("open");
              break;
          case WIFI_AUTH_WEP:
              Serial.print("WEP");
              break;
          case WIFI_AUTH_WPA_PSK:
              Serial.print("WPA");
              break;
          case WIFI_AUTH_WPA2_PSK:
              Serial.print("WPA2");
              break;
          case WIFI_AUTH_WPA_WPA2_PSK:
              Serial.print("WPA+WPA2");
              break;
          case WIFI_AUTH_WPA2_ENTERPRISE:
              Serial.print("WPA2-EAP");
              break;
          case WIFI_AUTH_WPA3_PSK:
              Serial.print("WPA3");
              break;
          case WIFI_AUTH_WPA2_WPA3_PSK:
              Serial.print("WPA2+WPA3");
              break;
          case WIFI_AUTH_WAPI_PSK:
              Serial.print("WAPI");
              break;
          default:
              Serial.print("unknown");
          }
          Serial.println();
          delay(10);
      }
  }
  Serial.println("");

  // Delete the scan result to free memory for code below.
  WiFi.scanDelete();
}

void setup_wifi() {
  delay(10);
  WiFi.begin(MY_WIFI_SSID, MY_WIFI_PASSWORD);
  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (++counter > 100) ESP.restart();
    Serial.print(".");
  }
  Serial.print("WiFi connected: ");
  Serial.println(MY_WIFI_SSID);
}

struct tm timeinfo;

char *format( const char *fmt, ... ) {
  static char buf[128];
  va_list arg;
  va_start(arg, fmt);
  vsnprintf(buf, sizeof(buf), fmt, arg);
  buf[sizeof(buf)-1] = '\0';
  va_end(arg);
  return buf;
}

uint32_t printTSL(){
  bool found = false;
  uint32_t milliLux = 0;

  if( Tsl.begin(TSL2561_ADDR) ) {
    found = true;
    Serial.println();

    uint16_t scaledFull = 0, scaledIr;
    uint32_t full, ir;
    uint8_t id;
    bool gain = 1;
    Tsl2561::exposure_t exposure = (Tsl2561::exposure_t)2;
    Tsl.on();

    Tsl.setSensitivity(gain, exposure);
    Tsl2561Util::waitNext(exposure);
    Tsl.id(id);
    Tsl.getSensitivity(gain, exposure);
    Tsl.fullLuminosity(scaledFull);
    Tsl.irLuminosity(scaledIr);

    Serial.print(format("Tsl2561 addr: 0x%02x, id: 0x%02x, sfull: %5u, sir: %5u, gain: %d, exp: %d",
      TSL2561_ADDR, id, scaledFull, scaledIr, gain, exposure));

    if( Tsl2561Util::normalizedLuminosity(gain, exposure, full = scaledFull, ir = scaledIr) ) {
      if( Tsl2561Util::milliLux(full, ir, milliLux, Tsl2561::packageCS(id)) ) {
        Serial.print(format(", full: %5lu, ir: %5lu, lux: %5lu.%03lu\n", (unsigned long)full, (unsigned long)ir, (unsigned long)milliLux/1000, (unsigned long)milliLux%1000));
      }
    }

    Tsl.off();
  }
  return milliLux;
}

void setup() {
  Serial.begin(115200);
	Serial.println("Starting ....");

	pinMode(PIN_LED, OUTPUT);
	digitalWrite(PIN_LED, LED_OFF);

  Wire.begin(PIN_SDA, PIN_SCL);
  

	Serial.println("Setup done ....");

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(MAX_BRIGHTNESS);

  //testLed();

  configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);
  scanNetworks();
  setup_wifi();

  initTime("CET-1CEST,M3.5.0/02,M10.5.0/3");
  printTSL();

}

long nextUpdate = 0;

void loop()
{	
  if(WiFi.status() != WL_CONNECTED){
    setup_wifi();
    digitalWrite(PIN_LED, LED_OFF);
  }else{
    digitalWrite(PIN_LED, LED_ON);
  }

  long currentTimestamp = millis();

  if(currentTimestamp > nextUpdate){
    
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    nextUpdate = millis() + (60-timeinfo.tm_sec)*1000;

    /*if(timeinfo.tm_min % 5 == 0){
      setStime(timeinfo.tm_hour, timeinfo.tm_min);
    }else{
      setStimeDigital(timeinfo.tm_hour, timeinfo.tm_min);
    }*/
    setStime(timeinfo.tm_hour, timeinfo.tm_min);
  }

  if(currentTimestamp > 500){
    uint32_t milliLux = printTSL();
    float lux = milliLux / 1000.0f;
    Serial.printf("Lux: %.2f\n", lux);
    if(lux < 10.0f){
      strip.setBrightness(MIN_BRIGHTNESS);
    }else if(lux > 300.0f){
      strip.setBrightness(MAX_BRIGHTNESS);
    }else{
      strip.setBrightness(MIN_BRIGHTNESS + (lux - 10.0f) * (MAX_BRIGHTNESS - MIN_BRIGHTNESS) / (300.0f - 10.0f));
    }
    strip.show();
  }

}