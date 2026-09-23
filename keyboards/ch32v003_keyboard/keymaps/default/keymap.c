/* ch32v003_keyboard - default keymap.
 *
 * Plain number keys, so a hardware test shows at a glance which switch
 * fired: 1 2 3 on the top row (SW1-SW3), 4 5 6 on the bottom (SW4-SW6).
 * Remap takes over from here; `Reset Keymap` comes back to this.
 *
 * Layers 1-3 are left empty and become KC_TRNS at init.
 */

#include "keymap.h"

const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS] = {
	/* Layer 0 - base */
	{
		{ KC_1, KC_2, KC_3 },
		{ KC_4, KC_5, KC_6 },
	},
};
