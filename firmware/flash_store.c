#include "ch32fun.h"
#include "flash_store.h"
#include "keymap.h"
#include <string.h>

static volatile uint8_t  store_dirty;
static volatile uint8_t  store_save_now;
static volatile uint32_t store_dirty_ms;

// 64バイト(=1ページ)を Flash へ書き込む。
// 消去・書き込み中はコアがストールするため、USB(ソフトウェア実装)の割り込みが
// 遅延します。1ページずつ割り込みを開放して影響を最小限にします。
static void flash_write_page(uint32_t *dst, const uint32_t *src) {
    __disable_irq();

    // ページ消去
    FLASH->CTLR = CR_PAGE_ER;
    FLASH->ADDR = (intptr_t)dst;
    FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
    while (FLASH->STATR & FLASH_STATR_BSY);

    // ページ書き込み準備
    FLASH->CTLR = CR_PAGE_PG;
    FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
    FLASH->ADDR = (intptr_t)dst;
    while (FLASH->STATR & FLASH_STATR_BSY);

    // 64バイト分 (uint32_t 16個) をバッファにロード
    for (int i = 0; i < FLASH_PAGE_SIZE / 4; i++) {
        dst[i] = src[i];
        FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
        while (FLASH->STATR & FLASH_STATR_BSY);
    }

    // バッファを実際のフラッシュにコミット
    FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
    while (FLASH->STATR & FLASH_STATR_BSY);

    __enable_irq();
}

void flash_store_load(void) {
    const uint8_t *src = (const uint8_t *)FLASH_OFFSET;
    uint32_t magic;
    memcpy(&magic, src, sizeof(magic));

    // マジックとマトリクス構成が一致した場合のみ復元する。
    // (ビルド設定を変えて書き込み直した直後に、旧レイアウトのデータを
    //  読み込んでしまうのを防ぎます)
    if (magic == FLASH_STORE_MAGIC &&
        src[4] == LAYERS &&
        src[5] == LOGICAL_ROWS &&
        src[6] == LOGICAL_COLS) {
        memcpy(current_keymap, src + FLASH_HEADER_BYTES, sizeof(current_keymap));
        return;
    }

    keymap_reset_to_default();
}

void flash_store_save(void) {
    const uint8_t *keymap_bytes = (const uint8_t *)current_keymap;

    uint8_t header[FLASH_HEADER_BYTES];
    uint32_t magic = FLASH_STORE_MAGIC;
    memcpy(header, &magic, sizeof(magic));
    header[4] = LAYERS;
    header[5] = LOGICAL_ROWS;
    header[6] = LOGICAL_COLS;
    header[7] = 0;

    // Flashの基本ロック解除 (必須)
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;

    // Flashのプログラミング・消去ロック解除 (ch32v003fun)
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;

    for (int page = 0; page < FLASH_PAGES; page++) {
        union {
            uint8_t  b[FLASH_PAGE_SIZE];
            uint32_t w[FLASH_PAGE_SIZE / 4];
        } page_buf;

        for (int i = 0; i < FLASH_PAGE_SIZE; i++) {
            int abs = page * FLASH_PAGE_SIZE + i;
            if (abs < FLASH_HEADER_BYTES) {
                page_buf.b[i] = header[abs];
            } else if (abs - FLASH_HEADER_BYTES < KEYMAP_TOTAL_BYTES) {
                page_buf.b[i] = keymap_bytes[abs - FLASH_HEADER_BYTES];
            } else {
                page_buf.b[i] = 0xFF; // 末尾の余り
            }
        }

        flash_write_page((uint32_t *)(FLASH_OFFSET + (page * FLASH_PAGE_SIZE)), page_buf.w);
    }

    FLASH->CTLR = CR_LOCK_Set;

    store_dirty = 0;
    store_save_now = 0;
}

void flash_store_mark_dirty(uint32_t now_ms) {
    store_dirty = 1;
    store_dirty_ms = now_ms;
}

void flash_store_request_save(void) {
    store_save_now = 1;
}

void flash_store_task(uint32_t now_ms) {
    if (store_save_now) {
        flash_store_save();
        return;
    }

    if (!store_dirty) return;

    // 連続した書き込みコマンドの途中で保存しないよう、一定時間アイドルを待つ
    if ((now_ms - store_dirty_ms) < FLASH_AUTOSAVE_DELAY_MS) return;

    flash_store_save();
}
