#include "via.h"
#include "keymap.h"
#include "matrix.h"
#include "flash_store.h"
#include <string.h>

// ---------------------------------------------------------------------------
// VIA コマンドID (QMK quantum/via.h と同じ)
// ---------------------------------------------------------------------------
enum via_command_id {
    id_get_protocol_version                 = 0x01,
    id_get_keyboard_value                   = 0x02,
    id_set_keyboard_value                   = 0x03,
    id_dynamic_keymap_get_keycode           = 0x04,
    id_dynamic_keymap_set_keycode           = 0x05,
    id_dynamic_keymap_reset                 = 0x06,
    id_custom_set_value                     = 0x07,
    id_custom_get_value                     = 0x08,
    id_custom_save                          = 0x09,
    id_eeprom_reset                         = 0x0A,
    id_bootloader_jump                      = 0x0B,
    id_dynamic_keymap_macro_get_count       = 0x0C,
    id_dynamic_keymap_macro_get_buffer_size = 0x0D,
    id_dynamic_keymap_macro_get_buffer      = 0x0E,
    id_dynamic_keymap_macro_set_buffer      = 0x0F,
    id_dynamic_keymap_macro_reset           = 0x10,
    id_dynamic_keymap_get_layer_count       = 0x11,
    id_dynamic_keymap_get_buffer            = 0x12,
    id_dynamic_keymap_set_buffer            = 0x13,
    id_dynamic_keymap_get_encoder           = 0x14,
    id_dynamic_keymap_set_encoder           = 0x15,
    id_unhandled                            = 0xFF,
};

enum via_keyboard_value_id {
    id_uptime              = 0x01,
    id_layout_options      = 0x02,
    id_switch_matrix_state = 0x03,
    id_firmware_version    = 0x04,
    id_device_indication   = 0x05,
    // BLE Micro Pro 系で使われている「今すぐ不揮発領域へ保存」コマンド。
    // 本ファームウェアでは Flash への即時書き込みに割り当てています。
    id_store_keymap_persistently = 0xFF,
};

// id_dynamic_keymap_get_buffer / set_buffer で1回に運べるバイト数
// (32 - コマンドID(1) - offset(2) - size(1) = 28)
#define VIA_BUFFER_CHUNK_MAX 28

// id_switch_matrix_state における1行あたりのバイト数 (QMK の matrix_row_t 相当)
#if LOGICAL_COLS <= 8
#define VIA_MATRIX_ROW_BYTES 1
#elif LOGICAL_COLS <= 16
#define VIA_MATRIX_ROW_BYTES 2
#else
#define VIA_MATRIX_ROW_BYTES 4
#endif

// ---------------------------------------------------------------------------
// 32バイトレポートの組み立て / 分割
//
// rv003usb は Low-Speed USB のため 1 パケット 8 バイトが上限です。
// VIA の 32 バイトレポートはホスト側で 8 バイト x 4 トランザクションに
// 分割されるので、ここで再構成します。
//
// VIA は要求レポートを書き換えて返す (エコーバック) 仕様なので、
// 送受信で同じバッファを使います。rv003usb は 4 バイト境界前提のため揃えます。
// ---------------------------------------------------------------------------
static uint8_t via_buf[VIA_RAW_EPSIZE] __attribute__((aligned(4)));
static uint8_t via_rx_len;

// 受信完了フラグ。実際のコマンド処理はメインループ (via_task) で行います。
static volatile uint8_t via_cmd_ready;

// 応答の送信状態
#define VIA_TX_IDLE    0
#define VIA_TX_QUEUED  1  // 応答を用意済み。次の IN から送信開始する
#define VIA_TX_SENDING 2  // 送信中
static volatile uint8_t via_tx_state;
static uint32_t via_tx_base; // 送信開始時点の ACK カウンタ

static uint32_t via_layout_options;
static volatile uint32_t via_now_ms;

static void via_process_command(void);

void via_init(void) {
    via_rx_len = 0;
    via_cmd_ready = 0;
    via_tx_state = VIA_TX_IDLE;
    via_tx_base = 0;
    via_layout_options = 0;
    via_now_ms = 0;
}

void via_task(uint32_t now_ms) {
    via_now_ms = now_ms;

    // コマンド処理は割り込み外で行う。
    // rv003usb は usb_handle_user_data() から戻った直後に ACK を返すため、
    // 割り込み内で時間のかかる処理 (キーマップ全体の初期化など) を行うと
    // Low-Speed USB のターンアラウンド時間を守れず通信が壊れます。
    if (via_cmd_ready) {
        via_process_command();
        via_cmd_ready = 0;
        via_tx_state = VIA_TX_QUEUED;
    }
}

// ---------------------------------------------------------------------------
// dynamic keymap アクセサ
// VIA のバッファ表現はキーコードをビッグエンディアンで並べたバイト列です。
// RAM 上の current_keymap は uint16 のリトルエンディアンなので、
// バイト単位アクセス時に入れ替えます。
// ---------------------------------------------------------------------------
static uint16_t dynamic_keymap_get_keycode(uint8_t layer, uint8_t row, uint8_t col) {
    if (layer >= LAYERS || row >= LOGICAL_ROWS || col >= LOGICAL_COLS) return KC_NO;
    return current_keymap[layer][row][col];
}

static void dynamic_keymap_set_keycode(uint8_t layer, uint8_t row, uint8_t col, uint16_t keycode) {
    if (layer >= LAYERS || row >= LOGICAL_ROWS || col >= LOGICAL_COLS) return;
    current_keymap[layer][row][col] = keycode;
}

static uint8_t dynamic_keymap_get_byte(uint16_t index) {
    if (index >= KEYMAP_TOTAL_BYTES) return 0;
    const uint16_t *keymap = (const uint16_t *)current_keymap;
    uint16_t keycode = keymap[index >> 1];
    return (index & 1) ? (keycode & 0xFF) : (keycode >> 8);
}

static void dynamic_keymap_set_byte(uint16_t index, uint8_t value) {
    if (index >= KEYMAP_TOTAL_BYTES) return;
    uint16_t *keymap = (uint16_t *)current_keymap;
    uint16_t keycode = keymap[index >> 1];
    keymap[index >> 1] = (index & 1)
        ? ((keycode & 0xFF00) | value)
        : ((keycode & 0x00FF) | ((uint16_t)value << 8));
}

// Remap の Test Matrix 機能向け。command_data[1] は返却開始バイトのオフセット。
static void via_get_switch_matrix_state(uint8_t *command_data) {
    uint8_t offset = command_data[1];
    uint8_t out = 2;
    uint16_t index = 0;

    for (uint8_t row = 0; row < LOGICAL_ROWS; row++) {
        uint32_t value = 0;
        for (uint8_t col = 0; col < LOGICAL_COLS; col++) {
            if (global_matrix_state[row][col]) value |= (1UL << col);
        }
        for (int8_t b = VIA_MATRIX_ROW_BYTES - 1; b >= 0; b--, index++) {
            if (index < offset) continue;
            if (out >= VIA_RAW_EPSIZE - 1) return;
            command_data[out++] = (value >> (8 * b)) & 0xFF;
        }
    }
}

// ---------------------------------------------------------------------------
// コマンド処理 (メインループから呼ばれる)
// VIA は受信したレポートを書き換えてそのまま返す (エコーバック) 仕様です。
// Remap 側も先頭バイト群が一致するかで応答を照合しているため、
// 要求内容をそのまま残す必要があります。
// ---------------------------------------------------------------------------
static void via_process_command(void) {
    uint8_t *command_id   = &via_buf[0];
    uint8_t *command_data = &via_buf[1];

    switch (*command_id) {
    case id_get_protocol_version:
        command_data[0] = (VIA_PROTOCOL_VERSION >> 8) & 0xFF;
        command_data[1] = VIA_PROTOCOL_VERSION & 0xFF;
        break;

    case id_get_keyboard_value:
        switch (command_data[0]) {
        case id_uptime: {
            uint32_t value = via_now_ms;
            command_data[1] = (value >> 24) & 0xFF;
            command_data[2] = (value >> 16) & 0xFF;
            command_data[3] = (value >> 8) & 0xFF;
            command_data[4] = value & 0xFF;
            break;
        }
        case id_layout_options: {
            uint32_t value = via_layout_options;
            command_data[1] = (value >> 24) & 0xFF;
            command_data[2] = (value >> 16) & 0xFF;
            command_data[3] = (value >> 8) & 0xFF;
            command_data[4] = value & 0xFF;
            break;
        }
        case id_switch_matrix_state:
            via_get_switch_matrix_state(command_data);
            break;
        case id_firmware_version: {
            uint32_t value = VIA_FIRMWARE_VERSION;
            command_data[1] = (value >> 24) & 0xFF;
            command_data[2] = (value >> 16) & 0xFF;
            command_data[3] = (value >> 8) & 0xFF;
            command_data[4] = value & 0xFF;
            break;
        }
        default:
            *command_id = id_unhandled;
            break;
        }
        break;

    case id_set_keyboard_value:
        switch (command_data[0]) {
        case id_layout_options:
            via_layout_options = ((uint32_t)command_data[1] << 24) |
                                 ((uint32_t)command_data[2] << 16) |
                                 ((uint32_t)command_data[3] << 8) |
                                 ((uint32_t)command_data[4]);
            break;
        case id_device_indication:
            // LED 点滅等による識別表示。未対応だがエラーにはしない。
            break;
        case id_store_keymap_persistently:
            // 実際の書き込みは割り込み外 (メインループ) で行う
            flash_store_request_save();
            command_data[1] = 0x00; // resultCode: 成功
            break;
        default:
            *command_id = id_unhandled;
            break;
        }
        break;

    case id_dynamic_keymap_get_keycode: {
        uint16_t keycode = dynamic_keymap_get_keycode(command_data[0], command_data[1], command_data[2]);
        command_data[3] = (keycode >> 8) & 0xFF;
        command_data[4] = keycode & 0xFF;
        break;
    }

    case id_dynamic_keymap_set_keycode:
        dynamic_keymap_set_keycode(command_data[0], command_data[1], command_data[2],
                                   ((uint16_t)command_data[3] << 8) | command_data[4]);
        flash_store_mark_dirty(via_now_ms);
        break;

    case id_dynamic_keymap_reset:
    case id_eeprom_reset:
        keymap_reset_to_default();
        flash_store_mark_dirty(via_now_ms);
        break;

    case id_dynamic_keymap_get_layer_count:
        command_data[0] = LAYERS;
        break;

    case id_dynamic_keymap_get_buffer: {
        uint16_t offset = ((uint16_t)command_data[0] << 8) | command_data[1];
        uint8_t  size   = command_data[2];
        if (size > VIA_BUFFER_CHUNK_MAX) size = VIA_BUFFER_CHUNK_MAX;
        for (uint8_t i = 0; i < size; i++) {
            command_data[3 + i] = dynamic_keymap_get_byte(offset + i);
        }
        break;
    }

    case id_dynamic_keymap_set_buffer: {
        uint16_t offset = ((uint16_t)command_data[0] << 8) | command_data[1];
        uint8_t  size   = command_data[2];
        if (size > VIA_BUFFER_CHUNK_MAX) size = VIA_BUFFER_CHUNK_MAX;
        for (uint8_t i = 0; i < size; i++) {
            dynamic_keymap_set_byte(offset + i, command_data[3 + i]);
        }
        flash_store_mark_dirty(via_now_ms);
        break;
    }

    // マクロは未対応。0 個 / バッファ 0 バイトと返すことで
    // Remap 側のマクロ UI が無効化されます。
    case id_dynamic_keymap_macro_get_count:
        command_data[0] = 0;
        break;
    case id_dynamic_keymap_macro_get_buffer_size:
        command_data[0] = 0;
        command_data[1] = 0;
        break;
    case id_dynamic_keymap_macro_get_buffer:
        memset(&command_data[3], 0, VIA_RAW_EPSIZE - 4);
        break;
    case id_dynamic_keymap_macro_set_buffer:
    case id_dynamic_keymap_macro_reset:
        break;

    // ロータリーエンコーダ / ライティング (Custom UI) は未対応
    case id_dynamic_keymap_get_encoder:
    case id_dynamic_keymap_set_encoder:
    case id_custom_get_value:
    case id_custom_set_value:
    case id_custom_save:
    default:
        *command_id = id_unhandled;
        break;
    }
}

// EP2 OUT の割り込みハンドラ。
// rv003usb はこの関数から戻った直後にホストへ ACK を返すため、
// ここでは最小限のコピーだけを行い、コマンド処理は via_task() へ委譲します。
void via_receive_packet(const uint8_t *data, int len) {
    if (len <= 0) return;
    if (len > 8) len = 8;

    // 同期が外れた場合の保護
    if (via_rx_len + len > VIA_RAW_EPSIZE) via_rx_len = 0;

    memcpy(via_buf + via_rx_len, data, len);
    via_rx_len += len;

    // 32バイト揃った、または 8 バイト未満の短いパケット (転送終了) を受けたら確定
    if (via_rx_len >= VIA_RAW_EPSIZE || len < 8) {
        if (via_rx_len < VIA_RAW_EPSIZE) {
            memset(via_buf + via_rx_len, 0, VIA_RAW_EPSIZE - via_rx_len);
        }
        via_rx_len = 0;
        via_cmd_ready = 1;
    }
}

void via_handle_in(struct usb_endpoint *e, uint32_t sendtok) {
    // e->count はホストからの ACK ごとに +1 されます。
    // 送信開始時点の値を基準にして、送信済みチャンク数を求めます。
    // (基準値の取得も割り込み内で行うことで、メインループとの競合を避けます)
    if (via_tx_state == VIA_TX_QUEUED) {
        via_tx_base = e->count;
        via_tx_state = VIA_TX_SENDING;
    }

    if (via_tx_state == VIA_TX_SENDING) {
        uint32_t chunk = e->count - via_tx_base;
        if (chunk < (VIA_RAW_EPSIZE / 8)) {
            usb_send_data(via_buf + (chunk * 8), 8, 0, sendtok);
            return;
        }
        via_tx_state = VIA_TX_IDLE;
    }

    // 応答が無いときは必ず空パケットを返すこと。
    // ここでデータを返すと Remap は「要求していない応答」と判断して
    // 接続を閉じてしまいます (WebHid.ts の handleInputReport)。
    usb_send_empty(sendtok);
}
