#ifndef CODE_VOICE_DRIVE_ACTION_H_
#define CODE_VOICE_DRIVE_ACTION_H_

#include "zf_common_typedef.h"

typedef enum
{
    VOICE_DRIVE_ACTION_NONE = 0,
    VOICE_DRIVE_ACTION_FORWARD_10M,
    VOICE_DRIVE_ACTION_BACKWARD_10M,
    VOICE_DRIVE_ACTION_SNAKE_FORWARD_10M,
    VOICE_DRIVE_ACTION_SNAKE_BACKWARD_10M,
    VOICE_DRIVE_ACTION_CCW_CIRCLE,
    VOICE_DRIVE_ACTION_CW_CIRCLE,
    VOICE_DRIVE_ACTION_TURN_LEFT,
    VOICE_DRIVE_ACTION_TURN_RIGHT,
    VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M,
    VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M,
} voice_drive_action_mode_t;

void voice_drive_action_start(voice_drive_action_mode_t mode);
void voice_drive_action_stop(void);
void voice_drive_action_task(void);
uint8 voice_drive_action_get_mode(void);
float voice_drive_action_get_distance(void);
float voice_drive_action_get_yaw_delta(void);

#endif
