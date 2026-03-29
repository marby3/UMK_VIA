#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>
#include "matrix.h"

#define LAYERS 4

// 現在のキーマップを保持するRAM配置の多次元配列
extern uint16_t current_keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS];

// 現在のアクティブなレイヤー番号
extern uint8_t current_layer;

// マトリックス状態を受け取り、8バイトの標準的なBoot Keyboardレポートを生成する
void keymap_generate_report(uint8_t debounced_matrix[MATRIX_ROWS][MATRIX_COLS], uint8_t *report_buf);

#endif
