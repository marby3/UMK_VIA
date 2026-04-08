#include "ch32fun.h"
#include "split.h"

#ifdef CUSTOM_SPLIT_ENABLE

static uint8_t split_role = SPLIT_ROLE_UNKNOWN;
static uint8_t is_left_hand = 0;
static uint32_t last_tx_time = 0;

void split_init(void) {
    // 1. Handedness Pin Initialization
#ifdef CUSTOM_HANDEDNESS_PIN
    funPinMode(CUSTOM_HANDEDNESS_PIN, GPIO_CFGLR_IN_PUPD);
    funDigitalWrite(CUSTOM_HANDEDNESS_PIN, 1); // Pull-up mode
    Delay_Us(50);
    
    // If the pin is pulled to GND, it's left side.
    if (funDigitalRead(CUSTOM_HANDEDNESS_PIN) == 0) {
        is_left_hand = 1;
    } else {
        is_left_hand = 0;
    }
#else
    is_left_hand = 1; // 判別不能な場合は安全のためマスターを常に左としておくフォールバック
#endif

    // 2. UART Initialization (USART1 on PD5(Tx)/PD6(Rx))
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;
    
    funPinMode(PD5, GPIO_CFGLR_OUT_10Mhz_AF_PP);
    funPinMode(PD6, GPIO_CFGLR_IN_FLOATING);
    
    USART1->CTLR1 = 0;
    USART1->CTLR2 = 0;
    USART1->CTLR3 = 0;
    USART1->BRR = (FUNCONF_SYSTEM_CORE_CLOCK + 115200 / 2) / 115200;
    USART1->CTLR1 = USART_CTLR1_UE | USART_CTLR1_TE | USART_CTLR1_RE;
}

void split_set_master(uint8_t is_master) {
    if (is_master) {
        split_role = SPLIT_ROLE_MASTER;
    } else {
        split_role = SPLIT_ROLE_SLAVE;
    }
}

uint8_t split_is_master(void) {
    return (split_role == SPLIT_ROLE_MASTER);
}

uint8_t split_is_left(void) {
    return is_left_hand;
}

static void uart_send_byte(uint8_t data) {
    while (!(USART1->STATR & USART_STATR_TXE));
    USART1->DATAR = data;
}

void split_slave_task(uint8_t local_matrix[MATRIX_ROWS][MATRIX_COLS], uint32_t now_ms, uint8_t changed) {
    if (changed || (now_ms - last_tx_time > 20)) {
        uart_send_byte(SPLIT_SYNC_BYTE_1);
        uart_send_byte(SPLIT_SYNC_BYTE_2);
        
        for (int r = 0; r < MATRIX_ROWS; r++) {
            uint8_t col_bits = 0;
            for (int c = 0; c < MATRIX_COLS; c++) {
                if (local_matrix[r][c]) {
                    col_bits |= (1 << (c % 8));
                }
                
                if ((c % 8) == 7 || c == MATRIX_COLS - 1) {
                    uart_send_byte(col_bits);
                    col_bits = 0;
                }
            }
        }
        last_tx_time = now_ms;
    }
}

// Memory block to hold the last received slave matrix state locally
static uint8_t received_slave_state[MATRIX_ROWS][MATRIX_COLS] = {0};

void split_master_task(uint8_t local_matrix[MATRIX_ROWS][MATRIX_COLS], uint8_t global_matrix[LOGICAL_ROWS][LOGICAL_COLS]) {
    static uint8_t sync_state = 0;
    static uint8_t current_row = 0;
    static uint8_t current_byte_idx = 0;
    
    // Read all available bytes
    while(USART1->STATR & USART_STATR_RXNE) {
        uint8_t b = USART1->DATAR;
        
        if (sync_state == 0) {
            if (b == SPLIT_SYNC_BYTE_1) sync_state = 1;
        } else if (sync_state == 1) {
            if (b == SPLIT_SYNC_BYTE_2) {
                sync_state = 2; 
                current_row = 0;
                current_byte_idx = 0;
            } else {
                sync_state = 0;
            }
        } else if (sync_state == 2) {
            uint8_t slave_col_offset = current_byte_idx * 8;
            
            for (int bit = 0; bit < 8; bit++) {
                int slave_c = slave_col_offset + bit;
                if (slave_c >= MATRIX_COLS) break;
                
                received_slave_state[current_row][slave_c] = (b & (1 << bit)) ? 1 : 0;
            }
            
            current_byte_idx++;
            if (slave_col_offset + 8 >= MATRIX_COLS) {
                current_row++;
                current_byte_idx = 0;
            }
            if (current_row >= MATRIX_ROWS) {
                sync_state = 0; // Completed one full frame
            }
        }
    }
    
    // Combine local_matrix and received_slave_state into global_matrix
    for (int r = 0; r < MATRIX_ROWS; r++) {
        for (int c = 0; c < MATRIX_COLS; c++) {
            
            uint8_t l_state = local_matrix[r][c];
            uint8_t r_state = received_slave_state[r][c];
            
            int master_r = r;
            int master_c = c;
            int slave_r = r;
            int slave_c = c;
            
            // Map the layout. Default assumes Master Left, Slave Right.
#ifdef CUSTOM_SPLIT_COMBINE_COLS
            if (is_left_hand) {
                slave_c += MATRIX_COLS; 
            } else {
                master_c += MATRIX_COLS;
            }
#elif defined(CUSTOM_SPLIT_COMBINE_ROWS)
            if (is_left_hand) {
                slave_r += MATRIX_ROWS; 
            } else {
                master_r += MATRIX_ROWS;
            }
#endif

            // Safely write to global matrix
            if (master_r < LOGICAL_ROWS && master_c < LOGICAL_COLS) {
                global_matrix[master_r][master_c] = l_state;
            }
            if (slave_r < LOGICAL_ROWS && slave_c < LOGICAL_COLS) {
                global_matrix[slave_r][slave_c] = r_state;
            }
        }
    }
}

#endif
