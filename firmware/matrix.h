#ifndef MATRIX_H
#define MATRIX_H

#include <stdint.h>

#if __has_include("config.h")
#include "config.h"
#endif

#ifndef CUSTOM_MATRIX_ROWS
#define MATRIX_ROWS 4
#else
#define MATRIX_ROWS CUSTOM_MATRIX_ROWS
#endif

#ifndef CUSTOM_MATRIX_COLS
#define MATRIX_COLS 6
#else
#define MATRIX_COLS CUSTOM_MATRIX_COLS
#endif

// 論理マトリクス（分割キーボード結合後）のサイズ定義
#ifdef CUSTOM_SPLIT_ENABLE
    #ifdef CUSTOM_SPLIT_COMBINE_COLS
        #define LOGICAL_ROWS MATRIX_ROWS
        #define LOGICAL_COLS (MATRIX_COLS * 2)
    #elif defined(CUSTOM_SPLIT_COMBINE_ROWS)
        #define LOGICAL_ROWS (MATRIX_ROWS * 2)
        #define LOGICAL_COLS MATRIX_COLS
    #else
        #define LOGICAL_ROWS MATRIX_ROWS
        #define LOGICAL_COLS MATRIX_COLS
    #endif
#else
    #define LOGICAL_ROWS MATRIX_ROWS
    #define LOGICAL_COLS MATRIX_COLS
#endif

// 分割キーボード結合後の論理マトリクス状態 (実体は main.c)
extern uint8_t global_matrix_state[LOGICAL_ROWS][LOGICAL_COLS];

// Initialize the matrix GPIO pins
void matrix_init(void);

// Scan the matrix. Returns 1 if any state changed, 0 otherwise.
uint8_t matrix_scan(uint8_t debounced_state[MATRIX_ROWS][MATRIX_COLS], uint32_t now_ms);

#endif
