/* UMK_VIA - WS2812B RGB LEDs, driven by DMA + SPI. */

#include <stdint.h>
#include "ch32fun.h"
#include "rgb_led.h"

#ifdef CUSTOM_RGB_ENABLE


/* The ch32v003fun WS2812B driver clocks the bitstream out of SPI1 MOSI, which
 * on the CH32V003 is hard-wired to PC6. CUSTOM_RGB_PIN exists so a board can
 * state its wiring explicitly (and so the Web UI has something to generate),
 * but any other value would silently produce no output - so reject it. */
#ifndef CUSTOM_RGB_PIN
#define CUSTOM_RGB_PIN PC6
#endif
#if CUSTOM_RGB_PIN != PC6
#error "CUSTOM_RGB_PIN must be PC6: the DMA+SPI WS2812B driver can only \
output on the SPI1 MOSI pin. Change the wiring or the driver."
#endif

#if CUSTOM_RGB_NUM_LEDS > 32
#error "CUSTOM_RGB_NUM_LEDS must be <= 32 (WS2812B DMA driver buffer limit)"
#endif

#define DMALEDS 32
/* WSRBG makes the driver read the callback's return value as 0x00RRGGBB and
 * emit it in the G,R,B order WS2812B actually wants on the wire. */
#define WSRBG
/* Let the USB interrupt preempt the LED DMA interrupt: refilling the bit
 * buffer takes long enough to corrupt a software USB packet otherwise. */
#define WS2812B_ALLOW_INTERRUPT_NESTING
#define WS2812DMA_IMPLEMENTATION
#include "ws2812b_dma_spi_led_driver.h"

static uint32_t rgb_last_frame_ms;
static uint32_t rgb_last_step_ms;
static uint8_t  rgb_hue;
static uint8_t  rgb_reactive_level;

/* Compact HSV -> 0x00RRGGBB. h/s/v are 0..255. */
static uint32_t hsv_to_rgb(uint8_t h, uint8_t s, uint8_t v)
{
	uint8_t region    = h / 43;
	uint8_t remainder = (uint8_t)((h - (region * 43)) * 6);

	uint8_t p = (uint8_t)((v * (255 - s)) >> 8);
	uint8_t q = (uint8_t)((v * (255 - ((s * remainder) >> 8))) >> 8);
	uint8_t t = (uint8_t)((v * (255 - ((s * (255 - remainder)) >> 8))) >> 8);

	uint8_t r, g, b;
	switch (region) {
	case 0:  r = v; g = t; b = p; break;
	case 1:  r = q; g = v; b = p; break;
	case 2:  r = p; g = v; b = t; break;
	case 3:  r = p; g = q; b = v; break;
	case 4:  r = t; g = p; b = v; break;
	default: r = v; g = p; b = q; break;
	}

	return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

/* Called from the DMA interrupt once per LED, per frame. */
uint32_t WS2812BLEDCallback(int ledno)
{
	if (ledno >= CUSTOM_RGB_NUM_LEDS) {
		return 0;
	}

	switch (CUSTOM_RGB_MODE) {
	case RGB_MODE_STATIC:
		return hsv_to_rgb(128, 255, 60);

	case RGB_MODE_BREATHING: {
		/* 2000ms triangle wave over brightness 0..100. */
		uint32_t phase = timer_ms % 2000;
		uint8_t  v     = (phase < 1000) ? (uint8_t)(phase / 10)
		                                : (uint8_t)((2000 - phase) / 10);
		return hsv_to_rgb(128, 255, v);
	}

	case RGB_MODE_REACTIVE:
		return hsv_to_rgb(128, 255, rgb_reactive_level);

	case RGB_MODE_RAINBOW:
	default:
		/* Phase-shift each LED so the strip shows a moving gradient. */
		return hsv_to_rgb((uint8_t)(rgb_hue + ledno * (256 / DMALEDS)), 255, 60);
	}
}

void rgb_led_init(void)
{
	rgb_last_frame_ms  = 0;
	rgb_last_step_ms   = 0;
	rgb_hue            = 0;
	rgb_reactive_level = 0;
	WS2812BDMAInit();
}

void rgb_led_notify_keypress(uint8_t row, uint8_t col)
{
	(void)row;
	(void)col;
	rgb_reactive_level = 150;
}

void rgb_led_task(void)
{
	/* Animation stepping runs off its own timer so the effects keep their
	 * documented rates regardless of the frame rate. */
	uint32_t elapsed = (uint32_t)(timer_ms - rgb_last_step_ms);
	if (elapsed) {
		rgb_last_step_ms = timer_ms;

		if ((timer_ms % 10) == 0) {
			rgb_hue++;
		}
		if ((timer_ms % 5) == 0 && rgb_reactive_level) {
			rgb_reactive_level--;
		}
	}

	if ((uint32_t)(timer_ms - rgb_last_frame_ms) < RGB_FRAME_INTERVAL_MS) {
		return;
	}
	rgb_last_frame_ms = timer_ms;

	if (!WS2812BLEDInUse) {
		WS2812BDMAStart(CUSTOM_RGB_NUM_LEDS);
	}
}

#endif /* CUSTOM_RGB_ENABLE */
