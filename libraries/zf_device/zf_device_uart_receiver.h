/*********************************************************************************************************************
* TC264 Opensourec Library
* File: zf_device_uart_receiver
********************************************************************************************************************/
#ifndef _zf_device_uart_receiver_h
#define _zf_device_uart_receiver_h

#include "zf_common_typedef.h"

#define UART_RECEVIER_UART_INDEX            (UART_2)            // SBUS receiver UART
#define UART_RECEVIER_TX_PIN                (UART2_TX_P10_5)    // Placeholder TX pin for UART init
#define UART_RECEVIER_RX_PIN                (UART2_RX_P10_6)    // Receiver TX connects to MCU RX
#define SBUS_UART_BAUDRATE                  (100000)            // SBUS fixed baudrate
#define UART_RECEVIER_CHANNEL_NUM           ( 6 )               // Remote channel count

#define REV_DATA_LEN                        ( 25   )            // SBUS frame length
#define FRAME_STAR                          ( 0X0F )            // SBUS frame header
#define FRAME_END                           ( 0X00 )            // SBUS frame tail
#define SBUS_ABNORMAL_STATE                 ( 0X04 )            // Failsafe/lost frame state

typedef struct
{
    volatile uint16 channel[UART_RECEVIER_CHANNEL_NUM];
    volatile uint8  state;                                      // 1 normal, 0 failsafe/lost
    volatile uint8  finsh_flag;                                 // 1 when one full frame is received
    volatile uint32 frame_count;                                 // accepted SBUS frames
    volatile uint32 drop_count;                                  // bytes discarded while waiting for header
    volatile uint32 resync_count;                                // parser resync events
    volatile uint32 uart_error_count;                            // UART hardware error events
} uart_receiver_struct;

extern uart_receiver_struct uart_receiver;

void uart_receiver_init(void);
void uart_receiver_reset_parser(void);
void uart_receiver_note_uart_error(void);

#endif
