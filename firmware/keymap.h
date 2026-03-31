#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>
#include "matrix.h"

#define LAYERS 4

// キーコードマクロ
#define KC_NO   0x0000
#define KC_TRNS 0xFFFF

// 現在のキーマップを保持するRAM配置の多次元配列
extern uint16_t current_keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS];

// レイヤー状態
extern uint16_t default_layer_state; // `TO` 等で切り替わるベースレイヤー状態
extern uint16_t active_layer_state;  // 現在アクティブな全レイヤー(ビットマスク)

// キャッシュ
extern uint16_t keycode_cache[MATRIX_ROWS][MATRIX_COLS];

// 初期化
void keymap_init(void);

// エッジイベントの処理
void keymap_process_press(uint8_t row, uint8_t col);
void keymap_process_release(uint8_t row, uint8_t col);

// レポート生成
void keymap_generate_report(uint8_t *report_buf);

#endif
