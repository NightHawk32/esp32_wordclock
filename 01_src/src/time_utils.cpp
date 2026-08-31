#include "time_utils.h"
#include <time.h>

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
  setenv("TZ",timezone.c_str(),1);
  tzset();
}

void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst){
  struct tm tm;

  tm.tm_year = yr - 1900;
  tm.tm_mon = month-1;
  tm.tm_mday = mday;
  tm.tm_hour = hr;
  tm.tm_min = minute;
  tm.tm_sec = sec;
  tm.tm_isdst = isDst;
  time_t t = mktime(&tm);
  Serial.printf("Setting time: %s", asctime(&tm));
  struct timeval now = { .tv_sec = t };
  settimeofday(&now, NULL);
}

void initTime(String timezone){
  // configTzTime returns immediately - SNTP keeps running in the background and
  // re-syncs on its own, so we must not block the loop waiting for the first
  // packet here. Several servers give us a fallback if one is unreachable.
  Serial.println("Starting SNTP");
  configTzTime(timezone.c_str(), "pool.ntp.org", "time.nist.gov", "time.cloudflare.com");
  setTimezone(timezone);
}

bool isTimeValid(){
  time_t now = time(nullptr);
  // Anything before 2021 means the RTC is still at its power-on epoch.
  return now > 1609459200;
}
