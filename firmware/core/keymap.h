/* UMK_VIA - QMK compatible keycode evaluation, layers and Tap/Hold.
 *
 * Keycodes are the plain 16 bit QMK values that VIA sends over the wire; no
 * translation table sits in between. See section 4.4 of the spec for the
 * ranges that are actually honoured.
 */
#ifndef _UMK_KEYMAP_H
#define _UMK_KEYMAP_H

#include <stdint.h>
#include "board_config.h"

/* ------------------------------------------------------------------------ */
/* Keycode ranges (QMK)                                                      */
/* ------------------------------------------------------------------------ */

#define KC_NO   0x0000
#define KC_TRNS 0x0001

#define QK_BASIC_MAX  0x00FF
#define QK_MODS       0x0100 /* .. 0x1FFF  LSFT(KC_A) etc.       */
#define QK_MOD_TAP    0x2000 /* .. 0x3FFF  MT(mods, kc)          */
#define QK_LAYER_TAP  0x4000 /* .. 0x4FFF  LT(layer, kc)         */
#define QK_TO         0x5200 /* .. 0x521F                        */
#define QK_MOMENTARY  0x5220 /* .. 0x523F                        */
#define QK_DEF_LAYER  0x5240 /* .. 0x525F                        */
#define QK_TOGGLE     0x5260 /* .. 0x527F                        */

#define MT(mods, kc)  ((uint16_t)(QK_MOD_TAP   | (((mods) & 0x1F) << 8) | ((kc) & 0xFF)))
#define LT(layer, kc) ((uint16_t)(QK_LAYER_TAP | (((layer) & 0x0F) << 8) | ((kc) & 0xFF)))
#define TO(layer)     ((uint16_t)(QK_TO        | ((layer) & 0x1F)))
#define MO(layer)     ((uint16_t)(QK_MOMENTARY | ((layer) & 0x1F)))
#define DF(layer)     ((uint16_t)(QK_DEF_LAYER | ((layer) & 0x1F)))
#define TG(layer)     ((uint16_t)(QK_TOGGLE    | ((layer) & 0x1F)))

/* QMK modifier bits, as used by the QK_MODS / QK_MOD_TAP payload.
 * Bit 4 flips the whole set to the right hand side. */
#define MOD_LCTL 0x01
#define MOD_LSFT 0x02
#define MOD_LALT 0x04
#define MOD_LGUI 0x08
#define MOD_RCTL 0x11
#define MOD_RSFT 0x12
#define MOD_RALT 0x14
#define MOD_RGUI 0x18

#define LCTL(kc) ((uint16_t)(QK_MODS | (MOD_LCTL << 8) | ((kc) & 0xFF)))
#define LSFT(kc) ((uint16_t)(QK_MODS | (MOD_LSFT << 8) | ((kc) & 0xFF)))
#define LALT(kc) ((uint16_t)(QK_MODS | (MOD_LALT << 8) | ((kc) & 0xFF)))
#define LGUI(kc) ((uint16_t)(QK_MODS | (MOD_LGUI << 8) | ((kc) & 0xFF)))
#define RCTL(kc) ((uint16_t)(QK_MODS | (MOD_RCTL << 8) | ((kc) & 0xFF)))
#define RSFT(kc) ((uint16_t)(QK_MODS | (MOD_RSFT << 8) | ((kc) & 0xFF)))
#define RALT(kc) ((uint16_t)(QK_MODS | (MOD_RALT << 8) | ((kc) & 0xFF)))
#define RGUI(kc) ((uint16_t)(QK_MODS | (MOD_RGUI << 8) | ((kc) & 0xFF)))

/* ------------------------------------------------------------------------ */
/* Basic keycodes - identical to USB HID Keyboard Usage IDs                  */
/* ------------------------------------------------------------------------ */

#define KC_A 0x04
#define KC_B 0x05
#define KC_C 0x06
#define KC_D 0x07
#define KC_E 0x08
#define KC_F 0x09
#define KC_G 0x0A
#define KC_H 0x0B
#define KC_I 0x0C
#define KC_J 0x0D
#define KC_K 0x0E
#define KC_L 0x0F
#define KC_M 0x10
#define KC_N 0x11
#define KC_O 0x12
#define KC_P 0x13
#define KC_Q 0x14
#define KC_R 0x15
#define KC_S 0x16
#define KC_T 0x17
#define KC_U 0x18
#define KC_V 0x19
#define KC_W 0x1A
#define KC_X 0x1B
#define KC_Y 0x1C
#define KC_Z 0x1D
#define KC_1 0x1E
#define KC_2 0x1F
#define KC_3 0x20
#define KC_4 0x21
#define KC_5 0x22
#define KC_6 0x23
#define KC_7 0x24
#define KC_8 0x25
#define KC_9 0x26
#define KC_0 0x27
#define KC_ENT  0x28
#define KC_ESC  0x29
#define KC_BSPC 0x2A
#define KC_TAB  0x2B
#define KC_SPC  0x2C
#define KC_MINS 0x2D
#define KC_EQL  0x2E
#define KC_LBRC 0x2F
#define KC_RBRC 0x30
#define KC_BSLS 0x31
#define KC_NUHS 0x32
#define KC_SCLN 0x33
#define KC_QUOT 0x34
#define KC_GRV  0x35
#define KC_COMM 0x36
#define KC_DOT  0x37
#define KC_SLSH 0x38
#define KC_CAPS 0x39
#define KC_F1  0x3A
#define KC_F2  0x3B
#define KC_F3  0x3C
#define KC_F4  0x3D
#define KC_F5  0x3E
#define KC_F6  0x3F
#define KC_F7  0x40
#define KC_F8  0x41
#define KC_F9  0x42
#define KC_F10 0x43
#define KC_F11 0x44
#define KC_F12 0x45
#define KC_PSCR 0x46
#define KC_SCRL 0x47
#define KC_PAUS 0x48
#define KC_INS  0x49
#define KC_HOME 0x4A
#define KC_PGUP 0x4B
#define KC_DEL  0x4C
#define KC_END  0x4D
#define KC_PGDN 0x4E
#define KC_RGHT 0x4F
#define KC_LEFT 0x50
#define KC_DOWN 0x51
#define KC_UP   0x52
#define KC_NUM  0x53
#define KC_PSLS 0x54
#define KC_PAST 0x55
#define KC_PMNS 0x56
#define KC_PPLS 0x57
#define KC_PENT 0x58
#define KC_P1 0x59
#define KC_P2 0x5A
#define KC_P3 0x5B
#define KC_P4 0x5C
#define KC_P5 0x5D
#define KC_P6 0x5E
#define KC_P7 0x5F
#define KC_P8 0x60
#define KC_P9 0x61
#define KC_P0 0x62
#define KC_PDOT 0x63
#define KC_NUBS 0x64
#define KC_APP  0x65
#define KC_INT1 0x87 /* JIS backslash / ro    */
#define KC_INT2 0x88 /* JIS katakana/hiragana */
#define KC_INT3 0x89 /* JIS yen               */
#define KC_INT4 0x8A /* JIS henkan            */
#define KC_INT5 0x8B /* JIS muhenkan          */
#define KC_LCTL 0xE0
#define KC_LSFT 0xE1
#define KC_LALT 0xE2
#define KC_LGUI 0xE3
#define KC_RCTL 0xE4
#define KC_RSFT 0xE5
#define KC_RALT 0xE6
#define KC_RGUI 0xE7

/* ------------------------------------------------------------------------ */
/* State                                                                     */
/* ------------------------------------------------------------------------ */

/* The live, VIA-writable keymap. */
extern uint16_t current_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS];

/* Compile time keymap. Core ships an empty weak definition;
 * keyboards/<name>/keymaps/<keymap>/keymap.c overrides it. */
extern const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS];

void keymap_init(void);

/* Copies default_keymap into current_keymap. Unassigned cells on layer >= 1
 * become KC_TRNS unless CUSTOM_NO_TRNS_DEFAULT is set - without this every
 * higher layer would be completely dead. */
void keymap_reset_to_default(void);

void keymap_process_press(uint8_t row, uint8_t col);
void keymap_process_release(uint8_t row, uint8_t col);

/* Time dependent work: Tap/Hold resolution and tap pulse expiry.
 * Returns non-zero when the HID report needs rebuilding. */
uint8_t keymap_task(void);

/* Builds an 8 byte HID Boot Keyboard report into `report`. */
void keymap_generate_report(uint8_t *report);

#endif
