#include "zf_common_headfile.h"
#include "rear_motor/rear_motor.h"
#include "rear_motor/rear_odometry_pose_buffer.h"
#include "control.h"

extern float out_v_l;
extern float out_v_r;

static float target_mps = 0.0f;
static float actual_mps = 0.0f;
static float raw_actual_mps = 0.0f;
static float filtered_pulses_100ms = 0.0f;
static uint8 speed_filter_initialized = 0;
static int16 current_pwm = 0;
static int16 encoder_10ms = 0;
static int32 encoder_100ms_last = 0;
static int16 last_encoder_count = 0;
static uint8 encoder_first_read = 1;

static volatile int32 speed_window_build_pulses = 0;
static volatile uint8 speed_window_build_samples = 0;
static volatile int32 speed_window_ready_pulses = 0;
static volatile uint16 speed_window_ready_count = 0;
static volatile int32 odometry_total_pulses = 0;
static rear_odometry_pose_buffer_t odometry_pose_buffer;

static float integral = 0.0f;
static float last_error = 0.0f;
static int last_pwm = 0;
static int16 applied_pwm_l = 0;
static int16 applied_pwm_r = 0;
static float speed_limit_mps = 0.0f;

static uint8 brake_active = 0;
static uint8 brake_exit_reason = REAR_BRAKE_REASON_NONE;
static uint32 brake_start_ms = 0;
static uint32 brake_elapsed_ms = 0;
static int16 brake_output_pwm = 0;
static float brake_start_speed_mps = 0.0f;
static float brake_end_raw_mps = 0.0f;

static void rear_motor_write_wheel(
        pwm_channel_enum forward, pwm_channel_enum reverse,
        int16 pwm, int16 previous_pwm)
{
    if((pwm > 0 && previous_pwm < 0) || (pwm < 0 && previous_pwm > 0))
    {
        pwm_set_duty(forward, 0);
        pwm_set_duty(reverse, 0);
    }

    if(pwm > 0)
    {
        pwm_set_duty(reverse, 0);
        pwm_set_duty(forward, (uint32)pwm);
    }
    else if(pwm < 0)
    {
        pwm_set_duty(forward, 0);
        pwm_set_duty(reverse, (uint32)(-pwm));
    }
    else
    {
        pwm_set_duty(forward, 0);
        pwm_set_duty(reverse, 0);
    }
}

static void rear_motor_set_pwm(int16 pwm)
{
    int diff = (int)pwm - last_pwm;
    int16 pwm_l;
    int16 pwm_r;

    if(diff > REAR_PWM_RATE_LIMIT) diff = REAR_PWM_RATE_LIMIT;
    if(diff < -REAR_PWM_RATE_LIMIT) diff = -REAR_PWM_RATE_LIMIT;
    last_pwm += diff;
    if(last_pwm > REAR_PWM_HARD_LIMIT) last_pwm = REAR_PWM_HARD_LIMIT;
    if(last_pwm < -REAR_PWM_HARD_LIMIT) last_pwm = -REAR_PWM_HARD_LIMIT;

    current_pwm = (int16)last_pwm;
    pwm_l = current_pwm;
    pwm_r = current_pwm;

    if(conrtol_mode == GUANDAO)
    {
        int16 differential = (int16)((out_v_l - out_v_r) * REAR_DIFF_PWM_GAIN);
        if(differential > 1500) differential = 1500;
        if(differential < -1500) differential = -1500;
        pwm_l = (int16)(pwm_l + differential);
        pwm_r = (int16)(pwm_r - differential);
    }

    if(pwm_l > REAR_PWM_HARD_LIMIT) pwm_l = REAR_PWM_HARD_LIMIT;
    if(pwm_l < -REAR_PWM_HARD_LIMIT) pwm_l = -REAR_PWM_HARD_LIMIT;
    if(pwm_r > REAR_PWM_HARD_LIMIT) pwm_r = REAR_PWM_HARD_LIMIT;
    if(pwm_r < -REAR_PWM_HARD_LIMIT) pwm_r = -REAR_PWM_HARD_LIMIT;

    rear_motor_write_wheel(PWM_L1, PWM_L2, pwm_l, applied_pwm_l);
    rear_motor_write_wheel(PWM_R1, PWM_R2, pwm_r, applied_pwm_r);
    applied_pwm_l = pwm_l;
    applied_pwm_r = pwm_r;
}

static void rear_motor_reset_controller(void)
{
    integral = 0.0f;
    last_error = 0.0f;
    last_pwm = 0;
}

void rear_motor_init(void)
{
    pwm_init(PWM_L1, 17000, 0);
    pwm_init(PWM_L2, 17000, 0);
    pwm_init(PWM_R1, 17000, 0);
    pwm_init(PWM_R2, 17000, 0);

    target_mps = 0.0f;
    actual_mps = 0.0f;
    raw_actual_mps = 0.0f;
    filtered_pulses_100ms = 0.0f;
    speed_filter_initialized = 0;
    current_pwm = 0;
    encoder_10ms = 0;
    encoder_100ms_last = 0;
    speed_window_build_pulses = 0;
    speed_window_build_samples = 0;
    speed_window_ready_pulses = 0;
    speed_window_ready_count = 0;
    odometry_total_pulses = 0;
    rear_odometry_pose_buffer_init(&odometry_pose_buffer);
    last_encoder_count = encoder_get_count(TIM2_ENCODER);
    encoder_first_read = 0;
    rear_motor_reset_controller();
    applied_pwm_l = 0;
    applied_pwm_r = 0;
    speed_limit_mps = 0.0f;
    brake_active = 0;
    brake_exit_reason = REAR_BRAKE_REASON_NONE;
    brake_start_ms = 0;
    brake_elapsed_ms = 0;
    brake_output_pwm = 0;
    brake_start_speed_mps = 0.0f;
    brake_end_raw_mps = 0.0f;
}

void rear_motor_stop(void)
{
    uint32 interrupt_state;

    brake_active = 0;
    brake_output_pwm = 0;
    target_mps = 0.0f;
    speed_limit_mps = 0.0f;
    rear_motor_reset_controller();
    encoder_100ms_last = 0;
    actual_mps = 0.0f;
    raw_actual_mps = 0.0f;
    filtered_pulses_100ms = 0.0f;
    speed_filter_initialized = 0;

    interrupt_state = interrupt_global_disable();
    encoder_10ms = 0;
    speed_window_build_pulses = 0;
    speed_window_build_samples = 0;
    speed_window_ready_pulses = 0;
    speed_window_ready_count = 0;
    interrupt_global_enable(interrupt_state);

    pwm_set_duty(PWM_L1, 0);
    pwm_set_duty(PWM_L2, 0);
    pwm_set_duty(PWM_R1, 0);
    pwm_set_duty(PWM_R2, 0);
    current_pwm = 0;
    applied_pwm_l = 0;
    applied_pwm_r = 0;
}

void rear_motor_set_full_power(void)
{
    brake_active = 0;
    target_mps = 0.0f;
    speed_limit_mps = 0.0f;
    integral = 0.0f;
    last_error = 0.0f;
    last_pwm = PWM_DUTY_MAX;
    current_pwm = PWM_DUTY_MAX;
    pwm_set_duty(PWM_L2, 0);
    pwm_set_duty(PWM_R2, 0);
    pwm_set_duty(PWM_L1, PWM_DUTY_MAX);
    pwm_set_duty(PWM_R1, PWM_DUTY_MAX);
    applied_pwm_l = PWM_DUTY_MAX;
    applied_pwm_r = PWM_DUTY_MAX;
}

void rear_motor_set_target_mps(float mps)
{
    if(mps > REAR_SPEED_MAX_MPS) mps = REAR_SPEED_MAX_MPS;
    if(mps < REAR_SPEED_MIN_MPS) mps = REAR_SPEED_MIN_MPS;
    target_mps = mps;
    if(mps == 0.0f) rear_motor_reset_controller();
}

void rear_motor_set_speed_limit_mps(float limit_mps)
{
    speed_limit_mps = (limit_mps < 0.0f) ? -limit_mps : limit_mps;
}

void rear_motor_clear_speed_limit(void)
{
    speed_limit_mps = 0.0f;
}

void rear_motor_encoder_update_10ms(float yaw_deg)
{
    int16 current_count = encoder_get_count(TIM2_ENCODER);
    int32 delta = 0;

    if(encoder_first_read)
    {
        last_encoder_count = current_count;
        encoder_10ms = 0;
        encoder_first_read = 0;
    }
    else
    {
        delta = (int32)REAR_ENCODER_FEEDBACK_DIRECTION
                * (int32)calculate_delta(current_count, last_encoder_count);
        last_encoder_count = current_count;
        if(delta > REAR_ENCODER_DELTA_ABS_MAX || delta < -REAR_ENCODER_DELTA_ABS_MAX)
        {
            encoder_10ms = 0;
        }
        else
        {
            encoder_10ms = (int16)delta;
            odometry_total_pulses += delta;
            rear_odometry_pose_buffer_add(&odometry_pose_buffer, delta, yaw_deg);
        }
    }

    speed_window_build_pulses += (int32)encoder_10ms;
    speed_window_build_samples++;
    if(speed_window_build_samples >= 10u)
    {
        speed_window_ready_pulses += speed_window_build_pulses;
        speed_window_ready_count++;
        speed_window_build_pulses = 0;
        speed_window_build_samples = 0;
    }
}

static uint8 rear_motor_take_speed_windows(int32 *pulses, uint16 *window_count)
{
    uint32 interrupt_state = interrupt_global_disable();
    uint8 available = (speed_window_ready_count > 0u);
    if(available)
    {
        *pulses = speed_window_ready_pulses;
        *window_count = speed_window_ready_count;
        speed_window_ready_pulses = 0;
        speed_window_ready_count = 0;
    }
    interrupt_global_enable(interrupt_state);
    return available;
}

static float rear_motor_filter_speed(float measured_pulses)
{
    raw_actual_mps = measured_pulses * REAR_ENCODER_METERS_PER_PULSE / 0.1f;
    if(!speed_filter_initialized)
    {
        filtered_pulses_100ms = measured_pulses;
        speed_filter_initialized = 1;
    }
    else
    {
        filtered_pulses_100ms += REAR_SPEED_FILTER_ALPHA
                * (measured_pulses - filtered_pulses_100ms);
    }
    actual_mps = filtered_pulses_100ms * REAR_ENCODER_METERS_PER_PULSE / 0.1f;
    return filtered_pulses_100ms;
}

void rear_motor_pid_update_100ms(void)
{
    int32 window_pulses;
    uint16 window_count;
    float measured_pulses;
    float target_pulses;
    float filtered_pulses;
    float error;
    float derivative;
    float ff;
    float pwm_f;
    float speed_limit_scale = 1.0f;

    if(!rear_motor_take_speed_windows(&window_pulses, &window_count)) return;
    measured_pulses = (float)window_pulses / (float)window_count;
    encoder_100ms_last = (int32)measured_pulses;
    filtered_pulses = rear_motor_filter_speed(measured_pulses);

    if(target_mps == 0.0f)
    {
        rear_motor_stop();
        return;
    }

    target_pulses = target_mps / REAR_ENCODER_METERS_PER_PULSE * 0.1f;
    error = target_pulses - filtered_pulses;
    if(error < REAR_INTEGRAL_THRESHOLD && error > -REAR_INTEGRAL_THRESHOLD)
    {
        integral += error * 0.1f;
        if(integral > REAR_INTEGRAL_LIMIT) integral = REAR_INTEGRAL_LIMIT;
        if(integral < -REAR_INTEGRAL_LIMIT) integral = -REAR_INTEGRAL_LIMIT;
    }

    derivative = (error - last_error) / 0.1f;
    last_error = error;
    ff = target_pulses * REAR_FF_GAIN;
    if(fabsf(target_mps) > REAR_HIGH_SPEED_FF_START_MPS)
    {
        float high_speed_ff = (fabsf(target_mps) - REAR_HIGH_SPEED_FF_START_MPS)
                * REAR_HIGH_SPEED_FF_GAIN;
        ff += (target_mps < 0.0f) ? -high_speed_ff : high_speed_ff;
    }

    if(speed_limit_mps > 0.0f)
    {
        float speed_abs = fabsf(actual_mps);
        if(target_mps > 0.0f && actual_mps >= speed_limit_mps) ff = 0.0f;
        if(target_mps < 0.0f && actual_mps <= -speed_limit_mps) ff = 0.0f;
        if(speed_abs > speed_limit_mps - REAR_SPEED_LIMIT_SOFT_ZONE_MPS)
        {
            speed_limit_scale = (speed_limit_mps - speed_abs)
                    / REAR_SPEED_LIMIT_SOFT_ZONE_MPS;
            if(speed_limit_scale < 0.0f) speed_limit_scale = 0.0f;
            if(speed_limit_scale > 1.0f) speed_limit_scale = 1.0f;
        }
    }
    pwm_f = ff + REAR_KP * error + REAR_KI * integral + REAR_KD * derivative;

    if(target_mps < -0.01f && pwm_f > -(float)REAR_REVERSE_PWM_MIN)
        pwm_f = -(float)REAR_REVERSE_PWM_MIN;

    if(speed_limit_mps > 0.0f)
    {
        if(target_mps > 0.0f && actual_mps >= speed_limit_mps && pwm_f > 0.0f)
            pwm_f = 0.0f;
        else if(target_mps > 0.0f && pwm_f > 0.0f)
            pwm_f *= speed_limit_scale;
        if(target_mps < 0.0f && actual_mps <= -speed_limit_mps && pwm_f < 0.0f)
            pwm_f = 0.0f;
        else if(target_mps < 0.0f && pwm_f < 0.0f)
            pwm_f *= speed_limit_scale;
    }
    rear_motor_set_pwm((int16)pwm_f);
}

void rear_motor_open_loop_update(int16 pwm)
{
    int32 window_pulses;
    uint16 window_count;
    if(rear_motor_take_speed_windows(&window_pulses, &window_count))
    {
        float measured_pulses = (float)window_pulses / (float)window_count;
        encoder_100ms_last = (int32)measured_pulses;
        rear_motor_filter_speed(measured_pulses);
    }
    target_mps = 0.0f;
    integral = 0.0f;
    last_error = 0.0f;
    rear_motor_set_pwm(pwm);
}

static uint32 rear_motor_brake_time_since(uint32 now_ms, uint32 start_ms)
{
    if(now_ms >= start_ms) return now_ms - start_ms;
    return (REAR_BRAKE_SYSTEM_MS_WRAP - start_ms) + now_ms;
}

static void rear_motor_brake_finish(uint8 reason, float raw_speed_mps)
{
    brake_exit_reason = reason;
    brake_end_raw_mps = raw_speed_mps;
    rear_motor_stop();
}

void rear_motor_brake_start(void)
{
    if(brake_active) return;
    brake_start_speed_mps = fabsf(actual_mps);
    brake_start_ms = system_getval_ms();
    brake_elapsed_ms = 0;
    brake_output_pwm = 0;
    brake_exit_reason = REAR_BRAKE_REASON_NONE;
    brake_end_raw_mps = 0.0f;
    target_mps = 0.0f;
    integral = 0.0f;
    last_error = 0.0f;

    if(brake_start_speed_mps <= REAR_BRAKE_STOP_SPEED_MPS
            && fabsf(raw_actual_mps) <= REAR_BRAKE_STOP_SPEED_MPS)
    {
        rear_motor_brake_finish(REAR_BRAKE_REASON_LOW_SPEED, raw_actual_mps);
        return;
    }
    brake_active = 1;
}

void rear_motor_brake_update(void)
{
    float raw_speed_mps;
    float abs_speed_mps;
    uint8 high_speed_guard;

    if(!brake_active) return;
    raw_speed_mps = raw_actual_mps;
    abs_speed_mps = fabsf(raw_speed_mps);
    brake_elapsed_ms = rear_motor_brake_time_since(system_getval_ms(), brake_start_ms);
    high_speed_guard = (brake_start_speed_mps >= REAR_BRAKE_HIGH_SPEED_MPS);

    if(brake_elapsed_ms >= REAR_BRAKE_TIMEOUT_MS)
    {
        rear_motor_brake_finish(REAR_BRAKE_REASON_TIMEOUT, raw_speed_mps);
        return;
    }
    if(raw_speed_mps < -REAR_BRAKE_REVERSE_MPS
            && (!high_speed_guard || brake_elapsed_ms >= REAR_BRAKE_HIGH_REVERSE_GUARD_MS))
    {
        rear_motor_brake_finish(REAR_BRAKE_REASON_REVERSE, raw_speed_mps);
        return;
    }
    if(brake_elapsed_ms >= 100u && abs_speed_mps <= REAR_BRAKE_STOP_SPEED_MPS)
    {
        rear_motor_brake_finish(REAR_BRAKE_REASON_LOW_SPEED, raw_speed_mps);
        return;
    }

    if(abs_speed_mps > 2.5f) brake_output_pwm = REAR_BRAKE_PWM_HIGH;
    else if(abs_speed_mps > 1.0f) brake_output_pwm = REAR_BRAKE_PWM_MID;
    else brake_output_pwm = REAR_BRAKE_PWM_LOW;
    rear_motor_open_loop_update((int16)-brake_output_pwm);
}

uint8 rear_motor_brake_active(void) { return brake_active; }
uint8 rear_motor_brake_reason(void) { return brake_exit_reason; }
uint32 rear_motor_brake_elapsed_ms(void) { return brake_elapsed_ms; }
int16 rear_motor_brake_pwm(void) { return brake_output_pwm; }
float rear_motor_brake_end_raw_mps(void) { return brake_end_raw_mps; }

uint8 rear_motor_take_odometry_sample(int32 *pulses, float *yaw_deg)
{
    uint32 interrupt_state = interrupt_global_disable();
    rear_odometry_pose_sample_t sample;
    uint8 available = rear_odometry_pose_buffer_take(&odometry_pose_buffer, &sample);
    interrupt_global_enable(interrupt_state);
    if(available)
    {
        *pulses = (int32)sample.pulses;
        *yaw_deg = sample.yaw_deg;
    }
    return available;
}

uint32 rear_motor_get_odometry_merged_samples(void) { return odometry_pose_buffer.merged_samples; }
int32 rear_motor_get_odometry_total_pulses(void) { return odometry_total_pulses; }
uint8 rear_motor_get_odometry_pending_samples(void) { return odometry_pose_buffer.count; }

float rear_motor_get_target_mps(void) { return target_mps; }
float rear_motor_get_speed_mps(void) { return actual_mps; }
float rear_motor_get_raw_speed_mps(void) { return raw_actual_mps; }
float rear_motor_get_error_pulses(void) { return last_error; }
float rear_motor_get_integral_pulses(void) { return integral; }
int16 rear_motor_get_pwm(void) { return current_pwm; }
int16 rear_motor_get_encoder_10ms(void) { return encoder_10ms; }
int32 rear_motor_get_encoder_100ms(void) { return encoder_100ms_last; }

int32 rear_motor_get_total_encoder_pulses(void) { return odometry_total_pulses; }
float rear_motor_get_total_distance_m(void)
{
    return (float)odometry_total_pulses * REAR_ENCODER_METERS_PER_PULSE;
}

void rear_motor_clear_odometer(void)
{
    uint32 interrupt_state = interrupt_global_disable();
    odometry_total_pulses = 0;
    rear_odometry_pose_buffer_init(&odometry_pose_buffer);
    interrupt_global_enable(interrupt_state);
}
