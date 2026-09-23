/* UMK_VIA - QMK compatible keycode evaluation, layers and Tap/Hold. */

#include <stdint.h>
#include "keymap.h"
#ifdef CUSTOM_RGB_ENABLE
#include "rgb_led.h"
#endif

uint16_t current_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS];

/* Empty fallback. A keyboard keymap.c provides the real thing and wins the
 * link, so a board without a keymap still builds (and does nothing). */
__attribute__((weak))
const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS] = { { { 0 } } };

/* Keycode that was in effect when each key went down. Release is always
 * undone from here, never from a fresh layer lookup - otherwise changing
 * layers while a key is held would strand that key. */
static uint16_t keycode_cache[LOGICAL_ROWS][LOGICAL_COLS];

static uint16_t layer_state;   /* bitmask of MO/TG/TO activated layers */
static uint8_t  default_layer; /* set by DF() */

/* Only one Tap/Hold key can be undecided at a time. A second one forces the
 * first to resolve as a hold. */
static struct {
	uint8_t  active;
	uint8_t  row;
	uint8_t  col;
	uint16_t keycode;
	uint32_t press_time;
} th;

/* A settled tap is reported for TAP_REPORT_MS so the host actually sees it. */
static uint8_t  tap_kc;
static uint8_t  tap_mods;
static uint8_t  tap_active;
static uint32_t tap_until;

/* QMK packs the right hand side into bit 4; HID uses the high nibble. */
static inline uint8_t mods_to_hid(uint8_t qmk_mods)
{
	return (qmk_mods & 0x10) ? (uint8_t)((qmk_mods & 0x0F) << 4)
	                         : (uint8_t)(qmk_mods & 0x0F);
}

static inline uint8_t layer_is_active(uint8_t layer)
{
	return (layer == default_layer) || ((layer_state >> layer) & 1);
}

void keymap_reset_to_default(void)
{
	for (uint8_t l = 0; l < LAYERS; l++) {
		for (uint8_t r = 0; r < LOGICAL_ROWS; r++) {
			for (uint8_t c = 0; c < LOGICAL_COLS; c++) {
				uint16_t kc = default_keymap[l][r][c];
#ifndef CUSTOM_NO_TRNS_DEFAULT
				if (l > 0 && kc == KC_NO) {
					kc = KC_TRNS;
				}
#endif
				current_keymap[l][r][c] = kc;
			}
		}
	}
}

void keymap_init(void)
{
	layer_state   = 0;
	default_layer = 0;
	th.active     = 0;
	tap_active    = 0;

	for (uint8_t r = 0; r < LOGICAL_ROWS; r++) {
		for (uint8_t c = 0; c < LOGICAL_COLS; c++) {
			keycode_cache[r][c] = KC_NO;
		}
	}
}

/* Walks from the highest active layer down, falling through KC_TRNS only.
 * KC_NO stops the walk - that is "deliberately unassigned", per QMK. */
static uint16_t keymap_resolve(uint8_t row, uint8_t col)
{
	for (int8_t l = LAYERS - 1; l >= 0; l--) {
		if (!layer_is_active((uint8_t)l)) {
			continue;
		}
		uint16_t kc = current_keymap[l][row][col];
		if (kc == KC_TRNS) {
			continue;
		}
		return kc;
	}
	return KC_NO;
}

/* Turns the pending Tap/Hold key into its hold form.
 *
 * The cache entry is rewritten to a keycode that already behaves correctly on
 * release: a modifier-only QK_MODS for MT, and a plain MO() for LT. That way
 * keymap_process_release needs no special case for resolved Tap/Hold keys. */
static void th_resolve_hold(void)
{
	if (!th.active) {
		return;
	}
	th.active = 0;

	uint16_t kc = th.keycode;
	if (kc < QK_LAYER_TAP) { /* QK_MOD_TAP */
		uint8_t mods = (uint8_t)((kc >> 8) & 0x1F);
		keycode_cache[th.row][th.col] = (uint16_t)(QK_MODS | (mods << 8));
	} else {                 /* QK_LAYER_TAP */
		uint8_t layer = (uint8_t)((kc >> 8) & 0x0F);
		layer_state |= (uint16_t)(1u << layer);
		keycode_cache[th.row][th.col] = MO(layer);
	}
}

static void th_emit_tap(uint16_t kc)
{
	/* Both MT and LT send their bare tap keycode, no modifiers. */
	tap_mods   = 0;
	tap_kc     = (uint8_t)(kc & 0xFF);
	tap_active = 1;
	tap_until  = timer_ms + TAP_REPORT_MS;
}

void keymap_process_press(uint8_t row, uint8_t col)
{
	/* Any other key going down interrupts an undecided Tap/Hold. Resolve it
	 * first so the new key is looked up against the post-hold layer state. */
	if (th.active && !(th.row == row && th.col == col)) {
		th_resolve_hold();
	}

	uint16_t kc = keymap_resolve(row, col);
	keycode_cache[row][col] = kc;

#ifdef CUSTOM_RGB_ENABLE
	rgb_led_notify_keypress(row, col);
#endif

	if (kc == KC_NO || kc == KC_TRNS) {
		keycode_cache[row][col] = KC_NO;
		return;
	}

	if (kc >= QK_MOD_TAP && kc <= (QK_LAYER_TAP | 0x0FFF)) {
		/* MT / LT - hold off until the tapping term or an interruption. */
		th.active     = 1;
		th.row        = row;
		th.col        = col;
		th.keycode    = kc;
		th.press_time = timer_ms;
		return;
	}

	if (kc >= QK_TO && kc <= (QK_TOGGLE | 0x1F)) {
		uint8_t layer = (uint8_t)(kc & 0x1F);
		if (layer < LAYERS) {
			switch (kc & 0xFFE0) {
			case QK_TO:
				layer_state = (layer == 0) ? 0 : (uint16_t)(1u << layer);
				break;
			case QK_MOMENTARY:
				layer_state |= (uint16_t)(1u << layer);
				break;
			case QK_DEF_LAYER:
				default_layer = layer;
				break;
			case QK_TOGGLE:
				layer_state ^= (uint16_t)(1u << layer);
				break;
			default:
				break;
			}
		}
		return;
	}
}

void keymap_process_release(uint8_t row, uint8_t col)
{
	uint16_t kc = keycode_cache[row][col];
	keycode_cache[row][col] = KC_NO;

	if (th.active && th.row == row && th.col == col) {
		/* Released before the tapping term and never interrupted: a tap. */
		th.active = 0;
		if ((uint32_t)(timer_ms - th.press_time) < TAPPING_TERM_MS) {
			th_emit_tap(kc);
		} else {
			/* Term already elapsed but keymap_task had not run yet; treat
			 * it as a hold that ends immediately - nothing to undo. */
		}
		return;
	}

	/* MO(n), including the rewritten form of a held LT. */
	if ((kc & 0xFFE0) == QK_MOMENTARY) {
		uint8_t layer = (uint8_t)(kc & 0x1F);
		if (layer < LAYERS) {
			layer_state &= (uint16_t)~(1u << layer);
		}
	}
	/* Everything else is stateless - dropping it from the cache is enough. */
}

uint8_t keymap_task(void)
{
	uint8_t changed = 0;

	if (th.active && (uint32_t)(timer_ms - th.press_time) >= TAPPING_TERM_MS) {
		th_resolve_hold();
		changed = 1;
	}

	if (tap_active && (int32_t)(timer_ms - tap_until) >= 0) {
		tap_active = 0;
		changed    = 1;
	}

	return changed;
}

void keymap_generate_report(uint8_t *report)
{
	uint8_t mods = 0;
	uint8_t idx  = 2;

	for (uint8_t i = 0; i < 8; i++) {
		report[i] = 0;
	}

	for (uint8_t r = 0; r < LOGICAL_ROWS; r++) {
		for (uint8_t c = 0; c < LOGICAL_COLS; c++) {
			uint16_t kc = keycode_cache[r][c];
			uint8_t  base;

			if (kc == KC_NO) {
				continue;
			}
			if (kc >= QK_MOD_TAP) {
				/* Undecided Tap/Hold and every layer keycode send nothing. */
				continue;
			}
			if (kc >= QK_MODS) {
				mods |= mods_to_hid((uint8_t)((kc >> 8) & 0x1F));
				base = (uint8_t)(kc & 0xFF);
			} else {
				base = (uint8_t)kc;
			}

			if (base >= KC_LCTL && base <= KC_RGUI) {
				mods |= (uint8_t)(1u << (base - KC_LCTL));
			} else if (base >= 0x04 && idx < 8) {
				report[idx++] = base;
			}
		}
	}

	if (tap_active) {
		mods |= tap_mods;
		if (tap_kc >= KC_LCTL && tap_kc <= KC_RGUI) {
			mods |= (uint8_t)(1u << (tap_kc - KC_LCTL));
		} else if (tap_kc >= 0x04 && idx < 8) {
			report[idx++] = tap_kc;
		}
	}

	report[0] = mods;
}
