/* UMK_VIA - ch32v003fun global build configuration.
 *
 * This file is picked up by ch32fun.c and (indirectly) by rv003usb.
 * Keyboard specific knobs do NOT belong here - put them in
 * keyboards/<name>/config.h as CUSTOM_* macros.
 */
#ifndef _FUNCONFIG_H
#define _FUNCONFIG_H

/* rv003usb bit-bangs USB from the SysTick counter, so SysTick has to run at
 * the full core clock. rv003usb.h #errors out if this is not set. */
#define FUNCONF_SYSTICK_USE_HCLK 1

/* No printf support: every byte of flash counts on a 16KB part. */
#define FUNCONF_USE_DEBUGPRINTF 0
#define FUNCONF_USE_UARTPRINTF  0

#endif
