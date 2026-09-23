#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

#include "keymap.h"

// 内蔵Flash上のキーマップ保存フォーマット
//   +0 : magic  (4 bytes)
//   +4 : layers / logical rows / logical cols / reserved (各1 byte)
//   +8 : キーマップ本体 (uint16 のリトルエンディアン配列)
#define FLASH_STORE_MAGIC  0x314D4B56  // "VKM1"
#define FLASH_HEADER_BYTES 8

#define KEYMAP_TOTAL_BYTES (LAYERS * LOGICAL_ROWS * LOGICAL_COLS * 2)

// CH32V003 の高速書き込み単位は 64 バイト
#define FLASH_PAGE_SIZE 64
#define FLASH_PAGES     ((FLASH_HEADER_BYTES + KEYMAP_TOTAL_BYTES + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE)

// 16KB Flash (0x08000000 - 0x08003FFF) の末尾を保存領域として使用します。
// ファームウェア本体が大きくなるとここに衝突するため、リンク結果 (.text サイズ) に
// 注意してください。
#define FLASH_OFFSET (0x08004000 - (FLASH_PAGES * FLASH_PAGE_SIZE))

// VIA でキーマップが書き換えられた後、この時間だけ通信が途切れたら自動保存する
#ifndef FLASH_AUTOSAVE_DELAY_MS
#define FLASH_AUTOSAVE_DELAY_MS 750
#endif

// 起動時にFlashからRAMへキーマップを復元する。
// 保存データが無い / 構成が変わっている場合は default_keymap で初期化します。
void flash_store_load(void);

// RAMのキーマップを即座にFlashへ書き込む (再起動はしません)
void flash_store_save(void);

// キーマップが変更されたことを記録し、遅延自動保存を予約する
void flash_store_mark_dirty(uint32_t now_ms);

// 次の flash_store_task() で即座に保存させる。
// USB割り込み内から Flash 操作を行うと通信が壊れるため、実処理はメインループに
// 委譲します。
void flash_store_request_save(void);

// メインループから毎ミリ秒呼び出す。保存タイミングになったら書き込みます。
void flash_store_task(uint32_t now_ms);

#endif
