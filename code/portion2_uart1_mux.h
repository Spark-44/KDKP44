#ifndef CODE_PORTION2_UART1_MUX_H_
#define CODE_PORTION2_UART1_MUX_H_

#include "zf_common_headfile.h"

typedef enum
{
    PORTION2_UART1_OWNER_NONE = 0,
    PORTION2_UART1_OWNER_RECORD_RECEIVER,
    PORTION2_UART1_OWNER_RUN_DOT_MATRIX
} portion2_uart1_owner_t;

void portion2_uart1_select_record_receiver(void);
void portion2_uart1_select_run_dot_matrix(void);
void portion2_uart1_update_for_mode(Mode_Choice mode);
void portion2_uart1_rx_isr_handler(void);
void portion2_uart1_error_isr_handler(void);
portion2_uart1_owner_t portion2_uart1_get_owner(void);

#endif
