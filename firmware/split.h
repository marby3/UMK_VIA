#ifndef SPLIT_H
#define SPLIT_H

#include <stdint.h>
#include "matrix.h"

#ifdef CUSTOM_CONFIG_H
#include "custom_config.h"
#endif

// Role definitions
#define SPLIT_ROLE_UNKNOWN 0
#define SPLIT_ROLE_MASTER  1
#define SPLIT_ROLE_SLAVE   2

// Serial Packet Marker
#define SPLIT_SYNC_BYTE_1 0xAA
#define SPLIT_SYNC_BYTE_2 0x55

void split_init(void);
void split_set_master(uint8_t is_master);
uint8_t split_is_master(void);
uint8_t split_is_left(void);

// Slave mode behavior: stream to USART1
void split_slave_task(uint8_t local_matrix[MATRIX_ROWS][MATRIX_COLS], uint32_t now_ms, uint8_t changed);

// Master mode behavior: receive from USART1 and merge into global_matrix
void split_master_task(uint8_t local_matrix[MATRIX_ROWS][MATRIX_COLS], uint8_t global_matrix[LOGICAL_ROWS][LOGICAL_COLS]);

#endif
