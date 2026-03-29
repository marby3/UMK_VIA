#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>

#define MATRIX_ROWS 4
#define MATRIX_COLS 6

// Initialize the matrix GPIO pins
void matrix_init(void);

// Scan the matrix. Returns 1 if any state changed, 0 otherwise.
uint8_t matrix_scan(uint8_t debounced_state[MATRIX_ROWS][MATRIX_COLS], uint32_t now_ms);

#endif
