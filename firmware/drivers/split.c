/* UMK_VIA - split keyboard link over USART1. */

#include <stdint.h>
#include "ch32fun.h"
#include "split.h"
#include "matrix.h"

#ifdef CUSTOM_SPLIT_ENABLE

uint8_t is_left_hand = 1;

static uint8_t split_is_master;

/* Latest full frame decoded from the far half. */
static matrix_row_t received_slave_state[MATRIX_ROWS];

/* Master receive path: DMA1 Channel 5 fills this circularly, forever. */
#define SPLIT_RX_RING_SIZE 32
static uint8_t split_rx_ring[SPLIT_RX_RING_SIZE];
static uint16_t split_rx_tail;

/* Two-state resync: hunt for 0xAA 0x55, then take the payload verbatim. */
#define SPLIT_ST_SYNC    0
#define SPLIT_ST_PAYLOAD 1
static uint8_t split_rx_state;
static uint8_t split_rx_sync_seen;
static uint8_t split_rx_index;
static uint8_t split_rx_frame[MATRIX_ROWS * SPLIT_ROW_BYTES];

/* Slave transmit path. */
static matrix_row_t split_tx_last[MATRIX_ROWS];
static uint32_t split_tx_last_ms;

void split_init(void)
{
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;

	/* PD5 = USART1_TX (alternate function push-pull), PD6 = USART1_RX.
	 * OUTDR selects the pull direction in IN_PUPD mode, so it has to be set
	 * before the mode switch. UART idles high. */
	funPinMode(PD5, GPIO_CFGLR_OUT_10Mhz_AF_PP);
	funDigitalWrite(PD6, FUN_HIGH);
	funPinMode(PD6, GPIO_CFGLR_IN_PUPD);

	USART1->CTLR1 = 0;
	USART1->CTLR2 = 0;
	USART1->CTLR3 = 0;
	/* 8N1. With 16x oversampling BRR is simply fclk/baud. */
	USART1->BRR   = (FUNCONF_SYSTEM_CORE_CLOCK + SPLIT_BAUD / 2) / SPLIT_BAUD;
	USART1->CTLR1 = USART_CTLR1_TE | USART_CTLR1_RE | USART_CTLR1_UE;

#ifdef CUSTOM_HANDEDNESS_PIN
	funDigitalWrite(CUSTOM_HANDEDNESS_PIN, FUN_HIGH); /* pull-up */
	funPinMode(CUSTOM_HANDEDNESS_PIN, GPIO_CFGLR_IN_PUPD);
	Delay_Us(50);
	is_left_hand = funDigitalRead(CUSTOM_HANDEDNESS_PIN) ? 0 : 1;
#else
	/* No handedness pin: assume the conventional Master=left wiring. */
	is_left_hand = 1;
#endif

	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		received_slave_state[r] = 0;
		split_tx_last[r]        = 0;
	}
	split_rx_state     = SPLIT_ST_SYNC;
	split_rx_sync_seen = 0;
	split_rx_index     = 0;
	split_rx_tail      = 0;
	split_tx_last_ms   = 0;
}

static void split_start_rx_dma(void)
{
	RCC->AHBPCENR |= RCC_AHBPeriph_DMA1;

	DMA1_Channel5->CFGR  = 0;
	DMA1_Channel5->PADDR = (uint32_t)&USART1->DATAR;
	DMA1_Channel5->MADDR = (uint32_t)split_rx_ring;
	DMA1_Channel5->CNTR  = SPLIT_RX_RING_SIZE;
	DMA1_Channel5->CFGR  = DMA_M2M_Disable |
	                       DMA_Priority_High |
	                       DMA_MemoryDataSize_Byte |
	                       DMA_PeripheralDataSize_Byte |
	                       DMA_MemoryInc_Enable |
	                       DMA_Mode_Circular |
	                       DMA_DIR_PeripheralSRC;
	DMA1_Channel5->CFGR |= DMA_CFGR1_EN;

	USART1->CTLR3 |= USART_CTLR3_DMAR;
}

void split_set_master(uint8_t is_master)
{
	split_is_master = is_master;
	if (is_master) {
		split_start_rx_dma();
	}
}

/* ------------------------------------------------------------------------ */
/* Slave                                                                     */
/* ------------------------------------------------------------------------ */

static void split_tx_byte(uint8_t b)
{
	while (!(USART1->STATR & USART_STATR_TXE)) {
	}
	USART1->DATAR = b;
}

void split_slave_task(void)
{
	uint8_t changed = 0;
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		if (split_tx_last[r] != local_matrix_state[r]) {
			changed = 1;
			break;
		}
	}

	if (!changed &&
	    (uint32_t)(timer_ms - split_tx_last_ms) < SPLIT_SLAVE_IDLE_MS) {
		return;
	}

	split_tx_byte(SPLIT_SYNC0);
	split_tx_byte(SPLIT_SYNC1);
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		matrix_row_t state = local_matrix_state[r];
		split_tx_last[r]   = state;
		for (uint8_t b = 0; b < SPLIT_ROW_BYTES; b++) {
			split_tx_byte((uint8_t)(state >> (8 * b)));
		}
	}

	split_tx_last_ms = timer_ms;
}

/* ------------------------------------------------------------------------ */
/* Master                                                                    */
/* ------------------------------------------------------------------------ */

static void split_rx_feed(uint8_t b)
{
	if (split_rx_state == SPLIT_ST_SYNC) {
		if (!split_rx_sync_seen) {
			split_rx_sync_seen = (b == SPLIT_SYNC0);
		} else if (b == SPLIT_SYNC1) {
			split_rx_sync_seen = 0;
			split_rx_state     = SPLIT_ST_PAYLOAD;
			split_rx_index     = 0;
		} else {
			/* Could be the start of a new sync pair. */
			split_rx_sync_seen = (b == SPLIT_SYNC0);
		}
		return;
	}

	split_rx_frame[split_rx_index++] = b;
	if (split_rx_index < sizeof(split_rx_frame)) {
		return;
	}

	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		matrix_row_t state = 0;
		for (uint8_t b2 = 0; b2 < SPLIT_ROW_BYTES; b2++) {
			state |= (matrix_row_t)split_rx_frame[r * SPLIT_ROW_BYTES + b2]
			         << (8 * b2);
		}
		received_slave_state[r] = state;
	}

	split_rx_state = SPLIT_ST_SYNC;
}

void split_master_task(void)
{
	uint16_t head = (uint16_t)(SPLIT_RX_RING_SIZE - DMA1_Channel5->CNTR);
	while (split_rx_tail != head) {
		split_rx_feed(split_rx_ring[split_rx_tail]);
		split_rx_tail = (uint16_t)((split_rx_tail + 1) % SPLIT_RX_RING_SIZE);
	}

	/* Merge. is_left_hand decides which side lands at offset 0. */
	const matrix_row_t *own   = local_matrix_state;
	const matrix_row_t *other = received_slave_state;

#ifdef CUSTOM_SPLIT_COMBINE_ROWS
	uint8_t own_row_off   = is_left_hand ? 0 : MATRIX_ROWS;
	uint8_t other_row_off = is_left_hand ? MATRIX_ROWS : 0;
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		global_matrix_state[own_row_off + r]   = own[r];
		global_matrix_state[other_row_off + r] = other[r];
	}
#else /* CUSTOM_SPLIT_COMBINE_COLS */
	uint8_t own_shift   = is_left_hand ? 0 : MATRIX_COLS;
	uint8_t other_shift = is_left_hand ? MATRIX_COLS : 0;
	for (uint8_t r = 0; r < MATRIX_ROWS; r++) {
		global_matrix_state[r] = (matrix_row_t)(((matrix_row_t)own[r] << own_shift) |
		                                        ((matrix_row_t)other[r] << other_shift));
	}
#endif
}

#endif /* CUSTOM_SPLIT_ENABLE */
