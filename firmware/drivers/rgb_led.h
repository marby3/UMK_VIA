/* UMK_VIA - WS2812B RGB LEDs, driven by DMA + SPI. */
#ifndef _UMK_RGB_LED_H
#define _UMK_RGB_LED_H

#include <stdint.h>
#include "board_config.h"

#ifndef CUSTOM_RGB_NUM_LEDS
#define CUSTOM_RGB_NUM_LEDS 1
#endif

#ifndef CUSTOM_RGB_MODE
#define CUSTOM_RGB_MODE 0
#endif

#define RGB_MODE_RAINBOW   0
#define RGB_MODE_STATIC    1
#define RGB_MODE_BREATHING 2
#define RGB_MODE_REACTIVE  3

/* ~33Hz. */
#define RGB_FRAME_INTERVAL_MS 30

void rgb_led_init(void);

/* Main loop hook: kicks a DMA frame every RGB_FRAME_INTERVAL_MS. */
void rgb_led_task(void);

/* Reactive mode trigger. row/col are accepted for future per-key effects but
 * the current effects are position independent. */
void rgb_led_notify_keypress(uint8_t row, uint8_t col);

#endif
