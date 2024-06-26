#ifndef BOARD_H_
#define BOARD_H_

#include <Tsl2561Util.h>

#define ICNT_MAX_TOUCH 2
#define ICNT_ADDR 0x48

#define PIN_LED 8
#define LED_OFF true
#define LED_ON false

#define PIN_BOOT 9

#define PIN_EPD_BUSY D3
#define PIN_EPD_RST D2
#define PIN_EPD_DC D1
#define PIN_EPD_CS D0
#define PIN_EPD_CLK 1
#define PIN_EPD_MOSI D10

#define PIN_SDA 39
#define PIN_SCL 40
#define PIN_TOUCH_INT D6
#define PIN_TOUCH_RESET D7

#define TSL2561_ADDR Tsl2561::ADDR_FLOAT

#endif