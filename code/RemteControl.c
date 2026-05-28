/*
 * RemteControl.c
 *
 * SBUS remote receiver mapping.
 * The receiver signal is connected to UART2 RX P10_6. CH1/CH2 drive steering
 * and throttle. x6f_out[] is kept only as a compatibility state array for
 * older route-save/reverse logic; no X6F GPIO scan is used anymore.
 */

#include "zf_common_headfile.h"

int16 x6f_out[6];

float hot_rc_speed = 0;
float hot_rc_steer = 0;
float hot_rc_delta = 0;

#define SBUS_MIN    172
#define SBUS_MID    1024
#define SBUS_MAX    1811

void sbus_rc_control(void)
{
    if(uart_receiver.state == 0)
    {
        hot_rc_speed = 0;
        hot_rc_steer = 0;
        hot_rc_delta = 0;
        x6f_out[2] = 100;
        x6f_out[3] = 100;
        return;
    }

    uint16 ch_steer = uart_receiver.channel[0];
    uint16 ch_throttle = uart_receiver.channel[1];
    uint16 ch_save = uart_receiver.channel[2];
    uint16 ch_stop = uart_receiver.channel[3];

    hot_rc_steer = (float)(SBUS_MID - ch_steer) * 30.0f / (SBUS_MAX - SBUS_MID);
    if(hot_rc_steer > -2.0f && hot_rc_steer < 2.0f)
    {
        hot_rc_steer = 0;
    }

    hot_rc_speed = (float)(SBUS_MID - ch_throttle) * 15.0f / (SBUS_MAX - SBUS_MID);
    if(hot_rc_speed > -1.0f && hot_rc_speed < 1.0f)
    {
        hot_rc_speed = 0;
    }

    hot_rc_delta = (hot_rc_speed * tanf(hot_rc_steer / 3.0f / 180.0f * M_PI)) / WHEEL_BASE;

    x6f_out[0] = (int16)ch_steer;
    x6f_out[1] = (int16)ch_throttle;
    x6f_out[2] = (ch_save > 1500) ? 200 : 100;
    x6f_out[3] = (ch_stop > 1500) ? 200 : 100;
}

void hotRc_Show(void)
{
    ips200_show_int(10 * 0, 16 * 18, uart_receiver.channel[0], 4);
    ips200_show_int(10 * 4, 16 * 18, uart_receiver.channel[1], 4);
    ips200_show_int(10 * 8, 16 * 18, uart_receiver.state, 1);
    ips200_show_float(10 * 10, 16 * 18, hot_rc_steer, 3, 1);
    ips200_show_float(10 * 16, 16 * 18, hot_rc_speed, 2, 1);
}
