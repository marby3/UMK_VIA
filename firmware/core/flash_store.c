/* UMK_VIA - keymap persistence in the tail of internal flash. */

#include <stdint.h>
#include "ch32fun.h"
#include "flash_store.h"
#include "keymap.h"

static const uint8_t store_magic[4] = { 'V', 'K', 'M', '1' };

static uint8_t  store_dirty;
static uint8_t  store_save_now;
static uint32_t store_last_activity;

/* Absolute symbol carrying the reserved byte count, so tools/flash_guard.py
 * can check after linking that .text has not run into the store. */
#define UMK_STRINGIFY_(x) #x
#define UMK_STRINGIFY(x)  UMK_STRINGIFY_(x)
__asm__(".globl __umk_flash_store_reserved\n"
        ".set __umk_flash_store_reserved, "
        UMK_STRINGIFY(FLASH_PAGES *FLASH_PAGE_BYTES) "\n");

void flash_store_mark_dirty(void)
{
	store_dirty         = 1;
	store_last_activity = timer_ms;
}

void flash_store_note_activity(void)
{
	store_last_activity = timer_ms;
}

void flash_store_request_save(void)
{
	store_save_now = 1;
}

void flash_store_load(void)
{
	const uint8_t *p = (const uint8_t *)FLASH_OFFSET;

	/* Only trust the image when the geometry matches exactly - a keymap saved
	 * under a different layer/row/col count would be read as garbage. */
	if (p[0] == store_magic[0] && p[1] == store_magic[1] &&
	    p[2] == store_magic[2] && p[3] == store_magic[3] &&
	    p[4] == LAYERS && p[5] == LOGICAL_ROWS && p[6] == LOGICAL_COLS) {
		uint8_t *dst = (uint8_t *)current_keymap;
		const uint8_t *src = p + FLASH_STORE_HEADER_BYTES;
		for (uint32_t i = 0; i < KEYMAP_BYTES; i++) {
			dst[i] = src[i];
		}
	} else {
		keymap_reset_to_default();
	}

	store_dirty         = 0;
	store_save_now      = 0;
	store_last_activity = 0;
}

/* Byte `i` of the on-flash image: header first, then the raw keymap. */
static uint8_t store_byte_at(uint32_t i)
{
	if (i < 4) {
		return store_magic[i];
	}
	if (i == 4) {
		return LAYERS;
	}
	if (i == 5) {
		return LOGICAL_ROWS;
	}
	if (i == 6) {
		return LOGICAL_COLS;
	}
	if (i == 7) {
		return 0; /* reserved */
	}

	uint32_t k = i - FLASH_STORE_HEADER_BYTES;
	if (k >= KEYMAP_BYTES) {
		return 0xFF;
	}
	return ((const uint8_t *)current_keymap)[k];
}

static void flash_store_write(void)
{
	FLASH->KEYR     = FLASH_KEY1;
	FLASH->KEYR     = FLASH_KEY2;
	FLASH->MODEKEYR = FLASH_KEY1;
	FLASH->MODEKEYR = FLASH_KEY2;

	if (FLASH->CTLR & 0x8080) {
		return; /* still locked - give up rather than hang */
	}

	for (uint32_t page = 0; page < FLASH_PAGES; page++) {
		uint32_t addr = FLASH_OFFSET + page * FLASH_PAGE_BYTES;
		volatile uint32_t *dst = (volatile uint32_t *)addr;

		uint32_t words[FLASH_PAGE_BYTES / 4];
		uint8_t  page_differs = 0;
		for (uint32_t w = 0; w < FLASH_PAGE_BYTES / 4; w++) {
			uint32_t base = page * FLASH_PAGE_BYTES + w * 4;
			words[w] = (uint32_t)store_byte_at(base) |
			           ((uint32_t)store_byte_at(base + 1) << 8) |
			           ((uint32_t)store_byte_at(base + 2) << 16) |
			           ((uint32_t)store_byte_at(base + 3) << 24);
			if (dst[w] != words[w]) {
				page_differs = 1;
			}
		}

		/* Erase+program stalls the core for milliseconds and the software USB
		 * interrupt cannot run during that, so the host sees the device go
		 * quiet. Editing one keycode only dirties one page, so skipping the
		 * untouched ones keeps that outage as short as possible. */
		if (!page_differs) {
			continue;
		}

		/* Interrupts are re-enabled between pages for the same reason. */
		__disable_irq();

		FLASH->CTLR = CR_PAGE_ER;
		FLASH->ADDR = addr;
		FLASH->CTLR = CR_STRT_Set | CR_PAGE_ER;
		while (FLASH->STATR & FLASH_STATR_BSY) {
		}

		FLASH->CTLR = CR_PAGE_PG;
		FLASH->CTLR = CR_BUF_RST | CR_PAGE_PG;
		FLASH->ADDR = addr;
		while (FLASH->STATR & FLASH_STATR_BSY) {
		}

		for (uint32_t w = 0; w < FLASH_PAGE_BYTES / 4; w++) {
			dst[w]      = words[w];
			FLASH->CTLR = CR_PAGE_PG | FLASH_CTLR_BUF_LOAD;
			while (FLASH->STATR & FLASH_STATR_BSY) {
			}
		}

		FLASH->CTLR = CR_PAGE_PG | CR_STRT_Set;
		while (FLASH->STATR & FLASH_STATR_BSY) {
		}

		__enable_irq();
	}

	FLASH->CTLR = CR_LOCK_Set;
}

void flash_store_task(void)
{
	if (store_save_now) {
		store_save_now = 0;
		store_dirty    = 0;
		flash_store_write();
		return;
	}

	if (store_dirty &&
	    (uint32_t)(timer_ms - store_last_activity) >= FLASH_AUTOSAVE_DELAY_MS) {
		store_dirty = 0;
		flash_store_write();
	}

	/* Deliberately no reboot after saving: Remap would lose its connection. */
}
