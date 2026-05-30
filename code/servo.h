

#ifndef CODE_SERVO_H_
#define CODE_SERVO_H_

#include "zf_common_headfile.h"

#define AUX_SERVO_MOTOR_PWM         (ATOM2_CH4_P33_8)
#define AUX_SERVO_MOTOR_FREQ        (50)

#define AUX_SERVO_MOTOR_L_MAX       (0)
#define AUX_SERVO_MOTOR_R_MAX       (180)

#if (AUX_SERVO_MOTOR_FREQ < 50 || AUX_SERVO_MOTOR_FREQ > 300)
    #error "AUX_SERVO_MOTOR_FREQ ERROR!"
#endif

void    servo_init      (void);                                                 
void    servo_set_angle (float angle);
float   servo_get_angle (void);                                                 
void    servo_sweep     (void);                                                 
#endif /* CODE_SERVO_H_ */
