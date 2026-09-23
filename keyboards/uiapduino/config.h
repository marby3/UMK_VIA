/* uiapduino - a 4x6 (24 key) ortholinear macropad-style board.
 *
 * GENERATED FILE. build_server.py rewrites this from the Web UI's Hardware
 * Config on every build - edit it there, not here, unless you are building
 * from the umk CLI only.
 */
#ifndef _CONFIG_H
#define _CONFIG_H

#include "ch32fun.h"

/* --- Matrix ------------------------------------------------------------- */
/* Pins that are NOT available to the matrix on a CH32V003:
 *   PD3, PD4  USB D+/D-        (CUSTOM_USB_PIN_DP / _DM)
 *   PD1       SWIO debug pin
 *   PD5, PD6  split USART1 Tx/Rx, when CUSTOM_SPLIT_ENABLE is set
 *   PC6       WS2812B data,     when CUSTOM_RGB_ENABLE is set
 *   PD7       NRST unless the option byte reassigns it
 * That leaves PA1, PA2, PC0-PC5, PC7, PD0, PD2 for a stock build. */
#define CUSTOM_MATRIX_ROWS 4
#define CUSTOM_MATRIX_COLS 6

#define CUSTOM_ROW_PINS { PD0, PD2, PA1, PA2 }
#define CUSTOM_COL_PINS { PC0, PC1, PC2, PC3, PC4, PC5 }

/* Direct pin mode: no row pins, one switch per column pin. */
/* #define CUSTOM_DIRECT_PIN_MODE */

/* --- Layers ------------------------------------------------------------- */
#define CUSTOM_LAYERS 4

/* Leave unassigned cells on layers >= 1 as KC_NO instead of KC_TRNS. */
/* #define CUSTOM_NO_TRNS_DEFAULT */

/* --- USB ---------------------------------------------------------------- */
#define CUSTOM_VID 0x1209
#define CUSTOM_PID 0xB803

#define STR_MANUFACTURER u"UMK"
#define STR_PRODUCT      u"uiapduino"
#define STR_SERIAL       u"001"

/* D+ = PD3, D- = PD4 by default; override only if the board differs. */
/* #define CUSTOM_USB_PORT   D */
/* #define CUSTOM_USB_PIN_DP 3 */
/* #define CUSTOM_USB_PIN_DM 4 */

/* --- Split -------------------------------------------------------------- */
/* Tx/Rx are fixed at PD5/PD6, 115200 baud. */
/* #define CUSTOM_SPLIT_ENABLE */
/* #define CUSTOM_SPLIT_COMBINE_COLS */   /* default: LOGICAL_COLS = COLS*2 */
/* #define CUSTOM_SPLIT_COMBINE_ROWS */   /*          LOGICAL_ROWS = ROWS*2 */
/* #define CUSTOM_HANDEDNESS_PIN PC7 */   /* low = left hand */

/* --- RGB ---------------------------------------------------------------- */
/* CUSTOM_RGB_PIN must be PC6: the DMA+SPI WS2812B driver only drives SPI1
 * MOSI. See firmware/rgb_led.c. */
/* #define CUSTOM_RGB_ENABLE */
/* #define CUSTOM_RGB_PIN      PC6 */
/* #define CUSTOM_RGB_NUM_LEDS 8 */
/* #define CUSTOM_RGB_MODE     0 */       /* 0 rainbow 1 static 2 breathe 3 reactive */

/* --- Timing ------------------------------------------------------------- */
#define TAPPING_TERM_MS         200
#define TAP_REPORT_MS           20
#define FLASH_AUTOSAVE_DELAY_MS 750
#define DEBOUNCE_MS             5

#endif
