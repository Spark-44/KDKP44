#include "subject_2_gyro_route.h"
#include "zf_common_headfile.h"
#include "rear_motor/rear_motor.h"

#define SUBJECT_2_GYRO_ROUTE_SPEED_MPS         (1.0f)
#define SUBJECT_2_GYRO_ROUTE_MIN_SPEED_MPS     (0.35f)
#define SUBJECT_2_GYRO_ROUTE_SLOWDOWN_M        (1.0f)
#define SUBJECT_2_GYRO_ROUTE_KP                (0.80f)
#define SUBJECT_2_GYRO_ROUTE_KI                (0.20f)
#define SUBJECT_2_GYRO_ROUTE_KD                (0.04f)
#define SUBJECT_2_GYRO_ROUTE_INTEGRAL_LIMIT    (20.0f)
#define SUBJECT_2_GYRO_ROUTE_STEER_LIMIT_DEG   (25.0f)
#define SUBJECT_2_GYRO_ROUTE_STEER_RATE_DEG    (0.50f)
#define SUBJECT_2_GYRO_ROUTE_CONTROL_MS        (20U)
#define SUBJECT_2_GYRO_ROUTE_LOG_MS            (500U)
#define SUBJECT_2_GYRO_ROUTE_STALL_MS          (3000U)
#define SUBJECT_2_GYRO_ROUTE_PROGRESS_M        (0.002f)
#define SUBJECT_2_GYRO_ROUTE_MAX_ENCODER_DELTA 1000

typedef struct
{
    float distance_m;
    float relative_yaw_deg;
} subject_2_gyro_route_sample_t;

typedef struct
{
    const subject_2_gyro_route_sample_t *profile;
    uint16 sample_count;
    uint16 sample_index;
    uint8 route_number;
    uint8 reverse;
    uint8 active;
    int16 encoder_last_count;
    int32 encoder_delta_last;
    float distance_m;
    float start_yaw;
    float target_yaw;
    float yaw_error_last;
    float yaw_error_integral;
    float steer_command;
    float progress_distance_mark;
    uint32 last_control_ms;
    uint32 last_progress_ms;
    uint32 last_log_ms;
} subject_2_gyro_route_state_t;

static subject_2_gyro_route_state_t subject_2_gyro_route_state;

#define GYRO_ROUTE_13_SAMPLE(distance, yaw) { (distance), (yaw) }
#define GYRO_ROUTE_14_SAMPLE(distance, yaw) { (distance), (yaw) }

static const subject_2_gyro_route_sample_t subject_2_gyro_route_13[] =
{
    GYRO_ROUTE_13_SAMPLE(0.0000f, 0.00f),
    GYRO_ROUTE_13_SAMPLE(0.4001f, -7.67f),
    GYRO_ROUTE_13_SAMPLE(0.8080f, -15.79f),
    GYRO_ROUTE_13_SAMPLE(1.2133f, -18.34f),
    GYRO_ROUTE_13_SAMPLE(1.6281f, -16.80f),
    GYRO_ROUTE_13_SAMPLE(2.0307f, -13.78f),
    GYRO_ROUTE_13_SAMPLE(2.4467f, -10.14f),
    GYRO_ROUTE_13_SAMPLE(2.8498f, -5.68f),
    GYRO_ROUTE_13_SAMPLE(3.2598f, 2.18f),
    GYRO_ROUTE_13_SAMPLE(3.6560f, 12.35f),
    GYRO_ROUTE_13_SAMPLE(4.0641f, 20.13f),
    GYRO_ROUTE_13_SAMPLE(4.4665f, 28.09f),
    GYRO_ROUTE_13_SAMPLE(4.8715f, 34.53f),
    GYRO_ROUTE_13_SAMPLE(5.2855f, 33.14f),
    GYRO_ROUTE_13_SAMPLE(5.6937f, 26.88f),
    GYRO_ROUTE_13_SAMPLE(6.1100f, 18.57f),
    GYRO_ROUTE_13_SAMPLE(6.5223f, 7.50f),
    GYRO_ROUTE_13_SAMPLE(6.9234f, -3.64f),
    GYRO_ROUTE_13_SAMPLE(7.3279f, -14.26f),
    GYRO_ROUTE_13_SAMPLE(7.7359f, -23.22f),
    GYRO_ROUTE_13_SAMPLE(8.1431f, -28.39f),
    GYRO_ROUTE_13_SAMPLE(8.5549f, -30.44f),
    GYRO_ROUTE_13_SAMPLE(8.9709f, -29.80f),
    GYRO_ROUTE_13_SAMPLE(9.3779f, -25.53f),
    GYRO_ROUTE_13_SAMPLE(9.7923f, -17.70f),
    GYRO_ROUTE_13_SAMPLE(10.2002f, -8.11f),
    GYRO_ROUTE_13_SAMPLE(10.6103f, 2.34f),
    GYRO_ROUTE_13_SAMPLE(11.0164f, 12.20f),
    GYRO_ROUTE_13_SAMPLE(11.4370f, 20.46f),
    GYRO_ROUTE_13_SAMPLE(11.8442f, 24.20f),
    GYRO_ROUTE_13_SAMPLE(12.2425f, 27.10f),
    GYRO_ROUTE_13_SAMPLE(12.6584f, 23.09f),
    GYRO_ROUTE_13_SAMPLE(13.0506f, 13.99f),
    GYRO_ROUTE_13_SAMPLE(13.4781f, 2.93f),
    GYRO_ROUTE_13_SAMPLE(13.8982f, -8.19f),
    GYRO_ROUTE_13_SAMPLE(14.2985f, -18.44f),
    GYRO_ROUTE_13_SAMPLE(14.7065f, -19.28f),
    GYRO_ROUTE_13_SAMPLE(15.1188f, -9.73f),
    GYRO_ROUTE_13_SAMPLE(15.5290f, 4.00f),
    GYRO_ROUTE_13_SAMPLE(15.9449f, 9.08f),
    GYRO_ROUTE_13_SAMPLE(16.3480f, 0.87f),
    GYRO_ROUTE_13_SAMPLE(16.7480f, -2.19f),
};

static const subject_2_gyro_route_sample_t subject_2_gyro_route_14[] =
{
    GYRO_ROUTE_14_SAMPLE(0.0000f, 0.00f),
    GYRO_ROUTE_14_SAMPLE(0.4105f, 3.69f),
    GYRO_ROUTE_14_SAMPLE(0.8235f, 9.91f),
    GYRO_ROUTE_14_SAMPLE(1.2261f, 15.63f),
    GYRO_ROUTE_14_SAMPLE(1.6405f, 22.61f),
    GYRO_ROUTE_14_SAMPLE(2.0386f, 25.30f),
    GYRO_ROUTE_14_SAMPLE(2.4591f, 22.85f),
    GYRO_ROUTE_14_SAMPLE(2.8584f, 19.16f),
    GYRO_ROUTE_14_SAMPLE(3.2695f, 15.32f),
    GYRO_ROUTE_14_SAMPLE(3.6721f, 11.19f),
    GYRO_ROUTE_14_SAMPLE(4.0880f, 6.82f),
    GYRO_ROUTE_14_SAMPLE(4.5099f, 1.65f),
    GYRO_ROUTE_14_SAMPLE(4.9100f, -4.54f),
    GYRO_ROUTE_14_SAMPLE(5.3330f, -10.75f),
    GYRO_ROUTE_14_SAMPLE(5.7430f, -17.70f),
    GYRO_ROUTE_14_SAMPLE(6.1608f, -25.10f),
    GYRO_ROUTE_14_SAMPLE(6.5768f, -31.33f),
    GYRO_ROUTE_14_SAMPLE(6.9817f, -35.09f),
    GYRO_ROUTE_14_SAMPLE(7.3840f, -35.10f),
    GYRO_ROUTE_14_SAMPLE(7.7974f, -30.12f),
    GYRO_ROUTE_14_SAMPLE(8.2046f, -23.16f),
    GYRO_ROUTE_14_SAMPLE(8.6157f, -15.92f),
    GYRO_ROUTE_14_SAMPLE(9.0334f, -8.50f),
    GYRO_ROUTE_14_SAMPLE(9.4545f, -1.33f),
    GYRO_ROUTE_14_SAMPLE(9.8656f, 6.06f),
    GYRO_ROUTE_14_SAMPLE(10.2815f, 13.13f),
    GYRO_ROUTE_14_SAMPLE(10.7087f, 22.75f),
    GYRO_ROUTE_14_SAMPLE(11.1158f, 29.26f),
    GYRO_ROUTE_14_SAMPLE(11.5377f, 32.13f),
    GYRO_ROUTE_14_SAMPLE(11.9511f, 30.06f),
    GYRO_ROUTE_14_SAMPLE(12.3542f, 27.34f),
    GYRO_ROUTE_14_SAMPLE(12.7656f, 22.45f),
    GYRO_ROUTE_14_SAMPLE(13.1673f, 14.35f),
    GYRO_ROUTE_14_SAMPLE(13.5773f, 8.65f),
    GYRO_ROUTE_14_SAMPLE(14.0002f, 2.90f),
    GYRO_ROUTE_14_SAMPLE(14.4102f, -4.98f),
    GYRO_ROUTE_14_SAMPLE(14.8332f, -11.40f),
    GYRO_ROUTE_14_SAMPLE(15.2358f, -16.90f),
    GYRO_ROUTE_14_SAMPLE(15.6469f, -22.77f),
    GYRO_ROUTE_14_SAMPLE(16.0592f, -23.51f),
    GYRO_ROUTE_14_SAMPLE(16.4830f, -17.71f),
    GYRO_ROUTE_14_SAMPLE(16.8953f, -10.98f),
    GYRO_ROUTE_14_SAMPLE(17.2973f, -4.45f),
    GYRO_ROUTE_14_SAMPLE(17.7074f, 1.23f),
};

static float subject_2_gyro_route_abs(float value)
{
    return value < 0.0f ? -value : value;
}

static float subject_2_gyro_route_normalize_angle(float angle)
{
    while(angle > 180.0f) angle -= 360.0f;
    while(angle < -180.0f) angle += 360.0f;
    return angle;
}

static void subject_2_gyro_route_append_fixed100(char *line, int *position, float value)
{
    int32 scaled = (int32)(value * 100.0f);
    if(scaled < 0)
    {
        line[(*position)++] = '-';
        scaled = -scaled;
    }
    *position += sprintf(&line[*position], "%ld.%02ld", (long)(scaled / 100), (long)(scaled % 100));
}

static float subject_2_gyro_route_interpolate_yaw(float distance_m)
{
    const subject_2_gyro_route_sample_t *a;
    const subject_2_gyro_route_sample_t *b;
    float ratio;

    if(subject_2_gyro_route_state.profile == 0 || subject_2_gyro_route_state.sample_count == 0U)
    {
        return 0.0f;
    }
    if(distance_m <= subject_2_gyro_route_state.profile[0].distance_m)
    {
        subject_2_gyro_route_state.sample_index = 0U;
        return subject_2_gyro_route_state.profile[0].relative_yaw_deg;
    }
    if(distance_m >= subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_count - 1U].distance_m)
    {
        subject_2_gyro_route_state.sample_index = subject_2_gyro_route_state.sample_count - 1U;
        return subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_count - 1U].relative_yaw_deg;
    }

    while(subject_2_gyro_route_state.sample_index + 1U < subject_2_gyro_route_state.sample_count
            && distance_m > subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_index + 1U].distance_m)
    {
        subject_2_gyro_route_state.sample_index++;
    }
    while(subject_2_gyro_route_state.sample_index > 0U
            && distance_m < subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_index].distance_m)
    {
        subject_2_gyro_route_state.sample_index--;
    }

    a = &subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_index];
    b = &subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_index + 1U];
    ratio = (distance_m - a->distance_m) / (b->distance_m - a->distance_m);
    return a->relative_yaw_deg + ratio * (b->relative_yaw_deg - a->relative_yaw_deg);
}

static float subject_2_gyro_route_total_distance(void)
{
    if(subject_2_gyro_route_state.profile == 0 || subject_2_gyro_route_state.sample_count == 0U)
    {
        return 0.0f;
    }
    return subject_2_gyro_route_state.profile[subject_2_gyro_route_state.sample_count - 1U].distance_m;
}

static void subject_2_gyro_route_log(const char *tag, const char *reason, uint8 force)
{
    uint32 now_ms = system_getval_ms();
    float error = subject_2_gyro_route_normalize_angle(subject_2_gyro_route_state.target_yaw - Yaw_Straight_1);
    char line[320];
    int position = 0;

    if(!force && (uint32)(now_ms - subject_2_gyro_route_state.last_log_ms) < SUBJECT_2_GYRO_ROUTE_LOG_MS)
    {
        return;
    }
    subject_2_gyro_route_state.last_log_ms = now_ms;
    position += sprintf(line, "[GYRO-ROUTE] %s route=%u dir=%s dist=", tag,
            (unsigned)subject_2_gyro_route_state.route_number,
            subject_2_gyro_route_state.reverse ? "REV" : "FWD");
    subject_2_gyro_route_append_fixed100(line, &position, subject_2_gyro_route_state.distance_m);
    position += sprintf(&line[position], " total=");
    subject_2_gyro_route_append_fixed100(line, &position, subject_2_gyro_route_total_distance());
    position += sprintf(&line[position], " target=");
    subject_2_gyro_route_append_fixed100(line, &position, subject_2_gyro_route_state.target_yaw);
    position += sprintf(&line[position], " yaw=");
    subject_2_gyro_route_append_fixed100(line, &position, Yaw_Straight_1);
    position += sprintf(&line[position], " err=");
    subject_2_gyro_route_append_fixed100(line, &position, error);
    position += sprintf(&line[position], " steer=");
    subject_2_gyro_route_append_fixed100(line, &position, subject_2_gyro_route_state.steer_command);
    sprintf(&line[position], " enc=%ld reason=%s\r\n",
            (long)subject_2_gyro_route_state.encoder_delta_last,
            reason == 0 ? "NONE" : reason);
    uart_write_string(DEBUG_UART_INDEX, line);
}

static void subject_2_gyro_route_clear_outputs(void)
{
    out_v_l = 0.0f;
    out_v_r = 0.0f;
    out_servo = 0.0f;
    rear_motor_set_target_mps(0.0f);
}

static void subject_2_gyro_route_finish(const char *reason)
{
    subject_2_gyro_route_log("STOP", reason, 1U);
    subject_2_gyro_route_state.active = 0U;
    subject_2_gyro_route_clear_outputs();
}

static void subject_2_gyro_route_update_steering(uint32 now_ms)
{
    uint32 elapsed_ms = now_ms - subject_2_gyro_route_state.last_control_ms;

    if(elapsed_ms >= SUBJECT_2_GYRO_ROUTE_CONTROL_MS)
    {
        float dt_s = (float)elapsed_ms * 0.001f;
        float yaw_error = subject_2_gyro_route_normalize_angle(
                subject_2_gyro_route_state.target_yaw - Yaw_Straight_1);
        float yaw_error_delta = subject_2_gyro_route_normalize_angle(
                yaw_error - subject_2_gyro_route_state.yaw_error_last);
        float steer_command;
        float steer_delta;

        subject_2_gyro_route_state.yaw_error_integral += yaw_error * dt_s;
        Value_Limit_float(&subject_2_gyro_route_state.yaw_error_integral,
                -SUBJECT_2_GYRO_ROUTE_INTEGRAL_LIMIT,
                SUBJECT_2_GYRO_ROUTE_INTEGRAL_LIMIT);
        steer_command = -(SUBJECT_2_GYRO_ROUTE_KP * yaw_error
                + SUBJECT_2_GYRO_ROUTE_KI * subject_2_gyro_route_state.yaw_error_integral
                + SUBJECT_2_GYRO_ROUTE_KD * yaw_error_delta / dt_s);
        if(subject_2_gyro_route_state.reverse)
        {
            steer_command = -steer_command;
        }
        Value_Limit_float(&steer_command,
                -SUBJECT_2_GYRO_ROUTE_STEER_LIMIT_DEG,
                SUBJECT_2_GYRO_ROUTE_STEER_LIMIT_DEG);
        steer_delta = steer_command - subject_2_gyro_route_state.steer_command;
        Value_Limit_float(&steer_delta,
                -SUBJECT_2_GYRO_ROUTE_STEER_RATE_DEG,
                SUBJECT_2_GYRO_ROUTE_STEER_RATE_DEG);
        subject_2_gyro_route_state.steer_command += steer_delta;
        subject_2_gyro_route_state.yaw_error_last = yaw_error;
        subject_2_gyro_route_state.last_control_ms = now_ms;
    }
}

void subject_2_gyro_route_start(uint8 route_number, uint8 reverse)
{
    uint32 now_ms;

    subject_2_gyro_route_stop("RESTART");
    if(route_number != SUBJECT_2_GYRO_ROUTE_13 && route_number != SUBJECT_2_GYRO_ROUTE_14)
    {
        uart_write_string(DEBUG_UART_INDEX, "[GYRO-ROUTE] REJECT reason=ROUTE\r\n");
        return;
    }
    if(route_number == SUBJECT_2_GYRO_ROUTE_13 && !reverse)
    {
        uart_write_string(DEBUG_UART_INDEX, "[GYRO-ROUTE] REJECT route=13 reason=DIRECTION\r\n");
        return;
    }

    memset(&subject_2_gyro_route_state, 0, sizeof(subject_2_gyro_route_state));
    subject_2_gyro_route_state.route_number = route_number;
    subject_2_gyro_route_state.reverse = reverse ? 1U : 0U;
    if(route_number == SUBJECT_2_GYRO_ROUTE_13)
    {
        subject_2_gyro_route_state.profile = subject_2_gyro_route_13;
        subject_2_gyro_route_state.sample_count = sizeof(subject_2_gyro_route_13) / sizeof(subject_2_gyro_route_13[0]);
    }
    else
    {
        subject_2_gyro_route_state.profile = subject_2_gyro_route_14;
        subject_2_gyro_route_state.sample_count = sizeof(subject_2_gyro_route_14) / sizeof(subject_2_gyro_route_14[0]);
    }
    now_ms = system_getval_ms();
    subject_2_gyro_route_state.encoder_last_count = l_ecdcounter();
    subject_2_gyro_route_state.start_yaw = Yaw_Straight_1;
    subject_2_gyro_route_state.target_yaw = Yaw_Straight_1;
    subject_2_gyro_route_state.last_control_ms = now_ms;
    subject_2_gyro_route_state.last_progress_ms = now_ms;
    subject_2_gyro_route_state.last_log_ms = now_ms;
    subject_2_gyro_route_state.active = 1U;
    conrtol_mode = GUANDAO;
    subject_2_gyro_route_log("START", "NONE", 1U);
}

void subject_2_gyro_route_stop(const char *reason)
{
    if(!subject_2_gyro_route_state.active)
    {
        return;
    }
    subject_2_gyro_route_finish(reason == 0 ? "COMMAND" : reason);
}

void subject_2_gyro_route_task(void)
{
    uint32 now_ms;
    int16 encoder_now;
    int32 encoder_delta;
    float distance_step = 0.0f;
    float total_distance;
    float remaining;
    float speed_mps;

    if(!subject_2_gyro_route_state.active)
    {
        return;
    }
    now_ms = system_getval_ms();
    encoder_now = l_ecdcounter();
    encoder_delta = calculate_delta(encoder_now, subject_2_gyro_route_state.encoder_last_count);
    subject_2_gyro_route_state.encoder_last_count = encoder_now;
    subject_2_gyro_route_state.encoder_delta_last = encoder_delta;
    if(encoder_delta <= SUBJECT_2_GYRO_ROUTE_MAX_ENCODER_DELTA
            && encoder_delta >= -SUBJECT_2_GYRO_ROUTE_MAX_ENCODER_DELTA)
    {
        distance_step = subject_2_gyro_route_abs((float)encoder_delta) * ONE_TICK_DISTANCE;
        subject_2_gyro_route_state.distance_m += distance_step;
        if(subject_2_gyro_route_state.distance_m - subject_2_gyro_route_state.progress_distance_mark
                >= SUBJECT_2_GYRO_ROUTE_PROGRESS_M)
        {
            subject_2_gyro_route_state.progress_distance_mark = subject_2_gyro_route_state.distance_m;
            subject_2_gyro_route_state.last_progress_ms = now_ms;
        }
    }

    total_distance = subject_2_gyro_route_total_distance();
    if(subject_2_gyro_route_state.distance_m >= total_distance)
    {
        subject_2_gyro_route_finish("DISTANCE");
        return;
    }
    if((uint32)(now_ms - subject_2_gyro_route_state.last_progress_ms) >= SUBJECT_2_GYRO_ROUTE_STALL_MS)
    {
        subject_2_gyro_route_finish("STALL");
        return;
    }

    subject_2_gyro_route_state.target_yaw = subject_2_gyro_route_normalize_angle(
            subject_2_gyro_route_state.start_yaw
            + subject_2_gyro_route_interpolate_yaw(subject_2_gyro_route_state.distance_m));
    subject_2_gyro_route_update_steering(now_ms);

    remaining = total_distance - subject_2_gyro_route_state.distance_m;
    speed_mps = SUBJECT_2_GYRO_ROUTE_SPEED_MPS;
    if(remaining < SUBJECT_2_GYRO_ROUTE_SLOWDOWN_M)
    {
        speed_mps = SUBJECT_2_GYRO_ROUTE_MIN_SPEED_MPS
                + (SUBJECT_2_GYRO_ROUTE_SPEED_MPS - SUBJECT_2_GYRO_ROUTE_MIN_SPEED_MPS)
                * remaining / SUBJECT_2_GYRO_ROUTE_SLOWDOWN_M;
    }
    speed_mps = subject_2_gyro_route_state.reverse ? -speed_mps : speed_mps;
    conrtol_mode = GUANDAO;
    out_v_l = speed_mps / GUANDAO_SPEED_TO_MPS;
    out_v_r = speed_mps / GUANDAO_SPEED_TO_MPS;
    out_servo = subject_2_gyro_route_state.steer_command;
    subject_2_gyro_route_log("RUN", "NONE", 0U);
}

uint8 subject_2_gyro_route_is_active(void)
{
    return subject_2_gyro_route_state.active;
}
