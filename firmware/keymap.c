#include "keymap.h"
#include <string.h>

uint16_t default_layer_state = 1;
uint16_t active_layer_state = 1;
uint16_t keycode_cache[MATRIX_ROWS][MATRIX_COLS];

// 初期状態のキーマップ（0初期化）。今後はPhase5のWebHID通信等によりFlashからロード・保存されます。
uint16_t current_keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS] = {0};

void keymap_init(void) {
    default_layer_state = 1;
    active_layer_state = 1;
    memset(keycode_cache, 0, sizeof(keycode_cache));
}

static uint16_t evaluate_keycode(uint8_t row, uint8_t col) {
    for (int i = LAYERS - 1; i >= 0; i--) {
        if (active_layer_state & (1 << i)) {
            uint16_t code = current_keymap[i][row][col];
            // ユーザービリティ向上: レイヤー1以上で「未割当(0x0000)」の場合は、自動的に透過(TRNS)として扱い下のレイヤーへフォールスルーする。
            if (code == KC_TRNS || (i > 0 && code == KC_NO)) {
                continue;
            }
            return code;
        }
    }
    return KC_NO;
}

void keymap_process_press(uint8_t row, uint8_t col) {
    // キーが押された瞬間のレイヤー状態からキーコードを確定し、キャッシュする
    uint16_t keycode = evaluate_keycode(row, col);
    keycode_cache[row][col] = keycode;
    
    // レイヤー操作キーか判定
    uint16_t action = keycode & 0xFF00;
    uint8_t param = keycode & 0x00FF;
    
    if (action == 0x0100) { // MO (Momentary Layer)
        if (param < LAYERS) active_layer_state |= (1 << param);
    }
    else if (action == 0x0200) { // TG (Toggle Layer)
        if (param < LAYERS) {
            active_layer_state ^= (1 << param);
            // デフォルトレイヤーはOffにしないように保護する（QMK仕様準拠）
            active_layer_state |= default_layer_state;
        }
    }
    else if (action == 0x0300) { // TO (Go To Layer)
        if (param < LAYERS) {
            default_layer_state = (1 << param);
            active_layer_state = default_layer_state;
        }
    }
}

void keymap_process_release(uint8_t row, uint8_t col) {
    // 離された物理キーに紐づく前回確定したキーコードを取得
    uint16_t keycode = keycode_cache[row][col];
    
    uint16_t action = keycode & 0xFF00;
    uint8_t param = keycode & 0x00FF;
    
    if (action == 0x0100) { // MO (Momentary Layer) レリーズ処理
        if (param < LAYERS) {
            active_layer_state &= ~(1 << param);
            // デフォルトレイヤーはOffにしないようにする
            active_layer_state |= default_layer_state;
        }
    }
    
    // キャッシュをクリア
    keycode_cache[row][col] = 0;
}

void keymap_generate_report(uint8_t *report_buf) {
    memset(report_buf, 0, 8);
    uint8_t key_idx = 2; 
    
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            uint16_t keycode = keycode_cache[r][c];
            
            // 下位バイト（USB HIDコード）を取得
            uint8_t usb_code = keycode & 0xFF;
            
            // HID基本キー (0x01 ~ 0xFF) のみ処理し、アクションキー(0x0100以上)は送信しない
            if ((keycode & 0xFF00) == 0) {
                if (usb_code >= 0xE0 && usb_code <= 0xE7) {
                    report_buf[0] |= (1 << (usb_code - 0xE0));
                } else if (usb_code > 0x00 && usb_code <= 0xA4) {
                    // Maximum of 6 keys due to standard USB boot keyboard limitations
                    if (key_idx < 8) {
                        report_buf[key_idx++] = usb_code;
                    }
                }
            }
        }
    }
}
