/* UMK_VIA - keymap persistence in the tail of internal flash.
 *
 * There is no EEPROM on the CH32V003, so the last pages of the 16KB flash are
 * reclaimed as storage. That region is NOT reserved by the linker: if .text
 * grows into it the keymap is silently clobbered, which is why the build runs
 * tools/flash_guard.py after linking.
 */
#ifndef _UMK_FLASH_STORE_H
#define _UMK_FLASH_STORE_H

#include <stdint.h>
#include "board_config.h"

/* magic(4) + layers(1) + rows(1) + cols(1) + reserved(1) + keymap */
#define FLASH_STORE_HEADER_BYTES 8
#define FLASH_STORE_BYTES        (FLASH_STORE_HEADER_BYTES + KEYMAP_BYTES)

#define FLASH_PAGE_BYTES 64
#define FLASH_PAGES      ((FLASH_STORE_BYTES + FLASH_PAGE_BYTES - 1) / FLASH_PAGE_BYTES)
#define FLASH_END        0x08004000
#define FLASH_OFFSET     (FLASH_END - (FLASH_PAGES * FLASH_PAGE_BYTES))

/* Restores a saved keymap, or falls back to default_keymap when the stored
 * geometry does not match this build. */
void flash_store_load(void);

/* Main loop hook: performs a pending save once the bus has gone quiet. */
void flash_store_task(void);

/* VIA changed the keymap - arm the delayed autosave. */
void flash_store_mark_dirty(void);

/* Any VIA traffic - push the autosave deadline back. */
void flash_store_note_activity(void);

/* VIA set_keyboard_value 0xFF - save on the very next flash_store_task(). */
void flash_store_request_save(void);

#endif
