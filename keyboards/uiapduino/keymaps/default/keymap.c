/* uiapduino - default keymap.
 *
 * This is only the power-on fallback: once VIA/Remap writes a keymap it is
 * stored in flash and used instead. `Reset Keymap` in Remap comes back here.
 *
 * Unassigned cells on layers >= 1 are turned into KC_TRNS at init, so the
 * layers below still show through.
 */

#include "keymap.h"

const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS] = {
	/* Layer 0 - base */
	{
		{ KC_1,    KC_2,    KC_3,    KC_4,    KC_5,    KC_6    },
		{ KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,    KC_Y    },
		{ KC_A,    KC_S,    KC_D,    KC_F,    KC_G,    KC_H    },
		{ MO(1),   KC_Z,    KC_X,    KC_C,    KC_V,    LT(2, KC_SPC) },
	},

	/* Layer 1 - symbols and navigation */
	{
		{ KC_F1,   KC_F2,   KC_F3,   KC_F4,   KC_F5,   KC_F6   },
		{ KC_ESC,  KC_TAB,  KC_UP,   KC_BSPC, KC_DEL,  KC_HOME },
		{ KC_LSFT, KC_LEFT, KC_DOWN, KC_RGHT, KC_ENT,  KC_END  },
		{ KC_TRNS, KC_MINS, KC_EQL,  KC_LBRC, KC_RBRC, KC_TRNS },
	},

	/* Layer 2 - held via LT on the thumb key */
	{
		{ KC_7,    KC_8,    KC_9,    KC_0,    KC_NO,   KC_NO   },
		{ KC_4,    KC_5,    KC_6,    KC_PPLS, KC_PMNS, KC_NO   },
		{ KC_1,    KC_2,    KC_3,    KC_PAST, KC_PSLS, KC_NO   },
		{ KC_NO,   KC_0,    KC_DOT,  KC_NO,   TG(3),   KC_TRNS },
	},

	/* Layer 3 - toggled, modifier practice layer */
	{
		{ MT(MOD_LCTL, KC_A), MT(MOD_LSFT, KC_S), MT(MOD_LALT, KC_D),
		  MT(MOD_LGUI, KC_F), LCTL(KC_C),         LCTL(KC_V) },
		{ KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO   },
		{ KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO   },
		{ KC_NO,   KC_NO,   KC_NO,   KC_NO,   TO(0),   KC_NO   },
	},
};
