/* UMK_VIA - VIA protocol implementation. */

#include <stdint.h>
#include "ch32fun.h"
#include "rv003usb.h"
#include "usb_config.h"
#include "via.h"
#include "keymap.h"
#include "matrix.h"
#include "flash_store.h"

/* VIA command IDs */
#define ID_GET_PROTOCOL_VERSION            0x01
#define ID_GET_KEYBOARD_VALUE              0x02
#define ID_SET_KEYBOARD_VALUE              0x03
#define ID_DYNAMIC_KEYMAP_GET_KEYCODE      0x04
#define ID_DYNAMIC_KEYMAP_SET_KEYCODE      0x05
#define ID_DYNAMIC_KEYMAP_RESET            0x06
#define ID_EEPROM_RESET                    0x0A
#define ID_DYNAMIC_KEYMAP_MACRO_GET_COUNT  0x0C
#define ID_DYNAMIC_KEYMAP_MACRO_GET_BUFSZ  0x0D
#define ID_DYNAMIC_KEYMAP_MACRO_GET_BUFFER 0x0E
#define ID_DYNAMIC_KEYMAP_MACRO_SET_BUFFER 0x0F
#define ID_DYNAMIC_KEYMAP_MACRO_RESET      0x10
#define ID_DYNAMIC_KEYMAP_GET_LAYER_COUNT  0x11
#define ID_DYNAMIC_KEYMAP_GET_BUFFER       0x12
#define ID_DYNAMIC_KEYMAP_SET_BUFFER       0x13
#define ID_UNHANDLED                       0xFF

/* get/set_keyboard_value sub IDs */
#define ID_UPTIME              0x01
#define ID_LAYOUT_OPTIONS      0x02
#define ID_SWITCH_MATRIX_STATE 0x03
#define ID_FIRMWARE_VERSION    0x04
#define ID_DEVICE_INDICATION   0x05
/* UMK_VIA extension: force an immediate flash save. */
#define ID_UMK_SAVE_NOW        0xFF

/* One buffer serves both directions: VIA edits the command in place and
 * echoes it back, so there is nothing to copy. */
static uint8_t via_buf[VIA_PACKET_SIZE] __attribute__((aligned(4)));

static volatile uint8_t via_rx_pos;
static volatile uint8_t via_cmd_ready;

/* Reply state machine. ARMED is set by the main loop; the interrupt latches
 * the endpoint ACK counter itself, which avoids a race on the snapshot. */
#define VIA_TX_IDLE    0
#define VIA_TX_ARMED   1
#define VIA_TX_SENDING 2
static volatile uint8_t  via_tx_state;
static uint32_t          via_tx_base;

static uint32_t layout_options;

void via_init(void)
{
	via_rx_pos     = 0;
	via_cmd_ready  = 0;
	via_tx_state   = VIA_TX_IDLE;
	layout_options = 0;
}

/* ------------------------------------------------------------------------ */
/* Interrupt context                                                         */
/* ------------------------------------------------------------------------ */

void via_receive_packet(uint8_t *data, int len)
{
	if (via_cmd_ready || via_tx_state == VIA_TX_ARMED) {
		/* Previous exchange still in flight. VIA is strictly request/response
		 * so this only fires on a stray host. */
		return;
	}
	if (via_tx_state == VIA_TX_SENDING) {
		/* The host only sends a new command once it has read the whole reply,
		 * so close the reply out here instead of waiting for the extra IN
		 * poll that would otherwise be needed to notice. */
		via_tx_state = VIA_TX_IDLE;
	}

	uint8_t pos = via_rx_pos;
	for (int i = 0; i < len && pos < VIA_PACKET_SIZE; i++) {
		via_buf[pos++] = data[i];
	}
	via_rx_pos = pos;

	/* A full 32 bytes, or a short packet ending the transfer. */
	if (pos >= VIA_PACKET_SIZE || len < 8) {
		via_rx_pos    = 0;
		via_cmd_ready = 1;
	}
}

void via_handle_in(struct usb_endpoint *e, uint32_t sendtok)
{
	if (via_tx_state == VIA_TX_ARMED) {
		/* Latch the baseline here rather than in the main loop: e->count also
		 * advances for the empty packets sent while idle. */
		via_tx_base  = e->count;
		via_tx_state = VIA_TX_SENDING;
	}

	if (via_tx_state != VIA_TX_SENDING) {
		usb_send_empty(sendtok);
		return;
	}

	uint32_t chunk = (uint32_t)e->count - via_tx_base;
	if (chunk >= VIA_PACKET_SIZE / 8) {
		via_tx_state = VIA_TX_IDLE;
		usb_send_empty(sendtok);
		return;
	}

	usb_send_data(&via_buf[chunk * 8], 8, 0, sendtok);
}

/* ------------------------------------------------------------------------ */
/* Dynamic keymap buffer access                                              */
/*                                                                           */
/* current_keymap is a little-endian uint16_t array in RAM, but VIA's buffer  */
/* view is big-endian keycodes, so byte accesses swap halves.                 */
/* ------------------------------------------------------------------------ */

static uint8_t dk_read_byte(uint16_t offset)
{
	uint16_t idx = offset >> 1;
	if (idx >= KEYMAP_CELLS) {
		return 0;
	}
	uint16_t kc = ((const uint16_t *)current_keymap)[idx];
	return (offset & 1) ? (uint8_t)(kc & 0xFF) : (uint8_t)(kc >> 8);
}

static void dk_write_byte(uint16_t offset, uint8_t value)
{
	uint16_t idx = offset >> 1;
	if (idx >= KEYMAP_CELLS) {
		return;
	}
	uint16_t *p = &((uint16_t *)current_keymap)[idx];
	if (offset & 1) {
		*p = (uint16_t)((*p & 0xFF00) | value);
	} else {
		*p = (uint16_t)((*p & 0x00FF) | ((uint16_t)value << 8));
	}
}

static void put_be32(uint8_t *d, uint32_t v)
{
	d[0] = (uint8_t)(v >> 24);
	d[1] = (uint8_t)(v >> 16);
	d[2] = (uint8_t)(v >> 8);
	d[3] = (uint8_t)v;
}

static uint32_t get_be32(const uint8_t *d)
{
	return ((uint32_t)d[0] << 24) | ((uint32_t)d[1] << 16) |
	       ((uint32_t)d[2] << 8) | (uint32_t)d[3];
}

/* ------------------------------------------------------------------------ */
/* Main loop context                                                         */
/* ------------------------------------------------------------------------ */

static void via_get_keyboard_value(uint8_t *msg)
{
	switch (msg[1]) {
	case ID_UPTIME:
		put_be32(&msg[2], timer_ms);
		break;

	case ID_LAYOUT_OPTIONS:
		put_be32(&msg[2], layout_options);
		break;

	case ID_SWITCH_MATRIX_STATE: {
		/* Bit-packed live matrix, used by Remap's Test Matrix view. Each row
		 * goes out most significant byte first - that is what QMK's via.c
		 * emits, and what Remap decodes. */
		uint8_t *d = &msg[2];
		for (uint8_t r = 0; r < LOGICAL_ROWS; r++) {
			matrix_row_t state = global_matrix_state[r];
			for (int8_t b = VIA_MATRIX_ROW_BYTES - 1; b >= 0; b--) {
				if (d >= &msg[VIA_PACKET_SIZE]) {
					return;
				}
				*d++ = (uint8_t)(state >> (8 * b));
			}
		}
		break;
	}

	case ID_FIRMWARE_VERSION:
		put_be32(&msg[2], VIA_FIRMWARE_VERSION);
		break;

	default:
		msg[0] = ID_UNHANDLED;
		break;
	}
}

static void via_set_keyboard_value(uint8_t *msg)
{
	switch (msg[1]) {
	case ID_LAYOUT_OPTIONS:
		layout_options = get_be32(&msg[2]);
		break;

	case ID_DEVICE_INDICATION:
		/* No indicator hardware is mandated; acknowledge and move on. */
		break;

	case ID_UMK_SAVE_NOW:
		flash_store_request_save();
		break;

	default:
		msg[0] = ID_UNHANDLED;
		break;
	}
}

void via_task(void)
{
	if (!via_cmd_ready) {
		return;
	}

	uint8_t *msg = via_buf;

	/* Any VIA traffic pushes the autosave deadline back. */
	flash_store_note_activity();

	switch (msg[0]) {
	case ID_GET_PROTOCOL_VERSION:
		msg[1] = (uint8_t)(VIA_PROTOCOL_VERSION >> 8);
		msg[2] = (uint8_t)(VIA_PROTOCOL_VERSION & 0xFF);
		break;

	case ID_GET_KEYBOARD_VALUE:
		via_get_keyboard_value(msg);
		break;

	case ID_SET_KEYBOARD_VALUE:
		via_set_keyboard_value(msg);
		break;

	case ID_DYNAMIC_KEYMAP_GET_KEYCODE: {
		uint16_t kc = KC_NO;
		if (msg[1] < LAYERS && msg[2] < LOGICAL_ROWS && msg[3] < LOGICAL_COLS) {
			kc = current_keymap[msg[1]][msg[2]][msg[3]];
		}
		msg[4] = (uint8_t)(kc >> 8);
		msg[5] = (uint8_t)(kc & 0xFF);
		break;
	}

	case ID_DYNAMIC_KEYMAP_SET_KEYCODE:
		if (msg[1] < LAYERS && msg[2] < LOGICAL_ROWS && msg[3] < LOGICAL_COLS) {
			current_keymap[msg[1]][msg[2]][msg[3]] =
				(uint16_t)(((uint16_t)msg[4] << 8) | msg[5]);
			flash_store_mark_dirty();
		}
		break;

	case ID_DYNAMIC_KEYMAP_RESET:
	case ID_EEPROM_RESET:
		keymap_reset_to_default();
		flash_store_mark_dirty();
		break;

	/* Macros are out of scope. Reporting zero count and zero buffer is what
	 * makes Remap grey its macro UI out instead of erroring. */
	case ID_DYNAMIC_KEYMAP_MACRO_GET_COUNT:
		msg[1] = 0;
		break;

	case ID_DYNAMIC_KEYMAP_MACRO_GET_BUFSZ:
		msg[1] = 0;
		msg[2] = 0;
		break;

	case ID_DYNAMIC_KEYMAP_MACRO_GET_BUFFER: {
		uint8_t size = msg[3];
		if (size > 28) {
			size = 28;
		}
		for (uint8_t i = 0; i < size; i++) {
			msg[4 + i] = 0;
		}
		break;
	}

	case ID_DYNAMIC_KEYMAP_MACRO_SET_BUFFER:
	case ID_DYNAMIC_KEYMAP_MACRO_RESET:
		break;

	case ID_DYNAMIC_KEYMAP_GET_LAYER_COUNT:
		msg[1] = LAYERS;
		break;

	case ID_DYNAMIC_KEYMAP_GET_BUFFER: {
		uint16_t offset = (uint16_t)(((uint16_t)msg[1] << 8) | msg[2]);
		uint8_t  size   = msg[3];
		if (size > 28) {
			size = 28;
		}
		for (uint8_t i = 0; i < size; i++) {
			msg[4 + i] = dk_read_byte((uint16_t)(offset + i));
		}
		break;
	}

	case ID_DYNAMIC_KEYMAP_SET_BUFFER: {
		uint16_t offset = (uint16_t)(((uint16_t)msg[1] << 8) | msg[2]);
		uint8_t  size   = msg[3];
		if (size > 28) {
			size = 28;
		}
		for (uint8_t i = 0; i < size; i++) {
			dk_write_byte((uint16_t)(offset + i), msg[4 + i]);
		}
		flash_store_mark_dirty();
		break;
	}

	/* Encoders (0x14/0x15) and lighting (0x07-0x09) are out of scope. */
	default:
		msg[0] = ID_UNHANDLED;
		break;
	}

	via_cmd_ready = 0;
	via_tx_state  = VIA_TX_ARMED;
}
