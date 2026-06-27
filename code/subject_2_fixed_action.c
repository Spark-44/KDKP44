#include "subject_2_fixed_action.h"
#include "zf_common_headfile.h"
#include "rear_motor/rear_motor.h"

#define SUBJECT_2_FIXED_GUANDAO_SPEED_TO_MPS (0.1f)
#define SUBJECT_2_FIXED_TURN_SPEED_MPS     (0.35f)
#define SUBJECT_2_FIXED_STRAIGHT_DISTANCE_M (10.0f)
#define SUBJECT_2_FIXED_TURN_PRE_DISTANCE_M (2.0f)
#define SUBJECT_2_FIXED_SNAKE_DISTANCE_M    (10.0f)
#define SUBJECT_2_FIXED_SNAKE_PERIOD_MS     (1000U)
#define SUBJECT_2_FIXED_TURN_ANGLE_DEG     (90.0f)
#define SUBJECT_2_FIXED_TURN_TIMEOUT_MS     (60000U)
#define SUBJECT_2_FIXED_CIRCLE_ANGLE_DEG   (360.0f)
#define SUBJECT_2_FIXED_STEER_DEG          (25.0f)
#define SUBJECT_2_ENCODER_YAW_SPEED_MPS    (0.35f)
#define SUBJECT_2_ENCODER_YAW_MIN_SPEED_MPS (0.15f)
#define SUBJECT_2_ENCODER_YAW_DISTANCE_M   (10.0f)
#define SUBJECT_2_ENCODER_YAW_SLOWDOWN_M   (1.0f)
#define SUBJECT_2_ENCODER_YAW_KP           (0.80f)
#define SUBJECT_2_ENCODER_YAW_KI           (0.20f)
#define SUBJECT_2_ENCODER_YAW_KD           (0.04f)
#define SUBJECT_2_ENCODER_YAW_INTEGRAL_LIMIT (20.0f)
#define SUBJECT_2_ENCODER_YAW_DEADBAND_DEG (0.05f)
#define SUBJECT_2_ENCODER_YAW_STEER_LIMIT_DEG (8.0f)
#define SUBJECT_2_ENCODER_YAW_STEER_RATE_DEG (0.50f)
#define SUBJECT_2_ENCODER_YAW_CONTROL_MS   (20U)
#define SUBJECT_2_ENCODER_YAW_STALL_MS     (3000U)
#define SUBJECT_2_ENCODER_YAW_PROGRESS_M   (0.002f)
#define SUBJECT_2_ENCODER_YAW_MAX_DELTA    (1000)

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
    float phase_distance_m;
    float yaw_last;
    float yaw_delta;
    float yaw_target;
    uint8 turn_phase;
    int16 encoder_last_count;
    int32 encoder_delta_last;
    float yaw_error_last;
    float yaw_error_integral;
    float steer_command;
    float speed_command;
    float progress_distance_mark;
    uint32 last_control_ms;
    uint32 last_progress_ms;
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

typedef enum
{
    SUBJECT_2_FIXED_TURN_PHASE_NONE = 0,
    SUBJECT_2_FIXED_TURN_PHASE_STRAIGHT,
    SUBJECT_2_FIXED_TURN_PHASE_TURN,
} subject_2_fixed_turn_phase_t;

static subject_2_fixed_action_state_t subject_2_fixed_action_state =
{
    VOICE_DRIVE_ACTION_NONE,
    0,
    0,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    SUBJECT_2_FIXED_TURN_PHASE_NONE,
    0,
    0,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0.0f,
    0,
    0
};

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

static uint8 subject_2_fixed_is_encoder_yaw_action(voice_drive_action_mode_t mode)
{
    return (mode == VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M
            || mode == VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M) ? 1U : 0U;
}

static void subject_2_fixed_apply(float speed_mps, float steer_deg);

static void subject_2_straight_append_fixed100(char *line, int *position, float value)
{
    long scaled = (value >= 0.0f)
            ? (long)(value * 100.0f + 0.5f)
            : (long)(value * 100.0f - 0.5f);

    if(scaled < 0)
    {
        line[(*position)++] = '-';
        scaled = -scaled;
    }
    *position += sprintf(&line[*position], "%ld.%02ld", scaled / 100L, scaled % 100L);
}

static void subject_2_encoder_yaw_log(const char *tag, const char *reason, uint8 force)
{
    static uint32 last_log_ms = 0;
    uint32 now_ms = system_getval_ms();
    float yaw_error = subject_2_fixed_yaw_step(subject_2_fixed_action_state.yaw_target, Yaw_Straight_1);
    char line[320];
    int position = 0;

    if(!force && (uint32)(now_ms - last_log_ms) < 500U) return;
    last_log_ms = now_ms;
    position += sprintf(line,
            "[STRAIGHT] %s dir=%s dist=",
            tag,
            (subject_2_fixed_action_state.mode == VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M) ? "REV" : "FWD");
    subject_2_straight_append_fixed100(line, &position, subject_2_fixed_action_state.distance_m);
    position += sprintf(&line[position], " target=");
    subject_2_straight_append_fixed100(line, &position, subject_2_fixed_action_state.yaw_target);
    position += sprintf(&line[position], " yaw=");
    subject_2_straight_append_fixed100(line, &position, Yaw_Straight_1);
    position += sprintf(&line[position], " err=");
    subject_2_straight_append_fixed100(line, &position, yaw_error);
    position += sprintf(&line[position], " steer=");
    subject_2_straight_append_fixed100(line, &position, subject_2_fixed_action_state.steer_command);
    position += sprintf(&line[position], " speed=");
    subject_2_straight_append_fixed100(line, &position, subject_2_fixed_action_state.speed_command);
    position += sprintf(&line[position], " enc=%ld actual=", (long)subject_2_fixed_action_state.encoder_delta_last);
    subject_2_straight_append_fixed100(line, &position, rear_motor_get_speed_mps());
    position += sprintf(&line[position], " pwm=%d rack_target=", (int)rear_motor_get_pwm());
    subject_2_straight_append_fixed100(line, &position, angle_control_get_target_angle());
    position += sprintf(&line[position], " rack_actual=");
    subject_2_straight_append_fixed100(line, &position, angle_control_get_current_angle_float());
    sprintf(&line[position], " rack_pwm=%ld reason=%s\r\n", (long)angle_control_get_output_pwm(), reason);
    uart_write_string(DEBUG_UART_INDEX, line);
}

static void subject_2_encoder_yaw_finish(const char *reason)
{
    subject_2_encoder_yaw_log("STOP", reason, 1U);
    subject_2_fixed_action_state.mode = VOICE_DRIVE_ACTION_NONE;
    voice_drive_action_stop();
}

static void subject_2_encoder_yaw_task(uint32 now_ms)
{
    int16 encoder_now = l_ecdcounter();
    int32 encoder_delta = calculate_delta(encoder_now, subject_2_fixed_action_state.encoder_last_count);
    uint32 control_elapsed_ms = now_ms - subject_2_fixed_action_state.last_control_ms;
    float distance_step = 0.0f;
    float remaining;
    float speed_mps;

    subject_2_fixed_action_state.encoder_last_count = encoder_now;
    subject_2_fixed_action_state.encoder_delta_last = encoder_delta;
    if(encoder_delta <= SUBJECT_2_ENCODER_YAW_MAX_DELTA && encoder_delta >= -SUBJECT_2_ENCODER_YAW_MAX_DELTA)
    {
        distance_step = subject_2_fixed_abs_float((float)encoder_delta) * ONE_TICK_DISTANCE;
        subject_2_fixed_action_state.distance_m += distance_step;
        if(subject_2_fixed_action_state.distance_m - subject_2_fixed_action_state.progress_distance_mark
                >= SUBJECT_2_ENCODER_YAW_PROGRESS_M)
        {
            subject_2_fixed_action_state.last_progress_ms = now_ms;
            subject_2_fixed_action_state.progress_distance_mark = subject_2_fixed_action_state.distance_m;
        }
    }

    if(subject_2_fixed_action_state.distance_m >= SUBJECT_2_ENCODER_YAW_DISTANCE_M)
    {
        subject_2_encoder_yaw_finish("DISTANCE");
        return;
    }
    if((uint32)(now_ms - subject_2_fixed_action_state.last_progress_ms) >= SUBJECT_2_ENCODER_YAW_STALL_MS)
    {
        subject_2_encoder_yaw_finish("STALL");
        return;
    }

    remaining = SUBJECT_2_ENCODER_YAW_DISTANCE_M - subject_2_fixed_action_state.distance_m;
    speed_mps = SUBJECT_2_ENCODER_YAW_SPEED_MPS;
    if(remaining < SUBJECT_2_ENCODER_YAW_SLOWDOWN_M)
    {
        speed_mps = SUBJECT_2_ENCODER_YAW_MIN_SPEED_MPS
                + (SUBJECT_2_ENCODER_YAW_SPEED_MPS - SUBJECT_2_ENCODER_YAW_MIN_SPEED_MPS)
                * remaining / SUBJECT_2_ENCODER_YAW_SLOWDOWN_M;
    }

    if(control_elapsed_ms >= SUBJECT_2_ENCODER_YAW_CONTROL_MS)
    {
        float dt_s = (float)control_elapsed_ms * 0.001f;
        float yaw_error = subject_2_fixed_action_state.yaw_target - Yaw_Straight_1;
        float yaw_error_delta;
        float yaw_error_integral_old;
        float correction_without_new_integral;
        float correction_with_new_integral;
        float steer_command;
        float steer_delta;

        while(yaw_error > 180.0f) yaw_error -= 360.0f;
        while(yaw_error < -180.0f) yaw_error += 360.0f;
        if(subject_2_fixed_abs_float(yaw_error) < SUBJECT_2_ENCODER_YAW_DEADBAND_DEG) yaw_error = 0.0f;
        yaw_error_delta = subject_2_fixed_yaw_step(yaw_error, subject_2_fixed_action_state.yaw_error_last);
        yaw_error_integral_old = subject_2_fixed_action_state.yaw_error_integral;
        subject_2_fixed_action_state.yaw_error_integral += yaw_error * dt_s;
        Value_Limit_float(&subject_2_fixed_action_state.yaw_error_integral,
                          -SUBJECT_2_ENCODER_YAW_INTEGRAL_LIMIT,
                          SUBJECT_2_ENCODER_YAW_INTEGRAL_LIMIT);
        correction_without_new_integral = SUBJECT_2_ENCODER_YAW_KP * yaw_error
                + SUBJECT_2_ENCODER_YAW_KI * yaw_error_integral_old
                + SUBJECT_2_ENCODER_YAW_KD * yaw_error_delta / dt_s;
        correction_with_new_integral = SUBJECT_2_ENCODER_YAW_KP * yaw_error
                + SUBJECT_2_ENCODER_YAW_KI * subject_2_fixed_action_state.yaw_error_integral
                + SUBJECT_2_ENCODER_YAW_KD * yaw_error_delta / dt_s;
        if(subject_2_fixed_abs_float(correction_with_new_integral) > SUBJECT_2_ENCODER_YAW_STEER_LIMIT_DEG
                && subject_2_fixed_abs_float(correction_with_new_integral)
                        > subject_2_fixed_abs_float(correction_without_new_integral))
        {
            subject_2_fixed_action_state.yaw_error_integral = yaw_error_integral_old;
            correction_with_new_integral = correction_without_new_integral;
        }
        steer_command = -correction_with_new_integral;
        if(subject_2_fixed_action_state.mode == VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M)
        {
            steer_command = -steer_command;
        }
        Value_Limit_float(&steer_command,
                          -SUBJECT_2_ENCODER_YAW_STEER_LIMIT_DEG,
                          SUBJECT_2_ENCODER_YAW_STEER_LIMIT_DEG);
        steer_delta = steer_command - subject_2_fixed_action_state.steer_command;
        Value_Limit_float(&steer_delta,
                          -SUBJECT_2_ENCODER_YAW_STEER_RATE_DEG,
                          SUBJECT_2_ENCODER_YAW_STEER_RATE_DEG);
        subject_2_fixed_action_state.steer_command += steer_delta;
        subject_2_fixed_action_state.yaw_error_last = yaw_error;
        subject_2_fixed_action_state.last_control_ms = now_ms;
    }

    subject_2_fixed_action_state.speed_command =
            (subject_2_fixed_action_state.mode == VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M)
            ? -speed_mps : speed_mps;
    subject_2_fixed_apply(subject_2_fixed_action_state.speed_command,
                          subject_2_fixed_action_state.steer_command);
    subject_2_encoder_yaw_log("RUN", "NONE", 0U);
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

static uint8 subject_2_fixed_is_turn_action(voice_drive_action_mode_t mode)
{
    return (mode == VOICE_DRIVE_ACTION_TURN_LEFT || mode == VOICE_DRIVE_ACTION_TURN_RIGHT);
}

static float subject_2_fixed_turn_yaw_delta(void)
{
    return subject_2_fixed_yaw_step(Yaw_1, subject_2_fixed_action_state.yaw_target);
}

static void subject_2_fixed_log_state(const char *tag)
{
    static uint32 last_log_ms = 0;
    uint32 now_ms = system_getval_ms();
    char line[160];

    if((uint32)(now_ms - last_log_ms) < 200U)
    {
        return;
    }
    last_log_ms = now_ms;

    sprintf(line,
            "[FIXED] %s mode=%u phase=%u dist=%ld.%02ld yaw=%ld.%02ld delta=%ld.%02ld target=%ld.%02ld\r\n",
            tag,
            (unsigned)subject_2_fixed_action_state.mode,
            (unsigned)subject_2_fixed_action_state.turn_phase,
            (long)subject_2_fixed_action_state.distance_m,
            (long)(subject_2_fixed_abs_float(subject_2_fixed_action_state.distance_m - (long)subject_2_fixed_action_state.distance_m) * 100.0f),
            (long)Yaw_1,
            (long)(subject_2_fixed_abs_float(Yaw_1 - (long)Yaw_1) * 100.0f),
            (long)voice_drive_action_get_yaw_delta(),
            (long)(subject_2_fixed_abs_float(voice_drive_action_get_yaw_delta() - (long)voice_drive_action_get_yaw_delta()) * 100.0f),
            (long)subject_2_fixed_action_state.yaw_target,
            (long)(subject_2_fixed_abs_float(subject_2_fixed_action_state.yaw_target - (long)subject_2_fixed_action_state.yaw_target) * 100.0f));
    uart_write_string(DEBUG_UART_INDEX, line);
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

    if(subject_2_fixed_is_turn_action(mode))
    {
        if(subject_2_fixed_action_state.turn_phase == SUBJECT_2_FIXED_TURN_PHASE_STRAIGHT)
        {
            output->steer_deg = 0.0f;
            if(subject_2_fixed_action_state.phase_distance_m >= SUBJECT_2_FIXED_TURN_PRE_DISTANCE_M)
            {
                subject_2_fixed_action_state.turn_phase = SUBJECT_2_FIXED_TURN_PHASE_TURN;
                subject_2_fixed_action_state.phase_distance_m = 0.0f;
                subject_2_fixed_action_state.yaw_last = Yaw_1;
                subject_2_fixed_action_state.yaw_target = Yaw_1;
                subject_2_fixed_action_state.yaw_delta = 0.0f;
            }
        }
        else if(subject_2_fixed_action_state.turn_phase == SUBJECT_2_FIXED_TURN_PHASE_TURN)
        {
            yaw_delta = subject_2_fixed_turn_yaw_delta();
        }
    }

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
    else if(action->stop_type == SUBJECT_2_FIXED_STOP_YAW
            && (!subject_2_fixed_is_turn_action(mode) || subject_2_fixed_action_state.turn_phase == SUBJECT_2_FIXED_TURN_PHASE_TURN)
            && subject_2_fixed_abs_float(yaw_delta) >= action->stop_value)
    {
        output->finished = 1;
    }
    else if(subject_2_fixed_is_turn_action(mode) && elapsed_ms >= SUBJECT_2_FIXED_TURN_TIMEOUT_MS)
    {
        output->finished = 1;
    }
}

void voice_drive_action_stop(void)
{
    if(subject_2_fixed_is_encoder_yaw_action(subject_2_fixed_action_state.mode))
    {
        subject_2_encoder_yaw_log("STOP", "COMMAND", 1U);
    }
    subject_2_fixed_action_state.mode = VOICE_DRIVE_ACTION_NONE;
    subject_2_fixed_action_state.distance_m = 0.0f;
    subject_2_fixed_action_state.phase_distance_m = 0.0f;
    subject_2_fixed_action_state.yaw_delta = 0.0f;
    subject_2_fixed_action_state.yaw_target = 0.0f;
    subject_2_fixed_action_state.turn_phase = SUBJECT_2_FIXED_TURN_PHASE_NONE;
    subject_2_fixed_action_state.encoder_last_count = 0;
    subject_2_fixed_action_state.encoder_delta_last = 0;
    subject_2_fixed_action_state.yaw_error_last = 0.0f;
    subject_2_fixed_action_state.yaw_error_integral = 0.0f;
    subject_2_fixed_action_state.steer_command = 0.0f;
    subject_2_fixed_action_state.speed_command = 0.0f;
    subject_2_fixed_action_state.progress_distance_mark = 0.0f;
    subject_2_fixed_action_state.last_control_ms = 0;
    subject_2_fixed_action_state.last_progress_ms = 0;
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
    subject_2_fixed_action_state.phase_distance_m = 0.0f;
    subject_2_fixed_action_state.yaw_last = subject_2_fixed_is_encoder_yaw_action(mode) ? Yaw_Straight_1 : Yaw_1;
    subject_2_fixed_action_state.yaw_delta = 0.0f;
    subject_2_fixed_action_state.yaw_target = subject_2_fixed_is_encoder_yaw_action(mode) ? Yaw_Straight_1 : Yaw_1;
    subject_2_fixed_action_state.turn_phase = subject_2_fixed_is_turn_action(mode) ? SUBJECT_2_FIXED_TURN_PHASE_STRAIGHT : SUBJECT_2_FIXED_TURN_PHASE_NONE;
    subject_2_fixed_action_state.encoder_last_count = l_ecdcounter();
    subject_2_fixed_action_state.encoder_delta_last = 0;
    subject_2_fixed_action_state.yaw_error_last = 0.0f;
    subject_2_fixed_action_state.yaw_error_integral = 0.0f;
    subject_2_fixed_action_state.steer_command = 0.0f;
    subject_2_fixed_action_state.speed_command = 0.0f;
    subject_2_fixed_action_state.progress_distance_mark = 0.0f;
    subject_2_fixed_action_state.last_control_ms = subject_2_fixed_action_state.start_ms;
    subject_2_fixed_action_state.last_progress_ms = subject_2_fixed_action_state.start_ms;
    conrtol_mode = GUANDAO;
    if(subject_2_fixed_is_encoder_yaw_action(mode))
    {
        subject_2_encoder_yaw_log("START", "NONE", 1U);
    }
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
    if(subject_2_fixed_is_encoder_yaw_action(subject_2_fixed_action_state.mode))
    {
        subject_2_encoder_yaw_task(now_ms);
        return;
    }

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
    subject_2_fixed_log_state("RUN");

    if(!subject_2_fixed_is_encoder_yaw_action(subject_2_fixed_action_state.mode))
    {
        subject_2_fixed_action_state.distance_m += subject_2_fixed_abs_float(speed_mps) * dt_s;
        subject_2_fixed_action_state.phase_distance_m += subject_2_fixed_abs_float(speed_mps) * dt_s;
    }
    subject_2_fixed_apply(speed_mps, steer_deg);

    subject_2_fixed_action_update(subject_2_fixed_action_state.mode, elapsed_ms, subject_2_fixed_action_state.distance_m, subject_2_fixed_action_state.yaw_delta, subject_2_fixed_action_state.yaw_target, Yaw_1, &fixed_action_output);
    if(fixed_action_output.finished)
    {
        subject_2_fixed_log_state("STOP");
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
