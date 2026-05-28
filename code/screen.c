#include "screen.h"

#pragma section all "cpu0_dsram"

static uint8  rx_buf[64];
static uint8  rx_tmp[64];
static fifo_struct rx_fifo;

#define FRAME_LEN       8
#define FRAME_HEAD0     0x6B
#define FRAME_HEAD1_RX  0x79    
#define FRAME_HEAD1_TX  0x6A    
#define FRAME_MSG_RX    0x81
#define FRAME_MSG_TX    0x82
#define FRAME_TAIL      0xFB

typedef enum
{
    STATE_IDLE = 0,     
    STATE_HEAD1,        
    STATE_DATA          
} parse_state_t;

static parse_state_t parse_state = STATE_IDLE;
static uint8 frame_buf[FRAME_LEN];
static uint8 frame_pos;

static uint8 last_cmd_id = 0;   

#pragma section all restore

static void ci1302_send_response(uint8 cmd_id);
static void ci1302_execute_cmd(uint8 cmd_id);
static void parse_byte(uint8 byte);

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
static void ci1302_send_response(uint8 cmd_id)
{
    uint8 resp[FRAME_LEN];
    resp[0] = FRAME_HEAD0;
    resp[1] = FRAME_HEAD1_TX;
    resp[2] = 0x00;
    resp[3] = FRAME_MSG_TX;
    resp[4] = cmd_id;
    resp[5] = 0x00;
    resp[6] = (resp[0] + resp[1] + resp[2] + resp[3] + resp[4] + resp[5]) & 0xFF;
    resp[7] = FRAME_TAIL;

    uint8 i;
    for(i = 0; i < FRAME_LEN; i++)
    {
        uart_write_byte(DEBUG_UART_INDEX, resp[i]);
    }
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
static void ci1302_execute_cmd(uint8 cmd_id)
{
    last_cmd_id = cmd_id;

    switch(cmd_id)
    {
        
        case CI_CMD_TURN_LEFT:
            dot_matrix_screen_set_brightness(5000);
            dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_TURN_LEFT);
            break;

        case CI_CMD_TURN_RIGHT:
            dot_matrix_screen_set_brightness(5000);
            dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_TURN_RIGHT);
            break;

        case CI_CMD_HIGH_BEAM:
            dot_matrix_screen_set_brightness(5000);
            dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_HIGH_BEAM);
            break;

        case CI_CMD_LOW_BEAM:
            dot_matrix_screen_set_brightness(5000);
            dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_LOW_BEAM);
            break;

        case CI_CMD_FOG_LIGHT:
            dot_matrix_screen_set_brightness(5000);
            dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_FOG_LIGHT);
            break;

        case CI_CMD_DOUBLE_FLASH:
            dot_matrix_screen_set_brightness(5000);
            dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_DOUBLE_FLASH);
            break;

        case CI_CMD_INTERIOR_LIGHT:
            dot_matrix_screen_set_brightness(10000);
            dot_matrix_screen_show_string("***");
            break;

        
        case CI_CMD_WAKEUP:
        case CI_CMD_WELCOME:
        case CI_CMD_SLEEP:
            break;

        
        default:
            break;
    }

    
    uart_write_string(DEBUG_UART_INDEX, "[CI1302] CMD_ID=0x");
    uart_write_byte(DEBUG_UART_INDEX, "0123456789ABCDEF"[(cmd_id >> 4) & 0x0F]);
    uart_write_byte(DEBUG_UART_INDEX, "0123456789ABCDEF"[cmd_id & 0x0F]);
    uart_write_string(DEBUG_UART_INDEX, "\r\n");
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
static void parse_byte(uint8 byte)
{
    switch(parse_state)
    {
        case STATE_IDLE:
            if(byte == FRAME_HEAD0)
            {
                frame_buf[0] = byte;
                frame_pos = 1;
                parse_state = STATE_HEAD1;
            }
            break;

        case STATE_HEAD1:
            if(byte == FRAME_HEAD1_RX)
            {
                frame_buf[1] = byte;
                frame_pos = 2;
                parse_state = STATE_DATA;
            }
            else
            {
                parse_state = STATE_IDLE;    
            }
            break;

        case STATE_DATA:
            frame_buf[frame_pos++] = byte;
            if(frame_pos >= FRAME_LEN)
            {
                
                
                if(frame_buf[7] != FRAME_TAIL)
                {
                    uart_write_string(DEBUG_UART_INDEX, "[ERR] Bad tail\r\n");
                    parse_state = STATE_IDLE;
                    break;
                }
                
                if(frame_buf[3] != FRAME_MSG_RX)
                {
                    uart_write_string(DEBUG_UART_INDEX, "[ERR] Bad msg type\r\n");
                    parse_state = STATE_IDLE;
                    break;
                }
                
                uint8 chk = 0;
                uint8 k;
                for(k = 0; k < 6; k++)
                {
                    chk += frame_buf[k];
                }
                if(chk != frame_buf[6])
                {
                    uart_write_string(DEBUG_UART_INDEX, "[ERR] Bad checksum\r\n");
                    parse_state = STATE_IDLE;
                    break;
                }
                
                uint8 cmd_id = frame_buf[4];

                
                ci1302_send_response(cmd_id);

                
                ci1302_execute_cmd(cmd_id);

                parse_state = STATE_IDLE;
            }
            break;

        default:
            parse_state = STATE_IDLE;
            break;
    }
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void screen_init(void)
{
    fifo_init(&rx_fifo, FIFO_DATA_8BIT, rx_buf, 64);

    gpio_init(P11_2, GPO, 1, GPO_PUSH_PULL);

    system_delay_ms(100);
    dot_matrix_screen_init();

    
    uart_write_string(DEBUG_UART_INDEX, "\r\n[DIAG] Self-test: DoubleFlash ON\r\n");
    dot_matrix_screen_set_brightness(5000);
    dot_matrix_screen_show_led_pattern(DOT_MATRIX_PATTERN_DOUBLE_FLASH);
    system_delay_ms(3000);
    dot_matrix_screen_set_brightness(0);
    dot_matrix_screen_clear_pattern();
    uart_write_string(DEBUG_UART_INDEX, "[DIAG] Self-test: OFF\r\n");

    uart_write_string(DEBUG_UART_INDEX, "===== CI1302 Screen Ready =====\r\n");
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void screen_poll(void)
{
    uint32 count = fifo_used(&rx_fifo);
    if(count == 0)
    {
        return;
    }

    fifo_read_buffer(&rx_fifo, rx_tmp, &count, FIFO_READ_AND_CLEAN);

    uint32 i;
    for(i = 0; i < count; i++)
    {
        parse_byte(rx_tmp[i]);
    }
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void screen_uart_rx_handler(void)
{
    uint8 dat;
    uart_query_byte(DEBUG_UART_INDEX, &dat);
    fifo_write_buffer(&rx_fifo, &dat, 1);
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
uint8 screen_get_last_cmd(void)
{
    return last_cmd_id;
}
