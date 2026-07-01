

#ifndef CODE_REAR_MOTOR_H_
#define CODE_REAR_MOTOR_H_

#include "zf_common_headfile.h"

#define REAR_WHEEL_DIAMETER_M        0.24f
#define REAR_GEAR_RATIO              1.6f
#define REAR_ENCODER_PPR             1024
#define REAR_EFFECTIVE_PPR           ((float)REAR_ENCODER_PPR * REAR_GEAR_RATIO)
#define REAR_WHEEL_CIRCUM_M          (3.14159265358979323846f * REAR_WHEEL_DIAMETER_M)

#define REAR_KP                 10.0f
#define REAR_KI                 0.3f
#define REAR_KD                 0.8f
#define REAR_FF_GAIN            13.0f

// #define REAR_KP              8.0f
// #define REAR_KI              0.5f
// #define REAR_KD              0.2f
// #define REAR_FF_GAIN         10.0f

// #define REAR_KP              6.5f
// #define REAR_KI              0.25f
// #define REAR_KD              0.3f
// #define REAR_FF_GAIN         9.0f
#define REAR_PWM_HARD_LIMIT     9500
#define REAR_PWM_RATE_LIMIT     1000
#define REAR_INTEGRAL_LIMIT     2000.0f
#define REAR_INTEGRAL_THRESHOLD 60.0f
#define REAR_ENCODER_DELTA_ABS_MAX 300

#define REAR_SPEED_MAX_MPS      5.0f
#define REAR_SPEED_MIN_MPS      -5.0f

void rear_motor_init(void);

void rear_motor_stop(void);

void rear_motor_set_full_power(void);

void rear_motor_set_target_mps(float target_mps);

void rear_motor_encoder_update_10ms(void);

void rear_motor_pid_update_100ms(void);

float  rear_motor_get_target_mps(void);

float  rear_motor_get_speed_mps(void);

int16  rear_motor_get_pwm(void);

int16  rear_motor_get_encoder_10ms(void);

int32  rear_motor_get_encoder_100ms(void);

#endif /* CODE_REAR_MOTOR_H_ */
