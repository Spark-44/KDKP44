#include "remote_control.h"
#include "portion2_uart1_mux.h"
#include "rear_motor/rear_motor.h"
#include "zf_device_dot_matrix_screen.h"
#include "zf_device_tld7002.h"

#define REMOTE_CONTROL_FRAME_TIMEOUT_MS        (300U)
#define REMOTE_CONTROL_CHANNEL_DEAD_ZONE       (50)
#define REMOTE_CONTROL_CHANNEL_FULL_SCALE      (800.0f)
#define REMOTE_CONTROL_SPEED_CENTER            (992U)
#define REMOTE_CONTROL_SPEED_FULL_SCALE        (776.0f)
#define REMOTE_CONTROL_MAX_TARGET_ANGLE_DEG    (30.0f)
#define REMOTE_CONTROL_MAX_TARGET_SPEED_MPS    (2.0f)
#define REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS (0.02f)
#define REMOTE_CONTROL_SPEED_RAMP_STEP_MPS     (0.03f)
#define REMOTE_CONTROL_ANGLE_DIRECTION         (-1.0f)
#define REMOTE_CONTROL_SPEED_DIRECTION         (-1.0f)
#define REMOTE_CONTROL_MODE_LOW_THRESHOLD      (600U)
#define REMOTE_CONTROL_MODE_HIGH_THRESHOLD     (1400U)
#define REMOTE_CONTROL_BUTTON_THRESHOLD        (1400U)

#pragma section all "cpu0_dsram"
static uint16 remote_channel_start[UART_RECEVIER_CHANNEL_NUM] = {0};
static int32  remote_channel_offset[UART_RECEVIER_CHANNEL_NUM] = {0};
static uint8  remote_channel_start_init = 0;
static uint8  remote_control_active = 0;
static float  remote_target_angle_deg = 0.0f;
static float  remote_target_speed_mps = 0.0f;
static float  remote_command_speed_mps = 0.0f;
static uint32 remote_last_frame_ms = 0;
static uint32 remote_last_10ms = 0;
static uint8  remote_ch3_button_last = 0;
static uint8  remote_ch5_button_last = 0;
static uint8  remote_ch6_button_last = 0;
static volatile portion2_uart1_owner_t portion2_uart1_owner = PORTION2_UART1_OWNER_NONE;
#pragma section all restore

static void remote_control_apply_failsafe(const char *reason);

static int32 remote_control_apply_dead_zone(int32 value)
{
    if(value > -REMOTE_CONTROL_CHANNEL_DEAD_ZONE && value < REMOTE_CONTROL_CHANNEL_DEAD_ZONE)
    {
        return 0;
    }
    return value;
}

static float remote_control_limit_float(float value, float min, float max)
{
    if(value > max)
    {
        value = max;
    }
    if(value < min)
    {
        value = min;
    }
    return value;
}

static float remote_control_map_speed_from_ch2(uint16 channel_value)
{
    int32 offset = (int32)channel_value - (int32)REMOTE_CONTROL_SPEED_CENTER;
    float speed_mps;

    offset = remote_control_apply_dead_zone(offset);
    remote_channel_offset[1] = offset;

    speed_mps = (float)offset / REMOTE_CONTROL_SPEED_FULL_SCALE * REMOTE_CONTROL_MAX_TARGET_SPEED_MPS * REMOTE_CONTROL_SPEED_DIRECTION;
    return remote_control_limit_float(speed_mps, -REMOTE_CONTROL_MAX_TARGET_SPEED_MPS, REMOTE_CONTROL_MAX_TARGET_SPEED_MPS);
}

static float remote_control_limit_speed_command(float target_mps)
{
    float actual_mps = rear_motor_get_speed_mps();

    if(target_mps > 0.0f &&
       actual_mps >= (REMOTE_CONTROL_MAX_TARGET_SPEED_MPS + REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS))
    {
        return REMOTE_CONTROL_MAX_TARGET_SPEED_MPS;
    }
    if(target_mps < 0.0f &&
       actual_mps <= -(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS + REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS))
    {
        return -REMOTE_CONTROL_MAX_TARGET_SPEED_MPS;
    }

    return target_mps;
}

static float remote_control_ramp_speed_command(float target_mps)
{
    if(target_mps > remote_command_speed_mps + REMOTE_CONTROL_SPEED_RAMP_STEP_MPS)
    {
        remote_command_speed_mps += REMOTE_CONTROL_SPEED_RAMP_STEP_MPS;
    }
    else if(target_mps < remote_command_speed_mps - REMOTE_CONTROL_SPEED_RAMP_STEP_MPS)
    {
        remote_command_speed_mps -= REMOTE_CONTROL_SPEED_RAMP_STEP_MPS;
    }
    else
    {
        remote_command_speed_mps = target_mps;
    }

    return remote_command_speed_mps;
}

static void remote_control_capture_center(void)
{
    uint8 i;

    if(remote_channel_start_init)
    {
        return;
    }

    for(i = 0; i < UART_RECEVIER_CHANNEL_NUM; i++)
    {
        remote_channel_start[i] = uart_receiver.channel[i];
    }
    remote_channel_start_init = 1;
}

static void remote_control_update_target(void)
{
    uint8 i;

    for(i = 0; i < UART_RECEVIER_CHANNEL_NUM; i++)
    {
        remote_channel_offset[i] = remote_control_apply_dead_zone((int32)uart_receiver.channel[i] - (int32)remote_channel_start[i]);
    }

    remote_target_angle_deg = (float)remote_channel_offset[0] / REMOTE_CONTROL_CHANNEL_FULL_SCALE * REMOTE_CONTROL_MAX_TARGET_ANGLE_DEG * REMOTE_CONTROL_ANGLE_DIRECTION;
    remote_target_speed_mps = remote_control_map_speed_from_ch2(uart_receiver.channel[1]);
    remote_target_angle_deg = remote_control_limit_float(remote_target_angle_deg, -REMOTE_CONTROL_MAX_TARGET_ANGLE_DEG, REMOTE_CONTROL_MAX_TARGET_ANGLE_DEG);
}

static void remote_control_update_mode_from_ch4(void)
{
    Mode_Choice next_mode;

    if(uart_receiver.channel[3] < REMOTE_CONTROL_MODE_LOW_THRESHOLD)
    {
        next_mode = Guandao_Drive;
    }
    else if(uart_receiver.channel[3] < REMOTE_CONTROL_MODE_HIGH_THRESHOLD)
    {
        next_mode = Guandao_Portion2_Recode;
    }
    else
    {
        next_mode = Guandao_Voice;
    }

    if(main_mode != next_mode)
    {
        if(remote_control_active && next_mode != Guandao_Portion2_Recode)
        {
            remote_control_apply_failsafe("MODE");
        }
        main_mode = next_mode;

        if(next_mode == Guandao_Voice)
        {
            route_setting_choice = 3;
            conrtol_mode = GUANDAO;
            ips200_clear();
        }
        else if(next_mode == Guandao_Portion2_Recode)
        {
            route_setting_choice = 1;
            conrtol_mode = IDLE;
            ips200_clear();
        }
        else
        {
            route_setting_choice = 1;
            conrtol_mode = GUANDAO;
            ips200_clear();
        }
    }
}

static void remote_control_update_record_buttons(void)
{
    uint8 ch3_active = (uart_receiver.channel[2] >= REMOTE_CONTROL_BUTTON_THRESHOLD);
    uint8 ch5_active = (uart_receiver.channel[4] >= REMOTE_CONTROL_BUTTON_THRESHOLD);
    uint8 ch6_active = (uart_receiver.channel[5] >= REMOTE_CONTROL_BUTTON_THRESHOLD);

    if(main_mode == Guandao_Portion2_Recode)
    {
        if(ch3_active && !remote_ch3_button_last)
        {
            portion2_record_remote_start_stop_request();
        }
        if(ch5_active && !remote_ch5_button_last)
        {
            portion2_record_remote_clear_request();
        }
        if(ch6_active && !remote_ch6_button_last)
        {
            portion2_record_remote_save_request();
        }
    }

    remote_ch3_button_last = ch3_active;
    remote_ch5_button_last = ch5_active;
    remote_ch6_button_last = ch6_active;
}

static void remote_control_apply_failsafe(const char *reason)
{
    (void)reason;

    remote_control_active = 0;
    remote_channel_start_init = 0;
    remote_target_angle_deg = 0.0f;
    remote_target_speed_mps = 0.0f;
    remote_command_speed_mps = 0.0f;
    out_servo = 0.0f;
    conrtol_mode = IDLE;
    rear_motor_clear_speed_limit();
    rear_motor_stop();
    angle_control_set_target(0.0f);
}

static void remote_control_periodic_update(void)
{
    uint32 now_ms = system_getval_ms();

    if((uint32)(now_ms - remote_last_10ms) >= 10U)
    {
        remote_last_10ms = now_ms;
        conrtol_mode = IDLE;
        out_servo = remote_target_angle_deg;
        Steer_Moter_Contral(remote_target_angle_deg);
        rear_motor_encoder_update_10ms();
        rear_motor_set_speed_limit_mps(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS);
        remote_command_speed_mps = remote_control_ramp_speed_command(remote_control_limit_speed_command(remote_target_speed_mps));
        rear_motor_set_target_mps(remote_command_speed_mps);
        rear_motor_pid_update_100ms();
    }
}

void remote_control_init(void)
{
    uint8 i;

    for(i = 0; i < UART_RECEVIER_CHANNEL_NUM; i++)
    {
        remote_channel_start[i] = 0;
        remote_channel_offset[i] = 0;
    }
    remote_channel_start_init = 0;
    remote_control_active = 0;
    remote_target_angle_deg = 0.0f;
    remote_target_speed_mps = 0.0f;
    remote_command_speed_mps = 0.0f;
    remote_last_frame_ms = system_getval_ms();
    remote_last_10ms = remote_last_frame_ms;
    remote_ch3_button_last = 0;
    remote_ch5_button_last = 0;
    remote_ch6_button_last = 0;

    uart_receiver_init();
}

uint8 remote_control_task(void)
{
    uint32 now_ms = system_getval_ms();

    if(uart_receiver.finsh_flag)
    {
        uart_receiver.finsh_flag = 0;
        remote_last_frame_ms = now_ms;

        if(uart_receiver.state)
        {
            remote_control_update_mode_from_ch4();
            remote_control_update_record_buttons();
            if(main_mode == Guandao_Portion2_Recode)
            {
                remote_control_capture_center();
                remote_control_update_target();
                remote_control_active = 1;
            }
            else if(remote_control_active)
            {
                remote_control_apply_failsafe("MODE");
            }
        }
        else
        {
            remote_control_apply_failsafe("DISCONNECT");
            return 0;
        }
    }

    if(remote_control_active)
    {
        if(main_mode != Guandao_Portion2_Recode)
        {
            remote_control_apply_failsafe("MODE");
            return 0;
        }

        if((uint32)(now_ms - remote_last_frame_ms) > REMOTE_CONTROL_FRAME_TIMEOUT_MS)
        {
            remote_control_apply_failsafe("TIMEOUT");
            return 0;
        }

        remote_control_periodic_update();
        return 1;
    }

    return 0;
}

void remote_control_stop(void)
{
    if(remote_control_active)
    {
        remote_control_apply_failsafe("DISABLED");
    }
}

uint8 remote_control_is_active(void)
{
    return remote_control_active;
}

portion2_uart1_owner_t portion2_uart1_get_owner(void)
{
    return portion2_uart1_owner;
}

void portion2_uart1_select_record_receiver(void)
{
    if(portion2_uart1_owner == PORTION2_UART1_OWNER_RECORD_RECEIVER)
    {
        return;
    }

    portion2_uart1_owner = PORTION2_UART1_OWNER_RECORD_RECEIVER;
    remote_control_init();
}

void portion2_uart1_select_run_dot_matrix(void)
{
    if(portion2_uart1_owner == PORTION2_UART1_OWNER_RUN_DOT_MATRIX)
    {
        return;
    }

    remote_control_stop();
    portion2_uart1_owner = PORTION2_UART1_OWNER_RUN_DOT_MATRIX;
    dot_matrix_screen_init();
}

void portion2_uart1_update_for_mode(Mode_Choice mode)
{
    if(mode == Guandao_Portion2_Recode)
    {
        portion2_uart1_select_record_receiver();
    }
    else
    {
        portion2_uart1_select_run_dot_matrix();
    }
}

void portion2_uart1_rx_isr_handler(void)
{
    if(portion2_uart1_owner == PORTION2_UART1_OWNER_RECORD_RECEIVER)
    {
        uart_receiver_handler();
    }
    else if(portion2_uart1_owner == PORTION2_UART1_OWNER_RUN_DOT_MATRIX)
    {
        tld7002_callback();
    }
}

void portion2_uart1_error_isr_handler(void)
{
    if(portion2_uart1_owner == PORTION2_UART1_OWNER_RECORD_RECEIVER)
    {
        uart_receiver_note_uart_error();
    }
}
