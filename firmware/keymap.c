#include "keymap.h"
#include <string.h>

uint8_t current_layer = 0;

// 初期状態の簡単なキーマップ。後にPhase 4で起動時にFlashからロードされるようにします。
// テスト用に、1行目にA, B, C, D, E, Fをアサインしておきます。(A = 0x04)
uint16_t current_keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS] = {
    // Layer 0
    {
        { 0x04, 0x05, 0x06, 0x07, 0x08, 0x09 },
        { 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
    }
};

void keymap_generate_report(uint8_t debounced_matrix[MATRIX_ROWS][MATRIX_COLS], uint8_t *report_buf) {
    // 初期化: 8 bytes [Modifier (1 byte), Reserved (1 byte), Key1~Key6 (6 bytes)]
    memset(report_buf, 0, 8);
    uint8_t key_idx = 2; 
    
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            // キーが押下されているか確認
            if (debounced_matrix[r][c]) {
                uint16_t keycode = current_keymap[current_layer][r][c];
                
                // QMK Basic Keycodeの修飾キー (0xE0 ~ 0xE7: Ctrl, Shift, Alt, GUI)
                if (keycode >= 0xE0 && keycode <= 0xE7) {
                    report_buf[0] |= (1 << (keycode - 0xE0));
                } 
                // 通常のキー (1キーからA4までが一般的な英数字等)
                else if (keycode > 0x00 && keycode <= 0xA4) {
                    // Maximum of 6 keys due to standard USB boot keyboard limitations
                    if (key_idx < 8) {
                        report_buf[key_idx++] = (uint8_t)keycode;
                    }
                }
            }
        }
    }
}
