

#ifndef _zf_device_tld7002_h_
#define _zf_device_tld7002_h_

#include "zf_common_typedef.h"

#define TLD7002_UART_INDEX      (UART_1)            
#define TLD7002_UART_BAUD       (2000000)           
#define TLD7002_UART_RX         (UART1_TX_P33_12)   
#define TLD7002_UART_HLSIL      (UART1_RX_P33_13)   

#define TLD7002_GPIN0_PIN       (P32_4)             

extern uint16 tld7002_duty[16];

void    tld7002_set_duty        (uint8 tld7002_id);
void    tld7002_callback        (void);
void    tld7002_init            (void);

#endif
