/* ch32v003_keyboard - a 2x3 (6 key) macropad on a UIAPduino Pro Micro
 * CH32V003 V1.4, with an SK6812MINI-E under each key.
 *
 * Pins come from the board's KiCad schematic (CH32V003_keyboard.kicad_sch),
 * translated from UIAPduino pad names to CH32V003 GPIOs:
 *   Row0 = pad 12/A3 = PD2      Col0 = pad 9 (MISO) = PC7
 *   Row1 = pad 6/A2  = PC4      Col1 = pad 8 (MOSI) = PC6
 *                               Col2 = pad 10       = PD0
 *   LED data = pad 15/A5 (TX) = PD5
 * Diodes point column -> row (cathode on the row), which is what matrix.c
 * expects: rows are strobed low and columns read through pull-ups.
 */
#ifndef _CONFIG_H
#define _CONFIG_H

#include "ch32fun.h"

/* --- Matrix ------------------------------------------------------------- */
#define CUSTOM_MATRIX_ROWS 2
#define CUSTOM_MATRIX_COLS 3

#define CUSTOM_ROW_PINS { PD2, PC4 }
#define CUSTOM_COL_PINS { PC7, PC6, PD0 }

/* --- Layers ------------------------------------------------------------- */
#define CUSTOM_LAYERS 4

/* --- USB ---------------------------------------------------------------- */
/* Same VID/PID as the uiapduino sample: Remap tells definitions apart by the
 * product name below. D+ = PD3, D- = PD4 with a fixed pull-up on the
 * UIAPduino board, which is the firmware default. */
#define CUSTOM_VID 0x1209
#define CUSTOM_PID 0xB803

#define STR_MANUFACTURER u"UMK"
#define STR_PRODUCT      u"CH32V003_keyboard"
#define STR_SERIAL       u"001"

/* --- RGB ---------------------------------------------------------------- */
/* Left disabled. The LEDs are wired to PD5, but the DMA+SPI WS2812B driver
 * can only drive PC6 (SPI1 MOSI), so enabling it here would stop the build.
 * PC6 is a matrix column on this board anyway. */
/* #define CUSTOM_RGB_ENABLE */

/* --- Timing ------------------------------------------------------------- */
#define TAPPING_TERM_MS         200
#define TAP_REPORT_MS           20
#define FLASH_AUTOSAVE_DELAY_MS 750
#define DEBOUNCE_MS             5

#endif
