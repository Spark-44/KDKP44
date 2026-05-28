/*
 * RemteControl.h
 *
 * SBUS remote receiver interface. The receiver uses UART2 RX P10_6.
 * x6f_out[] is retained as a compatibility state array; it no longer maps to
 * physical X6F GPIO input pins.
 */

#ifndef CODE_REMTECONTROL_H_
#define CODE_REMTECONTROL_H_

extern int16 x6f_out[6];

extern float hot_rc_speed;
extern float hot_rc_steer;
extern float hot_rc_delta;

void sbus_rc_control(void);
void hotRc_Show(void);

#endif /* CODE_REMTECONTROL_H_ */
