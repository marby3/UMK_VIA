/* UMK_VIA - matrix / direct pin scanning.
 *
 * Geometry (MATRIX_ROWS/COLS, LOGICAL_ROWS/COLS, matrix_row_t) is resolved in
 * core/board_config.h; this header is only the scanning API.
 */
#ifndef _UMK_MATRIX_H
#define _UMK_MATRIX_H

#include <stdint.h>
#include "board_config.h"

/* This half's own switch state, MATRIX_ROWS entries. */
extern matrix_row_t local_matrix_state[MATRIX_ROWS];

/* Switch state as the keymap and VIA see it. On a non-split board this is a
 * plain copy of local_matrix_state. */
extern matrix_row_t global_matrix_state[LOGICAL_ROWS];

void matrix_init(void);

/* Scans the hardware and updates local_matrix_state with debouncing.
 * Must be called once per 1ms tick - the debounce timers count ticks. */
void matrix_scan(void);

#endif
