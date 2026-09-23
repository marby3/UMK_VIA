#ifndef VIA_H
#define VIA_H

#include <stdint.h>
#include "rv003usb.h"

// VIA プロトコルバージョン (VIA 3 / QMK 0.22 世代)。
// Remap はこの値でキーコードの解釈方法を切り替えます。
#define VIA_PROTOCOL_VERSION 0x000C

// ファームウェアバージョン (VIA の id_firmware_version で返す任意の32bit値)
#ifndef VIA_FIRMWARE_VERSION
#define VIA_FIRMWARE_VERSION 0x00000001
#endif

void via_init(void);

// EP2 OUT (割り込みコンテキスト): ホストからの 8 バイトパケットを
// 32 バイトのレポートへ組み立てるだけ。コマンド処理は via_task() が行います。
void via_receive_packet(const uint8_t *data, int len);

// EP2 IN (割り込みコンテキスト): 応答レポートを 8 バイトずつ送出する。
// 応答が無い場合は必ず空パケットを返します。
void via_handle_in(struct usb_endpoint *e, uint32_t sendtok);

// メインループから毎ミリ秒呼び出す。
// 受信済みコマンドの処理と uptime 用の時刻更新を行います。
void via_task(uint32_t now_ms);

#endif
