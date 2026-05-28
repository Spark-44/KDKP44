#ifndef _SCREEN_H_
#define _SCREEN_H_

#include "zf_common_headfile.h"
#include "zf_device_dot_matrix_screen.h"

#define CI_CMD_WAKEUP               (0x01)
#define CI_CMD_WELCOME              (0x02)
#define CI_CMD_SLEEP                (0x03)

#define CI_CMD_TURN_LEFT            (0x04)  
#define CI_CMD_TURN_RIGHT           (0x05)  
#define CI_CMD_HIGH_BEAM            (0x06)
#define CI_CMD_LOW_BEAM             (0x07)
#define CI_CMD_FOG_LIGHT            (0x08)
#define CI_CMD_DOUBLE_FLASH         (0x09)
#define CI_CMD_INTERIOR_LIGHT       (0x0A)

#define CI_CMD_WIPER                (0x0B)  

#define CI_CMD_HORN_1S              (0x0C)  
#define CI_CMD_HORN_2S              (0x0D)
#define CI_CMD_HORN_3S              (0x0E)
#define CI_CMD_HORN_2X              (0x0F)
#define CI_CMD_HORN_3X              (0x10)  
#define CI_CMD_HORN_4X              (0x11)  
#define CI_CMD_HORN_LONG_SHORT      (0x12)  
#define CI_CMD_HORN_RAPID           (0x13)  
#define CI_CMD_HORN_ALARM           (0x14)  

#define CI_CMD_GATE1_LEFT           (0x15)  
#define CI_CMD_GATE1                (0x16)  
#define CI_CMD_GATE2                (0x17)  
#define CI_CMD_GATE3                (0x18)  
#define CI_CMD_GATE3_RIGHT          (0x19)  

#define CI_CMD_GATE1_RIGHT_BACK     (0x1A)  
#define CI_CMD_GATE1_BACK           (0x1B)  
#define CI_CMD_GATE2_BACK           (0x1C)  
#define CI_CMD_GATE3_BACK           (0x1D)  
#define CI_CMD_GATE3_LEFT_BACK      (0x1E)  

#define CI_CMD_FORWARD_10M          (0x1F)
#define CI_CMD_BACKWARD_10M         (0x20)
#define CI_CMD_SNAKE_FORWARD        (0x21)
#define CI_CMD_SNAKE_BACKWARD       (0x22)
#define CI_CMD_CCW_CIRCLE           (0x23)
#define CI_CMD_CW_CIRCLE            (0x24)
#define CI_CMD_TURN_LEFT_DRIVE      (0x25)
#define CI_CMD_TURN_RIGHT_DRIVE     (0x26)  

#define CI_CMD_MAX                  (0x26)

void screen_init(void);

void screen_poll(void);

void screen_uart_rx_handler(void);

uint8 screen_get_last_cmd(void);

#endif
