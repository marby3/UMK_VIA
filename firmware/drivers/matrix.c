/* UMK_VIA - matrix / direct pin scanning. */

#include "ch32fun.h"
#include "matrix.h"

matrix_row_t local_matrix_state[MATRIX_ROWS];
matrix_row_t global_matrix_state[LOGICAL_ROWS];

static const uint8_t col_pins[MATRIX_COLS] = CUSTOM_COL_PINS;
#ifndef CUSTOM_DIRECT_PIN_MODE
static const uint8_t row_pins[MATRIX_ROWS] = CUSTOM_ROW_PINS;
#endif

/* Per key lockout, in 1ms ticks. A key that just changed ignores its raw
 * reading until its own timer expires, which keeps one bouncing switch from
 * masking the rest of the matrix. */
static uint8_t debounce_timer[MATRIX_ROWS][MATRIX_COLS];

void matrix_init(void)
{
	funGpioInitAll();

#ifndef CUSTOM_DIRECT_PIN_MODE
	/* Rows idle high-impedance; only the row being strobed is driven low. */
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		funPinMode(row_pins[r], GPIO_CFGLR_IN_FLOAT);
	}
#endif

	/* Columns are always pulled-up inputs: a pressed switch reads 0.
	 * OUTDR picks pull-up vs pull-down in IN_PUPD mode, so set it before
	 * switching the pin over - otherwise the pin briefly pulls down. */
	for (uint8_t c = 0; c < MATRIX_COLS; c++) {
		funDigitalWrite(col_pins[c], FUN_HIGH);
		funPinMode(col_pins[c], GPIO_CFGLR_IN_PUPD);
	}

	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		local_matrix_state[r] = 0;
		for (uint8_t c = 0; c < MATRIX_COLS; c++) {
			debounce_timer[r][c] = 0;
		}
	}
}

/* Folds one raw reading into the debounced state of a single key. */
static inline void matrix_apply(uint8_t r, uint8_t c, uint8_t pressed)
{
	if (debounce_timer[r][c]) {
		debounce_timer[r][c]--;
		return;
	}

	matrix_row_t bit = ((matrix_row_t)1) << c;
	uint8_t was = (local_matrix_state[r] & bit) ? 1 : 0;
	if (was == pressed) {
		return;
	}

	if (pressed) {
		local_matrix_state[r] |= bit;
	} else {
		local_matrix_state[r] &= (matrix_row_t)~bit;
	}
	debounce_timer[r][c] = DEBOUNCE_MS;
}

void matrix_scan(void)
{
#ifdef CUSTOM_DIRECT_PIN_MODE
	for (uint8_t c = 0; c < MATRIX_COLS; c++) {
		matrix_apply(0, c, !funDigitalRead(col_pins[c]));
	}
#else
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		/* Drive this row low, leave every other row floating. */
		funDigitalWrite(row_pins[r], FUN_LOW);
		funPinMode(row_pins[r], GPIO_CFGLR_OUT_10Mhz_PP);
		Delay_Us(10); /* let the column pull-ups settle */

		for (uint8_t c = 0; c < MATRIX_COLS; c++) {
			matrix_apply(r, c, !funDigitalRead(col_pins[c]));
		}

		funPinMode(row_pins[r], GPIO_CFGLR_IN_FLOAT);
	}
#endif
}
