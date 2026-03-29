#include "ch32fun.h"
#include <stdio.h>
#include <string.h>
#include "rv003usb.h"
#include "matrix.h"
#include "keymap.h"
#include "flash_store.h"

// 状態管理
uint8_t matrix_state[MATRIX_ROWS][MATRIX_COLS] = {0};

// キーボード用とカスタムHID用のレポートバッファ
uint8_t current_keyboard_report[8] = { 0 };
uint8_t current_custom_report[8] = { 0 };

volatile uint8_t pending_flash_save = 0;

int main()
{
	SystemInit();
	Delay_Ms(1); // USB再認識用のディレイ
    
    // Phase 4: 起動時に保存されたキー配列があればRAMへロードする
    flash_store_load();

    // Phase 2: ピンの初期化
    matrix_init();
    
    // Phase 1: USBセットアップ
	usb_setup();

	uint32_t system_millis = 0;

	while(1)
	{
		Delay_Ms(1);
		system_millis++;

        if (pending_flash_save) {
            flash_store_save_and_reboot();
            pending_flash_save = 0; // 実際には再起動するのでここは到達しません
        }

        // Phase 2: マトリックススキャン
        uint8_t changed = matrix_scan(matrix_state, system_millis);
        
        // 状態が変化した場合、USBレポートを作り直す
        if (changed) {
            keymap_generate_report(matrix_state, current_keyboard_report);
        }
	}
}

// Endpoint 2 (Custom/Raw HID) の OUT リクエスト (PC等からのコマンド受信)
void usb_handle_user_data( struct usb_endpoint * e, int current_endpoint, uint8_t * data, int len, struct rv003usb_internal * ist )
{
    // 送信されてきたペイロードが8バイト以上あるか確認
    if (len >= 8) {
        uint8_t command_id = data[0];
        
        // 0x01: キー書き込みコマンド
        if (command_id == 0x01) {
            uint8_t layer = data[1];
            uint8_t row   = data[2];
            uint8_t col   = data[3];
            uint16_t keycode = ((uint16_t)data[4] << 8) | data[5];
            
            // 境界チェック (不正な配列アクセス防止)
            if (layer < LAYERS && row < MATRIX_ROWS && col < MATRIX_COLS) {
                current_keymap[layer][row][col] = keycode;
            }
        }
        // 0x02: キー読み出しコマンド
        else if (command_id == 0x02) {
            uint8_t layer = data[1];
            uint8_t row   = data[2];
            uint8_t col   = data[3];
            
            if (layer < LAYERS && row < MATRIX_ROWS && col < MATRIX_COLS) {
                uint16_t key = current_keymap[layer][row][col];
                
                // EP2 INエンドポイントでポーリングされた際にこれを返す
                current_custom_report[0] = 0x02;
                current_custom_report[1] = layer;
                current_custom_report[2] = row;
                current_custom_report[3] = col;
                current_custom_report[4] = (key >> 8) & 0xFF;
                current_custom_report[5] = key & 0xFF;
                current_custom_report[6] = 0x00;
                current_custom_report[7] = 0x00;
            }
        }
        // Phase 4: フラッシュ保存処理
        else if (command_id == 0x99) {
            // コールバック内（割り込み処理中）でFlash操作を行うとフリーズする危険があるため、メインループへ処理を委譲します
            pending_flash_save = 1;
        }
    }
}

// HostからのINリクエストに対する応答 (ポーリング時)
void usb_handle_user_in_request( struct usb_endpoint * e, uint8_t * scratchpad, int endp, uint32_t sendtok, struct rv003usb_internal * ist )
{
	if( endp == 1 )
	{
		// EP1 Boot Keyboard: キー判定により更新されたレポートを送信
		usb_send_data( current_keyboard_report, 8, 0, sendtok );
	}
	else if( endp == 2 )
	{
		// EP2 Custom HID: 現在のカスタムレポートを返す (未使用時はゼロ)
		usb_send_data( current_custom_report, 8, 0, sendtok );
	}
	else
	{
		// Control endpoint等用
		usb_send_empty( sendtok );
	}
}
