/* UMK_VIA - split keyboard link over USART1.
 *
 * Wiring is fixed: Tx = PD5, Rx = PD6, 115200 8N1, one direction only
 * (Slave -> Master). The Master receives through DMA1 Channel 5 into a ring
 * buffer; polling the USART once per 1ms tick would drop bytes, and an RX
 * interrupt would fight the software USB for timing.
 */
#ifndef _UMK_SPLIT_H
#define _UMK_SPLIT_H

#include <stdint.h>
#include "matrix.h"

#define SPLIT_BAUD           115200
#define SPLIT_SYNC0          0xAA
#define SPLIT_SYNC1          0x55
#define SPLIT_SLAVE_IDLE_MS  20 /* resend even without changes */

void split_init(void);

/* Called by main.c once the USB role has been decided. */
void split_set_master(uint8_t is_master);

/* Slave: transmits local_matrix_state when it changed or the idle timer
 * elapsed. */
void split_slave_task(void);

/* Master: drains the DMA ring and merges the far half into
 * global_matrix_state together with local_matrix_state. */
void split_master_task(void);

/* 1 when this half is the left hand side. Determined from
 * CUSTOM_HANDEDNESS_PIN, defaulting to left when no pin is configured. */
extern uint8_t is_left_hand;

#endif
