#ifndef FLASH_STORE_H
#define FLASH_STORE_H

#include <stdint.h>

// Flash保存先 (16KB Flash = 0x08000000 - 0x08003FFF)
// 192 bytesの保存領域 (64byte/page x 3pages)
// 0x08003FFF から 192 バイト逆算した 0x08003F40 から使用する
#define FLASH_OFFSET 0x08003F40
#define FLASH_PAGES  3

// RAMのキーマップデータをFlashに書き込み、マイコンをリセットする
void flash_store_save_and_reboot(void);

// 起動時にFlashからRAMへキーマップを復元する
void flash_store_load(void);

#endif
