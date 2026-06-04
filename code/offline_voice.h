#ifndef CODE_OFFLINE_VOICE_H_
#define CODE_OFFLINE_VOICE_H_

#include "zf_common_headfile.h"

#define OFFLINE_VOICE_CMD_WAKEUP               (0x01)
#define OFFLINE_VOICE_CMD_WELCOME              (0x02)
#define OFFLINE_VOICE_CMD_SLEEP                (0x03)

#define OFFLINE_VOICE_CMD_TURN_LEFT            (0x04)
#define OFFLINE_VOICE_CMD_TURN_RIGHT           (0x05)
#define OFFLINE_VOICE_CMD_HIGH_BEAM            (0x06)
#define OFFLINE_VOICE_CMD_LOW_BEAM             (0x07)
#define OFFLINE_VOICE_CMD_FOG_LIGHT            (0x08)
#define OFFLINE_VOICE_CMD_DOUBLE_FLASH         (0x09)
#define OFFLINE_VOICE_CMD_INTERIOR_LIGHT       (0x0A)

#define OFFLINE_VOICE_CMD_WIPER                (0x0B)

#define OFFLINE_VOICE_CMD_HORN_1S              (0x0C)
#define OFFLINE_VOICE_CMD_HORN_2S              (0x0D)
#define OFFLINE_VOICE_CMD_HORN_3S              (0x0E)
#define OFFLINE_VOICE_CMD_HORN_2X              (0x0F)
#define OFFLINE_VOICE_CMD_HORN_3X              (0x10)
#define OFFLINE_VOICE_CMD_HORN_4X              (0x11)
#define OFFLINE_VOICE_CMD_HORN_LONG_SHORT      (0x12)
#define OFFLINE_VOICE_CMD_HORN_RAPID           (0x13)
#define OFFLINE_VOICE_CMD_HORN_ALARM           (0x14)

#define OFFLINE_VOICE_CMD_GATE1_LEFT           (0x15)
#define OFFLINE_VOICE_CMD_GATE1                (0x16)
#define OFFLINE_VOICE_CMD_GATE2                (0x17)
#define OFFLINE_VOICE_CMD_GATE3                (0x18)
#define OFFLINE_VOICE_CMD_GATE3_RIGHT          (0x19)

#define OFFLINE_VOICE_CMD_GATE1_RIGHT_BACK     (0x1A)
#define OFFLINE_VOICE_CMD_GATE1_BACK           (0x1B)
#define OFFLINE_VOICE_CMD_GATE2_BACK           (0x1C)
#define OFFLINE_VOICE_CMD_GATE3_BACK           (0x1D)
#define OFFLINE_VOICE_CMD_GATE3_LEFT_BACK      (0x1E)

#define OFFLINE_VOICE_CMD_FORWARD_10M          (0x1F)
#define OFFLINE_VOICE_CMD_BACKWARD_10M         (0x20)
#define OFFLINE_VOICE_CMD_SNAKE_FORWARD        (0x21)
#define OFFLINE_VOICE_CMD_SNAKE_BACKWARD       (0x22)
#define OFFLINE_VOICE_CMD_CCW_CIRCLE           (0x23)
#define OFFLINE_VOICE_CMD_CW_CIRCLE            (0x24)
#define OFFLINE_VOICE_CMD_TURN_LEFT_DRIVE      (0x25)
#define OFFLINE_VOICE_CMD_TURN_RIGHT_DRIVE     (0x26)

#define OFFLINE_VOICE_CMD_ROUTE_ARRIVE_CHANGE  (0x27)
#define OFFLINE_VOICE_CMD_ROUTE_START_CHANGE   (0x28)
#define OFFLINE_VOICE_CMD_ROUTE_STRAIGHT       (0x29)
#define OFFLINE_VOICE_CMD_ROUTE_SNAKE          (0x2A)
#define OFFLINE_VOICE_CMD_BACK_STRAIGHT        (0x2B)
#define OFFLINE_VOICE_CMD_BACK_SNAKE           (0x2C)

#define OFFLINE_VOICE_CMD_MAX                  (0x2C)

typedef void (*offline_voice_cmd_callback_t)(uint8 cmd_id, void *user_data);

typedef struct
{
    uint32 bytes_total;
    uint32 valid_frames;
    uint32 bad_tail;
    uint32 bad_checksum;
    uint32 bad_msg;
    uint8  last_cmd_id;
} offline_voice_stats_t;

void offline_voice_init(offline_voice_cmd_callback_t callback, void *user_data);
void offline_voice_poll(void);
void offline_voice_feed_byte(uint8 byte);
void offline_voice_send_response(uint8 cmd_id);
void offline_voice_reset_stats(void);
uint8 offline_voice_get_last_cmd(void);
void offline_voice_get_stats(offline_voice_stats_t *stats);

#endif
