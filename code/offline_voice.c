#include "offline_voice.h"
#include <stdio.h>

#define OFFLINE_VOICE_FRAME_LEN       (8)
#define OFFLINE_VOICE_HEAD0           (0x6B)
#define OFFLINE_VOICE_HEAD1_RX        (0x79)
#define OFFLINE_VOICE_HEAD1_TX        (0x6A)
#define OFFLINE_VOICE_MSG_RX          (0x81)
#define OFFLINE_VOICE_MSG_TX          (0x82)
#define OFFLINE_VOICE_TAIL            (0xFB)
typedef enum
{
    OFFLINE_VOICE_STATE_IDLE = 0,
    OFFLINE_VOICE_STATE_HEAD1,
    OFFLINE_VOICE_STATE_DATA,
} offline_voice_parse_state_t;

#pragma section all "cpu0_dsram"
static offline_voice_parse_state_t offline_voice_parse_state = OFFLINE_VOICE_STATE_IDLE;
static uint8 offline_voice_frame_buf[OFFLINE_VOICE_FRAME_LEN];
static uint8 offline_voice_frame_pos = 0;
static offline_voice_cmd_callback_t offline_voice_callback = 0;
static void *offline_voice_user_data = 0;
static offline_voice_stats_t offline_voice_stats = {0};
static uint32 offline_voice_last_stat_ms = 0;
#pragma section all restore

static void offline_voice_debug_hex(uint8 value)
{
    uart_write_byte(DEBUG_UART_INDEX, "0123456789ABCDEF"[(value >> 4) & 0x0F]);
    uart_write_byte(DEBUG_UART_INDEX, "0123456789ABCDEF"[value & 0x0F]);
}

static void offline_voice_debug_stats(void)
{
    uint32 now_ms = system_getval_ms();

    if((uint32)(now_ms - offline_voice_last_stat_ms) >= 1000U)
    {
        char line[160];
        int len;

        offline_voice_last_stat_ms = now_ms;
        len = sprintf(line,
                      "[VOICE-UART1] bytes=%lu valid=%lu badEnd=%lu badChecksum=%lu badMsg=%lu last=0x%02X\r\n",
                      (unsigned long)offline_voice_stats.bytes_total,
                      (unsigned long)offline_voice_stats.valid_frames,
                      (unsigned long)offline_voice_stats.bad_tail,
                      (unsigned long)offline_voice_stats.bad_checksum,
                      (unsigned long)offline_voice_stats.bad_msg,
                      (unsigned int)offline_voice_stats.last_cmd_id);
        if(len > 0)
        {
            uart_write_string(DEBUG_UART_INDEX, line);
        }
    }
}

static uint8 offline_voice_calc_checksum(const uint8 *frame)
{
    uint8 checksum = 0;
    uint8 i;

    for(i = 0; i < (OFFLINE_VOICE_FRAME_LEN - 2); i++)
    {
        checksum += frame[i];
    }

    return checksum;
}

static void offline_voice_reset_parser(void)
{
    offline_voice_parse_state = OFFLINE_VOICE_STATE_IDLE;
    offline_voice_frame_pos = 0;
}

static void offline_voice_handle_frame(void)
{
    uint8 cmd_id;

    if(offline_voice_frame_buf[7] != OFFLINE_VOICE_TAIL)
    {
        offline_voice_stats.bad_tail++;
        offline_voice_reset_parser();
        return;
    }

    if(offline_voice_frame_buf[3] != OFFLINE_VOICE_MSG_RX)
    {
        offline_voice_stats.bad_msg++;
        offline_voice_reset_parser();
        return;
    }

    if(offline_voice_calc_checksum(offline_voice_frame_buf) != offline_voice_frame_buf[6])
    {
        offline_voice_stats.bad_checksum++;
        offline_voice_reset_parser();
        return;
    }

    cmd_id = offline_voice_frame_buf[4];
    offline_voice_stats.last_cmd_id = cmd_id;
    offline_voice_stats.valid_frames++;

    offline_voice_send_response(cmd_id);

    uart_write_string(DEBUG_UART_INDEX, "[VOICE-UART1] CMD_ID=0x");
    offline_voice_debug_hex(cmd_id);
    uart_write_string(DEBUG_UART_INDEX, "\r\n");

    if(offline_voice_callback != 0)
    {
        offline_voice_callback(cmd_id, offline_voice_user_data);
    }

    offline_voice_reset_parser();
}

void offline_voice_init(offline_voice_cmd_callback_t callback, void *user_data)
{
    offline_voice_callback = callback;
    offline_voice_user_data = user_data;
    offline_voice_reset_stats();
    offline_voice_reset_parser();
    uart_init(OFFLINE_VOICE_UART_INDEX,
              OFFLINE_VOICE_UART_BAUD,
              OFFLINE_VOICE_UART_TX_PIN,
              OFFLINE_VOICE_UART_RX_PIN);
    uart_rx_interrupt(OFFLINE_VOICE_UART_INDEX, 1);
    uart_write_string(DEBUG_UART_INDEX, "[VOICE-UART1] init 9600 TX=P11.12 RX=P11.10\r\n");
}

void offline_voice_poll(void)
{
    uint8 data;

    while(uart_query_byte(OFFLINE_VOICE_UART_INDEX, &data))
    {
        offline_voice_feed_byte(data);
    }
    offline_voice_debug_stats();
}

void offline_voice_uart_rx_handler(void)
{
    uint8 data;

    while(uart_query_byte(OFFLINE_VOICE_UART_INDEX, &data))
    {
        offline_voice_feed_byte(data);
    }
}

void offline_voice_feed_byte(uint8 byte)
{
    offline_voice_stats.bytes_total++;

    switch(offline_voice_parse_state)
    {
        case OFFLINE_VOICE_STATE_IDLE:
            if(byte == OFFLINE_VOICE_HEAD0)
            {
                offline_voice_frame_buf[0] = byte;
                offline_voice_frame_pos = 1;
                offline_voice_parse_state = OFFLINE_VOICE_STATE_HEAD1;
            }
            break;

        case OFFLINE_VOICE_STATE_HEAD1:
            if(byte == OFFLINE_VOICE_HEAD1_RX)
            {
                offline_voice_frame_buf[1] = byte;
                offline_voice_frame_pos = 2;
                offline_voice_parse_state = OFFLINE_VOICE_STATE_DATA;
            }
            else if(byte == OFFLINE_VOICE_HEAD0)
            {
                offline_voice_frame_buf[0] = byte;
                offline_voice_frame_pos = 1;
            }
            else
            {
                offline_voice_reset_parser();
            }
            break;

        case OFFLINE_VOICE_STATE_DATA:
            offline_voice_frame_buf[offline_voice_frame_pos] = byte;
            offline_voice_frame_pos++;
            if(offline_voice_frame_pos >= OFFLINE_VOICE_FRAME_LEN)
            {
                offline_voice_handle_frame();
            }
            break;

        default:
            offline_voice_reset_parser();
            break;
    }
}

void offline_voice_send_response(uint8 cmd_id)
{
    uint8 response[OFFLINE_VOICE_FRAME_LEN];
    uint8 i;

    response[0] = OFFLINE_VOICE_HEAD0;
    response[1] = OFFLINE_VOICE_HEAD1_TX;
    response[2] = 0x00;
    response[3] = OFFLINE_VOICE_MSG_TX;
    response[4] = cmd_id;
    response[5] = 0x00;
    response[6] = offline_voice_calc_checksum(response);
    response[7] = OFFLINE_VOICE_TAIL;

    for(i = 0; i < OFFLINE_VOICE_FRAME_LEN; i++)
    {
        uart_write_byte(OFFLINE_VOICE_UART_INDEX, response[i]);
    }
}

void offline_voice_reset_stats(void)
{
    offline_voice_stats.bytes_total = 0;
    offline_voice_stats.valid_frames = 0;
    offline_voice_stats.bad_tail = 0;
    offline_voice_stats.bad_checksum = 0;
    offline_voice_stats.bad_msg = 0;
    offline_voice_stats.last_cmd_id = 0;
}

uint8 offline_voice_get_last_cmd(void)
{
    return offline_voice_stats.last_cmd_id;
}

void offline_voice_get_stats(offline_voice_stats_t *stats)
{
    if(stats != 0)
    {
        *stats = offline_voice_stats;
    }
}
