#ifndef RGB_LED_H
#define RGB_LED_H

#include <stdint.h>

#if __has_include("config.h")
#include "config.h"
#endif

void rgb_led_init(void);
void rgb_led_task(uint32_t system_millis);
void rgb_led_notify_keypress(uint8_t row, uint8_t col);

#endif
