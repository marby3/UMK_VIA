#include "keymap.h"
#include <string.h>

uint16_t default_layer_state = 1;
uint16_t active_layer_state = 1;
uint16_t keycode_cache[LOGICAL_ROWS][LOGICAL_COLS];

// 現在のキーマップ。起動時に Flash から復元されるか、default_keymap で初期化されます。
uint16_t current_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS] = {0};

// 初期キーマップ。keyboards/<name>/keymap.c で同名シンボルを定義すると上書きされます。
__attribute__((weak)) const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS] = {0};

// ---------------------------------------------------------------------------
// Tap/Hold 状態
// 同時に保留できる Tap/Hold キーは1つだけです（小規模キーボード向けの簡略実装）。
// 2つ目の Tap/Hold キーが押された場合、先行キーはホールド確定として扱います。
// ---------------------------------------------------------------------------
static struct {
    uint8_t  pending;      // 1 = Tap/Hold 判定を保留中
    uint8_t  interrupted;  // 保留中に他のキーが押された
    uint8_t  row;
    uint8_t  col;
    uint32_t press_ms;
} tap_hold;

// タップ確定時に一定時間だけホストへ見せるキー
static uint8_t  tap_keycode;
static uint32_t tap_expire_ms;

void keymap_init(void) {
    default_layer_state = 1;
    active_layer_state = 1;
    memset(keycode_cache, 0, sizeof(keycode_cache));
    memset(&tap_hold, 0, sizeof(tap_hold));
    tap_keycode = 0;
    tap_expire_ms = 0;
}

void keymap_reset_to_default(void) {
    memcpy(current_keymap, default_keymap, sizeof(current_keymap));

#ifndef CUSTOM_NO_TRNS_DEFAULT
    // QMK 仕様では KC_NO は「割り当てなし」で確定し、下位レイヤーへは落ちません。
    // 初期状態でレイヤー1以降が全て KC_NO だと MO(1) 等が全キー無反応になるため、
    // 既定では上位レイヤーの未割当を透過(KC_TRNS)で埋めておきます。
    // 明示的に KC_NO を置きたい場合は config.h で CUSTOM_NO_TRNS_DEFAULT を定義してください。
    for (int l = 1; l < LAYERS; l++) {
        for (int r = 0; r < LOGICAL_ROWS; r++) {
            for (int c = 0; c < LOGICAL_COLS; c++) {
                if (current_keymap[l][r][c] == KC_NO) {
                    current_keymap[l][r][c] = KC_TRNS;
                }
            }
        }
    }
#endif

    keymap_init();
}

static uint16_t evaluate_keycode(uint8_t row, uint8_t col) {
    for (int i = LAYERS - 1; i >= 0; i--) {
        if (active_layer_state & (1 << i)) {
            uint16_t code = current_keymap[i][row][col];
            // QMK 仕様: KC_TRNS のみ下位レイヤーへフォールスルーする。
            // KC_NO は「割り当てなし」として確定するため、ここで打ち切ります。
            if (code == KC_TRNS) {
                continue;
            }
            return code;
        }
    }
    return KC_NO;
}

// レイヤーを ON にする (デフォルトレイヤーは常に有効のまま)
static void layer_on(uint8_t layer) {
    if (layer < LAYERS) active_layer_state |= (1 << layer);
}

static void layer_off(uint8_t layer) {
    if (layer < LAYERS) {
        active_layer_state &= ~(1 << layer);
        active_layer_state |= default_layer_state;
    }
}

// Tap/Hold キーのホールド側アクションを適用する (レイヤーONのみ。モディファイアは
// keymap_generate_report がキャッシュから直接読み取るためここでは何もしません)
static void tap_hold_apply(uint16_t keycode) {
    if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) {
        layer_on((keycode >> 8) & 0x0F);
    }
}

void keymap_process_press(uint8_t row, uint8_t col, uint32_t now_ms) {
    // キーが押された瞬間のレイヤー状態からキーコードを確定し、キャッシュする
    uint16_t keycode = evaluate_keycode(row, col);
    keycode_cache[row][col] = keycode;

    // 保留中の Tap/Hold キーがあれば、他キーの押下によりホールド確定とする
    if (tap_hold.pending) {
        tap_hold.interrupted = 1;
    }

    if ((keycode >= QK_MOD_TAP && keycode <= QK_MOD_TAP_MAX) ||
        (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX)) {
        tap_hold.pending     = 1;
        tap_hold.interrupted = 0;
        tap_hold.row         = row;
        tap_hold.col         = col;
        tap_hold.press_ms    = now_ms;
        tap_hold_apply(keycode);
        return;
    }

    if (keycode >= QK_TO && keycode <= (QK_TO | QK_LAYER_RANGE_MASK)) {
        uint8_t layer = keycode & QK_LAYER_RANGE_MASK;
        if (layer < LAYERS) {
            default_layer_state = (1 << layer);
            active_layer_state = default_layer_state;
        }
    } else if (keycode >= QK_MOMENTARY && keycode <= (QK_MOMENTARY | QK_LAYER_RANGE_MASK)) {
        layer_on(keycode & QK_LAYER_RANGE_MASK);
    } else if (keycode >= QK_DEF_LAYER && keycode <= (QK_DEF_LAYER | QK_LAYER_RANGE_MASK)) {
        uint8_t layer = keycode & QK_LAYER_RANGE_MASK;
        if (layer < LAYERS) {
            // 一時レイヤーは維持したままベースレイヤーだけ差し替える
            active_layer_state &= ~default_layer_state;
            default_layer_state = (1 << layer);
            active_layer_state |= default_layer_state;
        }
    } else if (keycode >= QK_TOGGLE_LAYER && keycode <= (QK_TOGGLE_LAYER | QK_LAYER_RANGE_MASK)) {
        uint8_t layer = keycode & QK_LAYER_RANGE_MASK;
        if (layer < LAYERS) {
            active_layer_state ^= (1 << layer);
            // デフォルトレイヤーはOffにしないように保護する（QMK仕様準拠）
            active_layer_state |= default_layer_state;
        }
    }
}

void keymap_process_release(uint8_t row, uint8_t col, uint32_t now_ms) {
    // 離された物理キーに紐づく前回確定したキーコードを取得
    uint16_t keycode = keycode_cache[row][col];

    if (tap_hold.pending && tap_hold.row == row && tap_hold.col == col) {
        // 短時間かつ他キーの割り込みが無ければタップ確定
        if (!tap_hold.interrupted &&
            (now_ms - tap_hold.press_ms) < TAPPING_TERM_MS) {
            tap_keycode   = keycode & 0xFF;
            tap_expire_ms = now_ms + TAP_REPORT_MS;
        }
        tap_hold.pending = 0;
    }

    // 押下時に有効化したレイヤーを解除する。
    // 保留対象が別の Tap/Hold キーへ移っていても取りこぼさないよう、
    // 保留状態ではなくキャッシュしたキーコードを見て判定します。
    if (keycode >= QK_LAYER_TAP && keycode <= QK_LAYER_TAP_MAX) {
        layer_off((keycode >> 8) & 0x0F);
    } else if (keycode >= QK_MOMENTARY && keycode <= (QK_MOMENTARY | QK_LAYER_RANGE_MASK)) {
        layer_off(keycode & QK_LAYER_RANGE_MASK);
    }

    // キャッシュをクリア
    keycode_cache[row][col] = KC_NO;
}

uint8_t keymap_task(uint32_t now_ms) {
    // タップ送出時間を過ぎたらキーを離す
    if (tap_keycode && (int32_t)(now_ms - tap_expire_ms) >= 0) {
        tap_keycode = 0;
        return 1;
    }
    return 0;
}

// QK_MODS / QK_MOD_TAP の 5bit モディファイアを HID レポートのモディファイアバイトへ変換
static uint8_t qk_mods_to_hid(uint8_t qk_mods) {
    uint8_t base = qk_mods & 0x0F;
    return (qk_mods & QK_MOD_RIGHT) ? (base << 4) : base;
}

static void add_keycode_to_report(uint16_t keycode, uint8_t *report_buf, uint8_t *key_idx) {
    uint8_t basic = 0;

    if (keycode == KC_NO || keycode == KC_TRNS) {
        return;
    }

    if (keycode <= QK_BASIC_MAX) {
        basic = keycode & 0xFF;
    } else if (keycode <= QK_MODS_MAX) {
        // モディファイア付き基本キー (例: LSFT(KC_A))
        report_buf[0] |= qk_mods_to_hid((keycode >> 8) & 0x1F);
        basic = keycode & 0xFF;
    } else if (keycode <= QK_MOD_TAP_MAX) {
        // Mod-Tap のホールド中はモディファイアのみを送る
        report_buf[0] |= qk_mods_to_hid((keycode >> 8) & 0x1F);
        return;
    } else {
        // Layer-Tap / MO / TO / TG / DF などはレポートに出さない
        return;
    }

    if (basic >= 0xE0 && basic <= 0xE7) {
        report_buf[0] |= (1 << (basic - 0xE0));
    } else if (basic > 0x00 && basic <= 0xA4) {
        // Boot Keyboard の制約により同時押しは最大6キー
        if (*key_idx < 8) {
            report_buf[(*key_idx)++] = basic;
        }
    }
}

void keymap_generate_report(uint8_t *report_buf) {
    memset(report_buf, 0, 8);
    uint8_t key_idx = 2;

    for (int r = 0; r < LOGICAL_ROWS; r++) {
        for (int c = 0; c < LOGICAL_COLS; c++) {
            add_keycode_to_report(keycode_cache[r][c], report_buf, &key_idx);
        }
    }

    // Tap/Hold のタップ確定分
    if (tap_keycode) {
        add_keycode_to_report(tap_keycode, report_buf, &key_idx);
    }
}
