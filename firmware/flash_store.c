#include "ch32fun.h"
#include "flash_store.h"
#include "keymap.h"
#include <string.h>

// RAMに展開されている現在キーマップ (192 bytes)
// uint16_t current_keymap[LAYERS][MATRIX_ROWS][MATRIX_COLS];

void flash_store_load(void) {
    uint32_t *src = (uint32_t *)FLASH_OFFSET;
    
    // マジックワード等のチェックを実装する場合もありますが、
    // 今回は単純にオールF(未書き込み)の場合はロードをスキップし、初期配列を利用します。
    if (src[0] != 0xFFFFFFFF) {
        memcpy(current_keymap, (void *)FLASH_OFFSET, sizeof(current_keymap));
    }
}

void flash_store_save_and_reboot(void) {
    // 割り込み等を全停止して安全に書き込む
    __disable_irq();

    // Flashの基本ロック解除 (必須)
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;

    // Flashのプログラミング・消去ロック解除 (ch32v003fun)
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;

    uint32_t *src_data = (uint32_t *)current_keymap;

    // 192バイトを 64バイト(16x uint32) のページ x3 に分けて書き込む
    for (int page = 0; page < FLASH_PAGES; page++) {
        uint32_t *ptr = (uint32_t *)(FLASH_OFFSET + (page * 64));

        // ページ消去
        FLASH->CTLR = CR_PAGE_ER;
        FLASH->ADDR = (intptr_t)ptr;
        FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
        while( FLASH->STATR & FLASH_STATR_BSY );

        // ページ書き込み準備
        FLASH->CTLR = CR_PAGE_PG;
        FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
        FLASH->ADDR = (intptr_t)ptr;
        while( FLASH->STATR & FLASH_STATR_BSY );

        // 64バイト分 (uint32_t 16個) をバッファにロード
        for (int i = 0; i < 16; i++) {
            ptr[i] = src_data[(page * 16) + i];
            FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
            while( FLASH->STATR & FLASH_STATR_BSY );
        }

        // バッファを実際のフラッシュにコミット
        FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
        while( FLASH->STATR & FLASH_STATR_BSY );
    }

    // 書込完了後、ソフトウェアリセットで再起動し、新しいキーマップでUSBを再列挙する
    NVIC_SystemReset();
}
