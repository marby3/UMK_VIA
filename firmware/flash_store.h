#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

#include "keymap.h"

// Flash保存先 (16KB Flash = 0x08000000 - 0x08003FFF)
// 64byte/page 単位で保存。必要なページ数をマトリクスから動的に計算。
// LAYERS=4 * LOGICAL_ROWS * LOGICAL_COLS * 2 byte (uint16)
#define KEYMAP_TOTAL_BYTES (LAYERS * LOGICAL_ROWS * LOGICAL_COLS * 2)

#define FLASH_PAGES  ((KEYMAP_TOTAL_BYTES + 63) / 64)
#define FLASH_OFFSET (0x08004000 - (FLASH_PAGES * 64))

// RAMのキーマップデータをFlashに書き込み、マイコンをリセットする
void flash_store_save_and_reboot(void);

// 起動時にFlashからRAMへキーマップを復元する
void flash_store_load(void);

#endif
