#ifndef KEYMAP_H
#define KEYMAP_H

#include <stdint.h>
#include "matrix.h"

// レイヤー数。config.h で CUSTOM_LAYERS を定義すれば上書きできます。
// RAM 使用量 = LAYERS * LOGICAL_ROWS * LOGICAL_COLS * 2 バイト なので、
// CH32V003 (RAM 2KB) では増やしすぎないよう注意してください。
#ifndef CUSTOM_LAYERS
#define LAYERS 4
#else
#define LAYERS CUSTOM_LAYERS
#endif

// ---------------------------------------------------------------------------
// QMK 互換キーコード
//
// VIA/Remap は QMK の 16bit キーコードをそのまま送ってくるため、
// ファームウェア側も QMK と同じ数値体系で解釈する必要があります。
// (参考: QMK quantum_keycodes.h / Remap src/services/hid/compositions/*)
// ---------------------------------------------------------------------------
#define KC_NO               0x0000
#define KC_TRNS             0x0001

// 基本キー (USB HID Usage ID と同値)
#define QK_BASIC_MAX        0x00FF

// モディファイア付き基本キー: 上位 5bit がモディファイア、下位 8bit がキー
#define QK_MODS             0x0100
#define QK_MODS_MAX         0x1FFF

// Mod-Tap: 押しっぱなしでモディファイア、単押しで下位 8bit のキー
#define QK_MOD_TAP          0x2000
#define QK_MOD_TAP_MAX      0x3FFF

// Layer-Tap: 押しっぱなしでレイヤー、単押しで下位 8bit のキー
#define QK_LAYER_TAP        0x4000
#define QK_LAYER_TAP_MAX    0x4FFF

// レイヤー操作 (各レンジは 32 個 = 5bit のレイヤー番号)
#define QK_TO               0x5200  // TO(n)  : ベースレイヤーを n に切り替え
#define QK_MOMENTARY        0x5220  // MO(n)  : 押している間だけ n を有効化
#define QK_DEF_LAYER        0x5240  // DF(n)  : デフォルトレイヤーを n に変更
#define QK_TOGGLE_LAYER     0x5260  // TG(n)  : n の ON/OFF をトグル
#define QK_LAYER_RANGE_MASK 0x001F

// QK_MODS / QK_MOD_TAP のモディファイアビット
#define QK_MOD_CTRL         0x01
#define QK_MOD_SHIFT        0x02
#define QK_MOD_ALT          0x04
#define QK_MOD_GUI          0x08
#define QK_MOD_RIGHT        0x10

// Tap/Hold の判定時間 (ms)
#ifndef TAPPING_TERM_MS
#define TAPPING_TERM_MS 200
#endif

// タップ確定時にキーを押下状態としてホストへ見せる時間 (ms)
#ifndef TAP_REPORT_MS
#define TAP_REPORT_MS 20
#endif

// 現在のキーマップを保持するRAM配置の多次元配列
extern uint16_t current_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS];

// 起動時 / VIA の dynamic_keymap_reset で書き戻される初期キーマップ。
// キーボード側 (keyboards/<name>/keymap.c) で同名のシンボルを定義すると
// weak 定義が上書きされ、任意の初期配列を持たせることができます。
extern const uint16_t default_keymap[LAYERS][LOGICAL_ROWS][LOGICAL_COLS];

// レイヤー状態
extern uint16_t default_layer_state; // `TO`/`DF` 等で切り替わるベースレイヤー状態
extern uint16_t active_layer_state;  // 現在アクティブな全レイヤー(ビットマスク)

// キャッシュ
extern uint16_t keycode_cache[LOGICAL_ROWS][LOGICAL_COLS];

// 初期化
void keymap_init(void);

// 現在のキーマップを default_keymap で初期化する (VIA: id_dynamic_keymap_reset)
void keymap_reset_to_default(void);

// エッジイベントの処理
void keymap_process_press(uint8_t row, uint8_t col, uint32_t now_ms);
void keymap_process_release(uint8_t row, uint8_t col, uint32_t now_ms);

// 時間依存の処理 (Tap/Hold のタップ送出終了など)。
// レポートの作り直しが必要な場合に 1 を返します。
uint8_t keymap_task(uint32_t now_ms);

// レポート生成
void keymap_generate_report(uint8_t *report_buf);

#endif
