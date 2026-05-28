#ifndef CODE_SUBJECT_2_FIXED_ACTION_H_
#define CODE_SUBJECT_2_FIXED_ACTION_H_

#include "zf_common_typedef.h"
#include "voice_drive_action.h"

typedef struct
{
    float speed_mps;
    float steer_deg;
    uint8 finished;
} subject_2_fixed_action_output_t;

#endif
