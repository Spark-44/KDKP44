

#ifndef CODE_SERVO_H_
#define CODE_SERVO_H_

#include "zf_common_headfile.h"

#define SERVO_MOTOR_PWM             (ATOM2_CH4_P33_8)                           
#define SERVO_MOTOR_FREQ            (50 )                                       

#define SERVO_MOTOR_L_MAX           (0  )                                       
#define SERVO_MOTOR_R_MAX           (180)                                       

#if (SERVO_MOTOR_FREQ<50 || SERVO_MOTOR_FREQ>300)
    #error "SERVO_MOTOR_FREQ ERROE!"
#endif

void    servo_init      (void);                                                 
float   servo_get_angle (void);                                                 
void    servo_sweep     (void);                                                 
#endif /* CODE_SERVO_H_ */
