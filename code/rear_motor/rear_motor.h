#ifndef CODE_REAR_MOTOR_H_
#define CODE_REAR_MOTOR_H_

#include "zf_common_headfile.h"
#include "rear_encoder_calibration.h"

#define REAR_WHEEL_DIAMETER_M        0.24f
#define REAR_GEAR_RATIO              1.6f
#define REAR_ENCODER_PPR             1024
#define REAR_EFFECTIVE_PPR           ((float)REAR_ENCODER_PPR * REAR_GEAR_RATIO)
#define REAR_ENCODER_FEEDBACK_DIRECTION (-1)
#define REAR_WHEEL_CIRCUM_M          (3.14159265358979323846f * REAR_WHEEL_DIAMETER_M)

#define REAR_KP                 4.0f
#define REAR_KI                 0.6f
#define REAR_KD                 0.12f
#define REAR_FF_GAIN            8.0f
#define REAR_HIGH_SPEED_FF_START_MPS 2.5f
#define REAR_HIGH_SPEED_FF_GAIN 500.0f

#define REAR_PWM_HARD_LIMIT     9500
#define REAR_PWM_RATE_LIMIT     600
#define REAR_DIFF_PWM_GAIN      600.0f
#define REAR_REVERSE_PWM_MIN    1800
#define REAR_INTEGRAL_LIMIT     2000.0f
#define REAR_INTEGRAL_THRESHOLD 500.0f
#define REAR_ENCODER_DELTA_ABS_MAX 300
#define REAR_SPEED_FILTER_ALPHA 0.15f
#define REAR_SPEED_LIMIT_SOFT_ZONE_MPS 0.20f

#define REAR_BRAKE_TIMEOUT_MS 700u
#define REAR_BRAKE_HIGH_REVERSE_GUARD_MS 600u
#define REAR_BRAKE_HIGH_SPEED_MPS 3.75f
#define REAR_BRAKE_STOP_SPEED_MPS 0.05f
#define REAR_BRAKE_REVERSE_MPS 0.05f
#define REAR_BRAKE_PWM_HIGH 2500
#define REAR_BRAKE_PWM_MID 1800
#define REAR_BRAKE_PWM_LOW 1000
#define REAR_BRAKE_SYSTEM_MS_WRAP 42950u

#define REAR_BRAKE_REASON_NONE       0u
#define REAR_BRAKE_REASON_LOW_SPEED  1u
#define REAR_BRAKE_REASON_REVERSE    2u
#define REAR_BRAKE_REASON_TIMEOUT    3u

#define REAR_SPEED_MAX_MPS      5.0f
#define REAR_SPEED_MIN_MPS      -5.0f

void rear_motor_init(void);
void rear_motor_stop(void);

void rear_motor_brake_start(void);
void rear_motor_brake_update(void);
uint8 rear_motor_brake_active(void);
uint8 rear_motor_brake_reason(void);
uint32 rear_motor_brake_elapsed_ms(void);
int16 rear_motor_brake_pwm(void);
float rear_motor_brake_end_raw_mps(void);

void rear_motor_set_full_power(void);
void rear_motor_set_target_mps(float target_mps);
void rear_motor_set_speed_limit_mps(float limit_mps);
void rear_motor_clear_speed_limit(void);
void rear_motor_encoder_update_10ms(float yaw_deg);
void rear_motor_discard_odometry_samples(void);
uint8 rear_motor_take_odometry_sample(int32 *pulses, float *yaw_deg);
uint32 rear_motor_get_odometry_merged_samples(void);
int32 rear_motor_get_odometry_total_pulses(void);
uint8 rear_motor_get_odometry_pending_samples(void);
void rear_motor_pid_update_100ms(void);
void rear_motor_open_loop_update(int16 pwm);

float rear_motor_get_target_mps(void);
float rear_motor_get_speed_mps(void);
float rear_motor_get_raw_speed_mps(void);
float rear_motor_get_error_pulses(void);
float rear_motor_get_integral_pulses(void);
int16 rear_motor_get_pwm(void);
int16 rear_motor_get_encoder_10ms(void);
int32 rear_motor_get_encoder_100ms(void);

#endif /* CODE_REAR_MOTOR_H_ */
