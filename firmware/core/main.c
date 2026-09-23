/* UMK_VIA - main loop and USB interrupt dispatch.
 *
 * Startup order and the 1ms loop follow spec section 4.1.
 */

#include <stdint.h>
#include "ch32fun.h"
#include "rv003usb.h"
#include "usb_config.h"
#include "matrix.h"
#include "keymap.h"
#include "via.h"
#include "flash_store.h"
#ifdef CUSTOM_SPLIT_ENABLE
#include "split.h"
#endif
#ifdef CUSTOM_RGB_ENABLE
#include "rgb_led.h"
#endif

volatile uint32_t timer_ms;

/* Previous global_matrix_state, for edge detection. */
static matrix_row_t last_matrix_state[LOGICAL_ROWS];

#ifdef CUSTOM_SPLIT_ENABLE
/* How long to wait for the host before deciding we are the far half. */
#define SPLIT_ROLE_TIMEOUT_MS 500
static uint8_t split_is_slave;
#endif

/* Returns non-zero when at least one key changed state. */
static uint8_t report_matrix_changes(void)
{
	uint8_t changed = 0;

	for (uint8_t r = 0; r < LOGICAL_ROWS; r++) {
		matrix_row_t diff = global_matrix_state[r] ^ last_matrix_state[r];
		if (!diff) {
			continue;
		}
		changed = 1;
		for (uint8_t c = 0; c < LOGICAL_COLS; c++) {
			matrix_row_t bit = ((matrix_row_t)1) << c;
			if (!(diff & bit)) {
				continue;
			}
			if (global_matrix_state[r] & bit) {
				keymap_process_press(r, c);
			} else {
				keymap_process_release(r, c);
			}
		}
		last_matrix_state[r] = global_matrix_state[r];
	}

	return changed;
}

int main(void)
{
	SystemInit();
	usb_force_reenumerate(); /* must happen before the host looks at us */

	flash_store_load();
	matrix_init();
	keymap_init();
	via_init();
	usb_setup();

#ifdef CUSTOM_RGB_ENABLE
	rgb_led_init();
#endif

#ifdef CUSTOM_SPLIT_ENABLE
	split_init();
	/* Whichever half the USB cable is plugged into enumerates; the other one
	 * times out and becomes the Slave. */
	for (uint16_t i = 0; i < SPLIT_ROLE_TIMEOUT_MS && !usb_configured_flag; i++) {
		Delay_Ms(1);
	}
	split_is_slave = usb_configured_flag ? 0 : 1;
	split_set_master(usb_configured_flag ? 1 : 0);
#endif

	uint32_t last_tick = SysTick->CNT;

	for (;;) {
		uint32_t now = SysTick->CNT;
		if ((uint32_t)(now - last_tick) < DELAY_MS_TIME) {
			continue;
		}
		last_tick += DELAY_MS_TIME;
		/* A flash write stalls the core for milliseconds. Resync instead of
		 * grinding through a burst of catch-up ticks. */
		if ((uint32_t)(now - last_tick) > 50 * DELAY_MS_TIME) {
			last_tick = now;
		}
		timer_ms++;

#ifdef CUSTOM_SPLIT_ENABLE
		if (split_is_slave) {
			/* The far half only scans and forwards - no VIA, no flash, no
			 * RGB, so both halves cannot fight over the keymap. */
			matrix_scan();
			split_slave_task();
			continue;
		}
#endif

		via_task();
		flash_store_task();
#ifdef CUSTOM_RGB_ENABLE
		rgb_led_task();
#endif

		matrix_scan();

#ifdef CUSTOM_SPLIT_ENABLE
		split_master_task();
#else
		for (uint8_t r = 0; r < LOGICAL_ROWS; r++) {
			global_matrix_state[r] = local_matrix_state[r];
		}
#endif

		uint8_t changed = report_matrix_changes();

		/* Tap/Hold settling changes the report without any matrix edge. */
		changed |= keymap_task();

		if (changed) {
			keymap_generate_report(hid_keyboard_report);
		}
	}
}
