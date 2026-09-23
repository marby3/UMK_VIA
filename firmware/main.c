#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "matrix.h"
#include "split.h"
#include "rgb_led.h"
#include "keymap.h"
#include "flash_store.h"
#include "via.h"

// 状態管理
uint8_t local_matrix_state[MATRIX_ROWS][MATRIX_COLS] = {0};
uint8_t global_matrix_state[LOGICAL_ROWS][LOGICAL_COLS] = {0};

// キーボード用のレポートバッファ (EP2 の VIA レポートは via.c 側が保持)
uint8_t current_keyboard_report[8] = { 0 };

volatile uint8_t usb_configured_flag = 0; // USB通信が確立したかどうかのフラグ

int main()
{
	SystemInit();
	Delay_Ms(1); // USB再認識用のディレイ
    
    // Phase 4: 起動時に保存されたキー配列があればRAMへロードする
    // (保存が無い / 構成が変わっている場合は default_keymap で初期化されます)
    flash_store_load();

    // Phase 2: ピンの初期化
    matrix_init();

    // Phase 6: キーコードキャッシュとレイヤー状態の初期化
    keymap_init();

    // VIA (Remap) プロトコル層の初期化
    via_init();

    // Phase 1: USBセットアップ
	usb_setup();

#ifdef CUSTOM_RGB_ENABLE
    // Phase 9: RGB LED
    rgb_led_init();
#endif

#ifdef CUSTOM_SPLIT_ENABLE
    split_init();
    
    // USBコンフィギュレーション（Hostへの接続完了）を待機してMaster/Slaveを決定
    uint32_t wait_ms = 500;
    while (!usb_configured_flag && wait_ms > 0) {
        Delay_Ms(1);
        wait_ms--;
    }
    
    if (usb_configured_flag) {
        split_set_master(1); // Master Mode
    } else {
        split_set_master(0); // Slave Mode
    }
#endif

	uint32_t system_millis = 0;
    static uint8_t last_global_matrix_state[LOGICAL_ROWS][LOGICAL_COLS] = {0};

	while(1)
	{
		Delay_Ms(1);
		system_millis++;

#ifdef CUSTOM_SPLIT_ENABLE
        // If Slave, skip normal main logic and just run the slave task.
        if (!split_is_master()) {
            uint8_t changed = matrix_scan(local_matrix_state, system_millis);
            split_slave_task(local_matrix_state, system_millis, changed);
            continue;
        }
#endif

        via_task(system_millis);

        // VIA でキーマップが書き換えられていれば、通信が落ち着いた頃に自動保存する
        flash_store_task(system_millis);

#ifdef CUSTOM_RGB_ENABLE
        rgb_led_task(system_millis);
#endif

        // Phase 2: マトリックススキャン (ローカル部分)
        uint8_t changed = matrix_scan(local_matrix_state, system_millis);
        
        // Split Keyboardの処理
#ifdef CUSTOM_SPLIT_ENABLE
        split_master_task(local_matrix_state, global_matrix_state);
        // Master task will have inherently integrated local_matrix_state into global_matrix_state.
        // Therefore, we just assume "changed" needs to be re-evaluated on the global state.
        
        // We set changed = 1 if ANY global state diff exists to trigger a report rebuild.
        changed = 0;
#else
        // Single mode simply copies
        for(int r = 0; r < MATRIX_ROWS; r++){
            for(int c = 0; c < MATRIX_COLS; c++){
                global_matrix_state[r][c] = local_matrix_state[r][c];
            }
        }
#endif
        
        // Check global diff
        for(int r = 0; r < LOGICAL_ROWS; r++){
            for(int c = 0; c < LOGICAL_COLS; c++){
                if (global_matrix_state[r][c] != last_global_matrix_state[r][c]) {
                    changed = 1;
                    if (global_matrix_state[r][c] == 1) {
                        keymap_process_press(r, c, system_millis); // 押された瞬間
#ifdef CUSTOM_RGB_ENABLE
                        rgb_led_notify_keypress(r, c);
#endif
                    } else {
                        keymap_process_release(r, c, system_millis); // 離された瞬間
                    }
                    last_global_matrix_state[r][c] = global_matrix_state[r][c]; // 状態更新
                }
            }
        }

        // Tap/Hold のタップ送出終了など、時間で変化する要素を処理する
        if (keymap_task(system_millis)) {
            changed = 1;
        }

        // 状態が変化した場合、USBレポートを作り直す
        if (changed) {
            // キャッシュ情報から最新のUSBレポートを構築
            keymap_generate_report(current_keyboard_report);
        }
	}
}

// Endpoint 2 (VIA Raw HID) の OUT リクエスト (Remap / Web UI からのコマンド受信)
void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
    if( current_endpoint == 2 )
    {
        // Low-Speed USB では 1 パケット 8 バイトが上限のため、
        // VIA の 32 バイトレポートは via.c 側で組み立てます。
        // rv003usb はこの関数から戻った直後に ACK を返すので、
        // 実際のコマンド処理はメインループの via_task() へ委譲しています。
        via_receive_packet( data, len );
    }
}

// HostからのINリクエストに対する応答 (ポーリング時)
void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
    // ホストからポーリング要求が来た = USB接続確立とみなす
    usb_configured_flag = 1;

	if( endp == 1 )
	{
		// EP1 Boot Keyboard: キー判定により更新されたレポートを送信
		usb_send_data( current_keyboard_report, 8, 0, sendtok );
	}
	else if( endp == 2 )
	{
		// EP2 VIA Raw HID: 応答待ちがあれば 8 バイトずつ返す
		via_handle_in( e, sendtok );
	}
	else
	{
		// Control endpoint等用
		usb_send_empty( sendtok );
	}
}
