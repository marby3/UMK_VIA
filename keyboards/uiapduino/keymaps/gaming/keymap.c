/* uiapduino - "gaming" keymap.
 *
 * Exists to demonstrate the umk CLI's -km switch:
 *   umk compile -kb uiapduino -km gaming
 *
 * No Tap/Hold anywhere - every key fires the instant it goes down.
 */

#include "keymap.h"

const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS] = {
	/* Layer 0 - base */
	{
		{ KC_ESC,  KC_1,    KC_2,    KC_3,    KC_4,    KC_5    },
		{ KC_TAB,  KC_Q,    KC_W,    KC_E,    KC_R,    KC_T    },
		{ KC_LSFT, KC_A,    KC_S,    KC_D,    KC_F,    KC_G    },
		{ KC_LCTL, KC_Z,    KC_X,    KC_C,    KC_SPC,  MO(1)   },
	},

	/* Layer 1 */
	{
		{ KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6   },
		{ KC_NO,   KC_NO,   KC_UP,   KC_NO,   KC_NO,   KC_NO   },
		{ KC_NO,   KC_LEFT, KC_DOWN, KC_RGHT, KC_NO,   KC_NO   },
		{ KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_ENT,  KC_TRNS },
	},

	{ { 0 } },
	{ { 0 } },
};
