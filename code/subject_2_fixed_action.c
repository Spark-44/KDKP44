#include "subject_2_fixed_action.h"
#include "zf_common_headfile.h"
#include "rear_motor/rear_motor.h"

#define SUBJECT_2_FIXED_GUANDAO_SPEED_TO_MPS (0.1f)
#define SUBJECT_2_FIXED_TURN_SPEED_MPS     (0.35f)
#define SUBJECT_2_FIXED_STRAIGHT_DISTANCE_M (10.0f)
#define SUBJECT_2_FIXED_SNAKE_DISTANCE_M    (10.0f)
#define SUBJECT_2_FIXED_SNAKE_PERIOD_MS     (1000U)
#define SUBJECT_2_FIXED_TURN_ANGLE_DEG     (90.0f)
#define SUBJECT_2_FIXED_CIRCLE_ANGLE_DEG   (360.0f)
#define SUBJECT_2_FIXED_STEER_DEG          (25.0f)

typedef enum

{
    SUBJECT_2_FIXED_STOP_NONE = 0,
    SUBJECT_2_FIXED_STOP_DISTANCE,
    SUBJECT_2_FIXED_STOP_YAW,
} subject_2_fixed_stop_type_t;

typedef struct
{
    voice_drive_action_mode_t mode;
    float speed_mps;
    float steer_deg;
    float stop_value;
    subject_2_fixed_stop_type_t stop_type;
} subject_2_fixed_action_t;

typedef struct
{
    voice_drive_action_mode_t mode;
    uint32 start_ms;
    uint32 last_ms;
    float distance_m;
    float yaw_last;
    float yaw_delta;
    float yaw_target;
} subject_2_fixed_action_state_t;

static const subject_2_fixed_action_t subject_2_fixed_actions[] =
{
    {VOICE_DRIVE_ACTION_FORWARD_10M,        SUBJECT_2_FIXED_TURN_SPEED_MPS,  0.0f,                         SUBJECT_2_FIXED_STRAIGHT_DISTANCE_M, SUBJECT_2_FIXED_STOP_DISTANCE},
    {VOICE_DRIVE_ACTION_BACKWARD_10M,      -SUBJECT_2_FIXED_TURN_SPEED_MPS,  0.0f,                         SUBJECT_2_FIXED_STRAIGHT_DISTANCE_M, SUBJECT_2_FIXED_STOP_DISTANCE},
    {VOICE_DRIVE_ACTION_SNAKE_FORWARD_10M,  SUBJECT_2_FIXED_TURN_SPEED_MPS,  SUBJECT_2_FIXED_STEER_DEG,    SUBJECT_2_FIXED_SNAKE_DISTANCE_M,    SUBJECT_2_FIXED_STOP_DISTANCE},
    {VOICE_DRIVE_ACTION_SNAKE_BACKWARD_10M,-SUBJECT_2_FIXED_TURN_SPEED_MPS, -SUBJECT_2_FIXED_STEER_DEG,    SUBJECT_2_FIXED_SNAKE_DISTANCE_M,    SUBJECT_2_FIXED_STOP_DISTANCE},
    {VOICE_DRIVE_ACTION_CCW_CIRCLE,          SUBJECT_2_FIXED_TURN_SPEED_MPS,  SUBJECT_2_FIXED_STEER_DEG,       SUBJECT_2_FIXED_CIRCLE_ANGLE_DEG, SUBJECT_2_FIXED_STOP_YAW},
    {VOICE_DRIVE_ACTION_CW_CIRCLE,           SUBJECT_2_FIXED_TURN_SPEED_MPS, -SUBJECT_2_FIXED_STEER_DEG,       SUBJECT_2_FIXED_CIRCLE_ANGLE_DEG, SUBJECT_2_FIXED_STOP_YAW},
    {VOICE_DRIVE_ACTION_TURN_LEFT,           SUBJECT_2_FIXED_TURN_SPEED_MPS,  SUBJECT_2_FIXED_STEER_DEG,       SUBJECT_2_FIXED_TURN_ANGLE_DEG,   SUBJECT_2_FIXED_STOP_YAW},
    {VOICE_DRIVE_ACTION_TURN_RIGHT,          SUBJECT_2_FIXED_TURN_SPEED_MPS, -SUBJECT_2_FIXED_STEER_DEG,       SUBJECT_2_FIXED_TURN_ANGLE_DEG,   SUBJECT_2_FIXED_STOP_YAW},
};

static subject_2_fixed_action_state_t subject_2_fixed_action_state = {VOICE_DRIVE_ACTION_NONE, 0, 0, 0.0f, 0.0f, 0.0f};

static float subject_2_fixed_abs_float(float value)
{
    return value >= 0.0f ? value : -value;
}

static float subject_2_fixed_yaw_step(float now, float last)
{
    float diff = now - last;
    while(diff > 180.0f)
    {
        diff -= 360.0f;
    }
    while(diff < -180.0f)
    {
        diff += 360.0f;
    }
    return diff;
}

static void subject_2_fixed_apply(float speed_mps, float steer_deg)
{
    conrtol_mode = GUANDAO;
    out_v_l = speed_mps / SUBJECT_2_FIXED_GUANDAO_SPEED_TO_MPS;
    out_v_r = speed_mps / SUBJECT_2_FIXED_GUANDAO_SPEED_TO_MPS;
    out_servo = steer_deg;
}

static const subject_2_fixed_action_t *subject_2_fixed_find_action(voice_drive_action_mode_t mode)
{
    uint32 i;

    for(i = 0; i < sizeof(subject_2_fixed_actions) / sizeof(subject_2_fixed_actions[0]); i++)
    {
        if(subject_2_fixed_actions[i].mode == mode)
        {
            return &subject_2_fixed_actions[i];
        }
    }

    return 0;
}

static void subject_2_fixed_action_update(voice_drive_action_mode_t mode, uint32 elapsed_ms, float distance_m, float yaw_delta, float yaw_target, float yaw_current, subject_2_fixed_action_output_t *output)
{
    const subject_2_fixed_action_t *action;

    (void)elapsed_ms;
    (void)yaw_target;
    (void)yaw_current;

    if(output == 0)
    {
        return;
    }

    output->speed_mps = 0.0f;
    output->steer_deg = 0.0f;
    output->finished = 0;

    action = subject_2_fixed_find_action(mode);
    if(action == 0)
    {
        output->finished = 1;
        return;
    }

    output->speed_mps = action->speed_mps;
    output->steer_deg = action->steer_deg;

    if(mode == VOICE_DRIVE_ACTION_SNAKE_FORWARD_10M || mode == VOICE_DRIVE_ACTION_SNAKE_BACKWARD_10M)
    {
        if(((elapsed_ms / SUBJECT_2_FIXED_SNAKE_PERIOD_MS) % 2U) != 0U)
        {
            output->steer_deg = -action->steer_deg;
        }
    }

    if(action->stop_type == SUBJECT_2_FIXED_STOP_DISTANCE && distance_m >= action->stop_value)
    {
        output->finished = 1;
    }
    else if(action->stop_type == SUBJECT_2_FIXED_STOP_YAW && subject_2_fixed_abs_float(yaw_delta) >= action->stop_value)
    {
        output->finished = 1;
    }
}

void voice_drive_action_stop(void)
{
    subject_2_fixed_action_state.mode = VOICE_DRIVE_ACTION_NONE;
    subject_2_fixed_action_state.distance_m = 0.0f;
    subject_2_fixed_action_state.yaw_delta = 0.0f;
    subject_2_fixed_action_state.yaw_target = 0.0f;
    out_v_l = 0.0f;
    out_v_r = 0.0f;
    out_servo = 0.0f;
    rear_motor_set_target_mps(0.0f);
}

void voice_drive_action_start(voice_drive_action_mode_t mode)
{
    voice_drive_action_stop();
    subject_2_fixed_action_state.mode = mode;
    subject_2_fixed_action_state.start_ms = system_getval_ms();
    subject_2_fixed_action_state.last_ms = subject_2_fixed_action_state.start_ms;
    subject_2_fixed_action_state.distance_m = 0.0f;
    subject_2_fixed_action_state.yaw_last = Yaw_1;
    subject_2_fixed_action_state.yaw_delta = 0.0f;
    subject_2_fixed_action_state.yaw_target = Yaw_1;
    conrtol_mode = GUANDAO;
}

void voice_drive_action_task(void)
{
    uint32 now_ms;
    uint32 dt_ms;
    float dt_s;
    float speed_mps = 0.0f;
    float steer_deg = 0.0f;
    float yaw_step;
    uint32 elapsed_ms;
    subject_2_fixed_action_output_t fixed_action_output;

    if(subject_2_fixed_action_state.mode == VOICE_DRIVE_ACTION_NONE)
    {
        return;
    }

    now_ms = system_getval_ms();
    dt_ms = now_ms - subject_2_fixed_action_state.last_ms;
    subject_2_fixed_action_state.last_ms = now_ms;
    if(dt_ms > 200U)
    {
        dt_ms = 200U;
    }
    dt_s = (float)dt_ms * 0.001f;

    yaw_step = subject_2_fixed_yaw_step(Yaw_1, subject_2_fixed_action_state.yaw_last);
    subject_2_fixed_action_state.yaw_last = Yaw_1;
    subject_2_fixed_action_state.yaw_delta += yaw_step;
    elapsed_ms = now_ms - subject_2_fixed_action_state.start_ms;

    subject_2_fixed_action_update(subject_2_fixed_action_state.mode, elapsed_ms, subject_2_fixed_action_state.distance_m, subject_2_fixed_action_state.yaw_delta, subject_2_fixed_action_state.yaw_target, Yaw_1, &fixed_action_output);
    speed_mps = fixed_action_output.speed_mps;
    steer_deg = fixed_action_output.steer_deg;

    subject_2_fixed_action_state.distance_m += subject_2_fixed_abs_float(speed_mps) * dt_s;
    subject_2_fixed_apply(speed_mps, steer_deg);

    subject_2_fixed_action_update(subject_2_fixed_action_state.mode, elapsed_ms, subject_2_fixed_action_state.distance_m, subject_2_fixed_action_state.yaw_delta, subject_2_fixed_action_state.yaw_target, Yaw_1, &fixed_action_output);
    if(fixed_action_output.finished)
    {
        voice_drive_action_stop();
    }
}

uint8 voice_drive_action_get_mode(void)
{
    return (uint8)subject_2_fixed_action_state.mode;
}

float voice_drive_action_get_distance(void)
{
    return subject_2_fixed_action_state.distance_m;
}

float voice_drive_action_get_yaw_delta(void)
{
    return subject_2_fixed_action_state.yaw_delta;
}
