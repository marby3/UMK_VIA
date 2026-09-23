/* UMK_VIA - resolves a keyboard's CUSTOM_* knobs into the internal constants
 * the rest of the firmware is written against.
 *
 * This lives in core/ rather than drivers/ so that core modules do not have to
 * include a driver header just to learn how big the keymap is. Everything here
 * is compile-time only; no code, no state.
 */
#ifndef _UMK_BOARD_CONFIG_H
#define _UMK_BOARD_CONFIG_H

#include <stdint.h>
#include "config.h" /* keyboards/<name>/config.h, via -I */

/* ------------------------------------------------------------------------ */
/* Physical matrix geometry                                                  */
/* ------------------------------------------------------------------------ */

#if defined(CUSTOM_DIRECT_PIN_MODE)
/* Direct pin mode (macropads): no row pins exist, every switch sits on its own
 * column pin. Internally we pretend there is exactly one row. */
#define MATRIX_ROWS 1
#elif defined(CUSTOM_MATRIX_ROWS)
#define MATRIX_ROWS CUSTOM_MATRIX_ROWS
#else
#define MATRIX_ROWS 4
#endif

#if defined(CUSTOM_MATRIX_COLS)
#define MATRIX_COLS CUSTOM_MATRIX_COLS
#else
#define MATRIX_COLS 6
#endif

/* ------------------------------------------------------------------------ */
/* Logical matrix geometry (what VIA / Remap sees)                           */
/*                                                                           */
/* NOTE: build_server.py::logical_matrix_size() mirrors these rules. Change   */
/* one and you MUST change the other, or generated Remap definitions will     */
/* disagree with the firmware.                                               */
/* ------------------------------------------------------------------------ */

#if defined(CUSTOM_SPLIT_ENABLE) && defined(CUSTOM_SPLIT_COMBINE_ROWS)
#define LOGICAL_ROWS (MATRIX_ROWS * 2)
#define LOGICAL_COLS (MATRIX_COLS)
#elif defined(CUSTOM_SPLIT_ENABLE)
/* CUSTOM_SPLIT_COMBINE_COLS is the default combine direction. */
#define LOGICAL_ROWS (MATRIX_ROWS)
#define LOGICAL_COLS (MATRIX_COLS * 2)
#else
#define LOGICAL_ROWS (MATRIX_ROWS)
#define LOGICAL_COLS (MATRIX_COLS)
#endif

#if LOGICAL_COLS > 32
#error "LOGICAL_COLS must be <= 32 (one matrix row has to fit in a uint32_t)"
#endif

/* One packed row of switch state. Width follows the logical column count so
 * the same type can carry both local and combined state. */
#if LOGICAL_COLS <= 8
typedef uint8_t matrix_row_t;
#define VIA_MATRIX_ROW_BYTES 1
#elif LOGICAL_COLS <= 16
typedef uint16_t matrix_row_t;
#define VIA_MATRIX_ROW_BYTES 2
#else
typedef uint32_t matrix_row_t;
#define VIA_MATRIX_ROW_BYTES 4
#endif

/* Bytes per row on the split wire protocol - always sized from the *physical*
 * column count because that is what one half actually scans. */
#define SPLIT_ROW_BYTES ((MATRIX_COLS + 7) / 8)

/* ------------------------------------------------------------------------ */
/* Layers                                                                    */
/* ------------------------------------------------------------------------ */

#ifdef CUSTOM_LAYERS
#define LAYERS CUSTOM_LAYERS
#else
#define LAYERS 4
#endif

#if LAYERS > 16
#error "LAYERS must be <= 16 (layer_state is a uint16_t bitmask)"
#endif

/* RAM cost of the live keymap. The CH32V003 only has 2KB total. */
#define KEYMAP_CELLS (LAYERS * LOGICAL_ROWS * LOGICAL_COLS)
#define KEYMAP_BYTES (KEYMAP_CELLS * 2)

/* ------------------------------------------------------------------------ */
/* Timing                                                                    */
/* ------------------------------------------------------------------------ */

#ifndef DEBOUNCE_MS
#define DEBOUNCE_MS 5
#endif

#ifndef TAPPING_TERM_MS
#define TAPPING_TERM_MS 200
#endif

#ifndef TAP_REPORT_MS
#define TAP_REPORT_MS 20
#endif

#ifndef FLASH_AUTOSAVE_DELAY_MS
#define FLASH_AUTOSAVE_DELAY_MS 750
#endif

/* ------------------------------------------------------------------------ */
/* Shared 1ms time base. Owned and advanced by core/main.c.                   */
/* ------------------------------------------------------------------------ */
extern volatile uint32_t timer_ms;

#endif
