

#ifndef CODE_ANGLE_CONTROL_H_
#define CODE_ANGLE_CONTROL_H_
#include "zf_common_headfile.h"
#include "PID.h"

#define ANGLE_PWM_IN1       ATOM0_CH7_P02_7        
#define ANGLE_PWM_IN2       ATOM0_CH6_P02_6        

#define ANGLE_ENCODER       TIM4_ENCODER
#define ANGLE_ENCODER_A_PIN TIM4_ENCODER_CH1_P02_8 
#define ANGLE_ENCODER_B_PIN TIM4_ENCODER_CH2_P00_9

#define ANGLE_PPR           1024                   
#define ANGLE_GEAR_RATIO    300                    
#define ANGLE_MAX_DEGREE    60                     
#define ANGLE_MIN_DEGREE    -60
#define ANGLE_DEFAULT_KP    200.0f
#define ANGLE_DEFAULT_KI    4.0f
#define ANGLE_DEFAULT_KD    10.0f
#define ANGLE_OUTPUT_MAX    10000
#define ANGLE_DEAD_BAND     0.1f

typedef struct {
    PID_TypeDef pid;          
    float target_angle;
    float current_angle;      
    int32 encoder_zero_count; 
    uint32 control_count;     
} AngleControl_TypeDef;

extern AngleControl_TypeDef angle_ctrl;
extern int32 accumulated_encoder_count;

void angle_control_init(void);

void angle_control_update(void);

void angle_control_set_target(int32 target_angle);

void angle_control_rotate_relative(int32 delta_angle);

void angle_motor_set_pwm(int32 pwm_value);

int32 angle_control_get_current_angle(void);

void angle_control_reset(void);

#endif /* CODE_ANGLE_CONTROL_H_ */
