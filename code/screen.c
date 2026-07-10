#include "screen.h"
#include <stdio.h>

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

// Diagnostics state (steps 2-5)
static uint32 ci_bytes_total = 0;
static uint32 ci_valid_frames = 0;
static uint32 ci_bad_tail = 0;
static uint32 ci_bad_checksum = 0;
static uint32 ci_bad_msg = 0;
static uint32 ci_no_head = 0;
static uint32 ci_last_stat_ms = 0;
static uint32 ci_last_byte_ms = 0;

// Recent raw dump buffer (16 bytes)
static uint8  raw_buf[16];
static uint8  raw_len = 0;
static uint8  raw_state = 0;      // 0: seeking 0x6B, 1: seeking 0x79, 2: found head
static uint32 raw_last_ms = 0;

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
                    ci_bad_tail++;
                    parse_state = STATE_IDLE;
                    break;
                }
                
                if(frame_buf[3] != FRAME_MSG_RX)
                {
                    ci_bad_msg++;
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
                    ci_bad_checksum++;
                    parse_state = STATE_IDLE;
                    break;
                }
                
                uint8 cmd_id = frame_buf[4];

                
                ci1302_send_response(cmd_id);

                
                ci1302_execute_cmd(cmd_id);
                ci_valid_frames++;

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

    system_delay_ms(10);
    dot_matrix_screen_clear_pattern();
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void screen_poll(void)
{
    uint32 now = system_getval_ms();
    uint32 count = 0;
    // Read bytes from the debug ring buffer (UART0) as the CI1302 input source.
    count = debug_read_ring_buffer(rx_tmp, sizeof(rx_tmp));
    if(count == 0)
    {
        return;
    }

    ci_bytes_total += count;
    ci_last_byte_ms = now;

    uint32 i;
    for(i = 0; i < count; i++)
    {
        uint8 b = rx_tmp[i];

        // Raw diagnostics state machine (step 2): collect non-head garbage and dump
        if(raw_state == 0) // seeking 0x6B
        {
            if(b != FRAME_HEAD0)
            {
                raw_buf[raw_len < 16 ? raw_len++ : 15] = b;
                if(raw_len >= 16)
                {
                    raw_len = 0;
                    ci_no_head++;
                }
            }
            else
            {
                raw_state = 1; // saw 0x6B
                raw_len = 0;
            }
            raw_last_ms = now;
        }
        else if(raw_state == 1) // seeking 0x79 after 0x6B
        {
            if(b == FRAME_HEAD1_RX)
            {
                raw_state = 2; // found potential frame start; stop raw dumping until next search
                raw_len = 0;
            }
            else
            {
                // false head, treat previous as garbage chunk continue seeking
                raw_buf[raw_len < 16 ? raw_len++ : 15] = b;
                if(raw_len >= 16)
                {
                    raw_len = 0;
                    ci_no_head++;
                }
                raw_state = 0;
            }
            raw_last_ms = now;
        }
        else
        {
            // in-frame or after recognizing a head: no raw dump here
        }

        // Timeout dump for partial garbage before finding head (step 2)
        if(raw_len > 0 && (now - raw_last_ms) >= 50 && raw_state == 0)
        {
            raw_len = 0;
            ci_no_head++;
        }

        // Normal parser (step 3 is handled by parser improving tolerance below)
        parse_byte(b);
    }

    // Stats once per second (step 4)
    if(now - ci_last_stat_ms >= 1000)
    {
        ci_last_stat_ms = now;
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
