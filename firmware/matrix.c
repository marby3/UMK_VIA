#include "ch32fun.h"
#include "matrix.h"

static const uint32_t row_pins[MATRIX_ROWS] = { PC0, PC1, PC2, PC3 };
static const uint32_t col_pins[MATRIX_COLS] = { PC4, PC5, PC6, PC7, PD0, PD2 };

static uint8_t raw_state[MATRIX_ROWS][MATRIX_COLS] = {0};
static uint32_t debounce_time[MATRIX_ROWS][MATRIX_COLS] = {0};

#define DEBOUNCE_DELAY 5 // ms

void matrix_init(void) {
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;

    for (int r = 0; r < MATRIX_ROWS; r++) {
        funPinMode(row_pins[r], GPIO_CFGLR_OUT_10Mhz_PP);
        funDigitalWrite(row_pins[r], 1);
    }

    for (int c = 0; c < MATRIX_COLS; c++) {
        funPinMode(col_pins[c], GPIO_CFGLR_IN_PUPD);
        funDigitalWrite(col_pins[c], 1);
    }
}

uint8_t matrix_scan(uint8_t debounced[MATRIX_ROWS][MATRIX_COLS], uint32_t now_ms) {
    uint8_t changed = 0;

    for (int r = 0; r < MATRIX_ROWS; r++) {
        funDigitalWrite(row_pins[r], 0);
        Delay_Us(10); // 明確なディレイを取って確実な安定を待つ

        for (int c = 0; c < MATRIX_COLS; c++) {
            uint8_t state = (funDigitalRead(col_pins[c]) == 0) ? 1 : 0;
            
            if (state != raw_state[r][c]) {
                raw_state[r][c] = state;
                debounce_time[r][c] = now_ms;
            }

            if ((now_ms - debounce_time[r][c]) >= DEBOUNCE_DELAY) {
                if (debounced[r][c] != state) {
                    debounced[r][c] = state;
                    changed = 1;
                }
            }
        }
        
        funDigitalWrite(row_pins[r], 1);
    }
    return changed;
}
