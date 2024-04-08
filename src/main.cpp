#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <Adafruit_NeoPixel.h>

#include "board.h"
#include "myWifi.h"

bool ledState = LED_OFF;
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 0;

#define LED_PIN     4

// How many NeoPixels are attached to the Arduino?
#define LED_COUNT  121

// NeoPixel brightness, 0 (min) to 255 (max)
#define BRIGHTNESS 255

// Declare our NeoPixel strip object:
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRBW + NEO_KHZ800);

int time_it_is[5] = {0, 1, 3, 4, 5}; // es ist

int time_minutes[12][12] = {
  { 99, 100, 101,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}, // uhr
  {  7,   8,   9,  10,  38,  39,  40,  41,  -1,  -1,  -1,  -1}, // fünf nach
  { 18,  19,  20,  21,  38,  39,  40,  41,  -1,  -1,  -1,  -1}, // zehn nach
  { 26,  27,  28,  29,  30,  31,  32,  38,  39,  40,  41,  -1}, // viertel nach
  { 11,  12,  13,  14,  15,  16,  17,  38,  39,  40,  41,  -1}, // zwanzig nach
  {  7,   8,   9,  10,  35,  36,  37,  44,  45,  46,  47,  -1}, // fünf vor halb
  { 44,  45,  46,  47,  -1,  -1,  -1,  -1,  -1,  -1,  -1,  -1}, // halb 
  {  7,   8,   9,  10,  38,  39,  40,  41,  44,  45,  46,  47}, // fünf nach halb
  { 11,  12,  13,  14,  15,  16,  17,  35,  36,  37,  -1,  -1}, // zwanzig vor
  { 26,  27,  28,  29,  30,  31,  32,  35,  36,  37,  -1,  -1}, // viertel vor
  { 18,  19,  20,  21,  35,  36,  37,  -1,  -1,  -1,  -1,  -1}, // zehn vor
  {  7,   8,   9,  10,  35,  36,  37,  -1,  -1,  -1,  -1,  -1}  // fünf vor
};

int time_hours[13][6] = {
  { 49,  50,  51,  52,  53,  -1}, // zwölf 
  { 60,  61,  62,  63,  -1,  -1}, // eins
  { 62,  63,  64,  65,  -1,  -1}, // zwei
  { 67,  68,  69,  70,  -1,  -1}, // drei
  { 77,  78,  79,  80,  -1,  -1}, // vier
  { 73,  74,  75,  76,  -1,  -1}, // fünf 
  {104, 105, 106, 107, 108,  -1}, // sechs
  { 55,  56,  57,  58,  59,  60}, // sieben 
  { 89,  90,  91,  92,  -1,  -1}, // acht
  { 81,  82,  83,  84,  -1,  -1}, // neun 
  { 93,  94,  95,  96,  -1,  -1}, // zehn
  { 85,  86,  87,  -1,  -1,  -1}, // elf
  { -1,  61,  62,  63,  -1,  -1}, // ein uhr
};

void setStime(uint hour, uint min)
{
  if(min >=30){
    hour+=1;
  }
  if(hour >= 12 ){
    hour -= 12;
  }  
  strip.fill(strip.Color(0, 0, 0, 0)); 

  for(int j=0; j<5;j++){
    strip.setPixelColor(time_it_is[j], strip.Color(255, 0, 0, 255));
  }

  uint minTemp = min / 5;
  for(int j=0; j<12;j++){
    strip.setPixelColor(time_minutes[minTemp][j], strip.Color(255, 0, 0, 255));
  }

  if(hour == 1 && minTemp == 0){
    for(int j=0; j<6;j++){
      strip.setPixelColor(time_hours[12][j], strip.Color(255, 0, 0, 255));
    }    
  }else{
    for(int j=0; j<6;j++){
      strip.setPixelColor(time_hours[hour][j], strip.Color(255, 0, 0, 255));
    }
  }  
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

void setup() {
	pinMode(PIN_LED, OUTPUT);
	digitalWrite(PIN_LED, LED_OFF);


	Serial.println("Starting ....");

	Serial.println("Setup done ....");

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)
  strip.show();            // Turn OFF all pixels ASAP
  strip.setBrightness(BRIGHTNESS);

  configTime(gmtOffset_sec, daylightOffset_sec, NTP_SERVER);
  scanNetworks();
  setup_wifi();

  initTime("CET-1CEST,M3.5.0,M10.5.0/3");

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

  if(millis() > nextUpdate){
    
    if(!getLocalTime(&timeinfo)){
      Serial.println("Failed to obtain time");
      return;
    }
    Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    nextUpdate = millis() + (60-timeinfo.tm_sec)*1000;
    setStime(timeinfo.tm_hour, timeinfo.tm_min);
    //xSemaphoreGive(semDisplay);
  }

}