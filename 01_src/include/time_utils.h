#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

void printLocalTime();
void setTimezone(String timezone);
void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst);
void initTime(String timezone);

// True once SNTP has delivered a plausible wall-clock time. The RTC keeps
// running without WiFi, so this stays true across a network outage.
bool isTimeValid();

#endif // TIME_UTILS_H
