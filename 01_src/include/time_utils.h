#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <Arduino.h>

void printLocalTime();
void setTimezone(String timezone);
void setTime(int yr, int month, int mday, int hr, int minute, int sec, int isDst);
void initTime(String timezone);

#endif // TIME_UTILS_H
