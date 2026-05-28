

#include "servo.h"

//

//

//

#define SERVO_MOTOR_DUTY(x)         ((float)PWM_DUTY_MAX/(1000.0/(float)SERVO_MOTOR_FREQ)*(0.5+(float)(x)/90.0))

static float servo_motor_duty = 90.0;                                           
static float servo_motor_dir = 1;                                               

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void servo_init (void)
{
    pwm_init(SERVO_MOTOR_PWM, SERVO_MOTOR_FREQ, (unsigned long)SERVO_MOTOR_DUTY(90.0));
    servo_motor_duty = 90.0;
    servo_motor_dir = 1;
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void servo_set_angle (float angle)
{
    if(angle < SERVO_MOTOR_L_MAX) angle = SERVO_MOTOR_L_MAX;
    if(angle > SERVO_MOTOR_R_MAX) angle = SERVO_MOTOR_R_MAX;
    servo_motor_duty = angle;
    pwm_set_duty(SERVO_MOTOR_PWM, (unsigned long)SERVO_MOTOR_DUTY(angle));
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
float servo_get_angle (void)
{
    return servo_motor_duty;
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void servo_sweep (void)
{
    pwm_set_duty(SERVO_MOTOR_PWM, (unsigned long)SERVO_MOTOR_DUTY(servo_motor_duty));

    if(servo_motor_dir)
    {
        servo_motor_duty += 5;
        if(servo_motor_duty >= SERVO_MOTOR_R_MAX)
        {
            servo_motor_dir = 0x00;
        }
    }
    else
    {
        servo_motor_duty -= 5;
        if(servo_motor_duty <= SERVO_MOTOR_L_MAX)
        {
            servo_motor_dir = 0x01;
        }
    }
}
