#include "zf_common_headfile.h"
#include "IfxScu_reg.h"
#include "screen.h"
#include "offline_voice.h"
#include "buzzer_action.h"
#include "rear_motor/rear_motor.h"
#include "guandao.h"
#include "subject_2_gyro_route.h"
#include "display.h"
#include "serial_menu.h"
#include <stdio.h>
#pragma section all "cpu0_dsram"

extern int num;
static uint8 voice_inited = 0;

#define PORTION2_DRIVE_SPEED_MIN_MPS  (1.0f)
#define PORTION2_DRIVE_SPEED_STEP_MPS (1.0f)
#define PORTION2_DRIVE_SPEED_MAX_MPS  (5.0f)
#define PORTION2_DRIVE_GPS_SERIAL_PERIOD_MS (500U)

static float portion2_drive_target_mps = 0.0f;
static uint8 portion2_drive_full_power = 0;
static uint8 portion2_drive_active = 0;
static uint32 portion2_drive_k4_start_ms = 0;
static uint8 portion2_drive_k4_wait_release = 0;

#define REAR_MOTOR_TELEMETRY_PERIOD_MS (200U)
#define RECORD_IDLE_ENCODER_DIAG_PERIOD_MS (10U)

static void Guandao_Rear_Motor_Update(void)
{
    float target_mps = 0.0f;

    if(conrtol_mode == GUANDAO)
    {
        target_mps = (out_v_l + out_v_r) * 0.5f * GUANDAO_SPEED_TO_MPS;
    }
    else if(conrtol_mode == DAOCHE)
    {
        if(portion2_run_drive_reverse && portion2_selected_route == PORTION2_ROUTE_STRAIGHT)
        {
            target_mps = -(out_v_l + out_v_r) * 0.5f * GUANDAO_SPEED_TO_MPS;
        }
        else
        {
            target_mps = daoche_speed * GUANDAO_SPEED_TO_MPS;
        }
    }
    else
    {
        rear_motor_stop();
        return;
    }

    if(target_mps == 0.0f)
    {
        rear_motor_stop();
    }
    else
    {
        rear_motor_set_target_mps(target_mps);
        rear_motor_pid_update_100ms();
    }
}

// Output one line through the downloader/debug UART.
// Hardware path: TC264 UART0, TX=P14_0, RX=P14_1, 115200 baud, initialized by debug_init().
static void Serial_Debug_Write(const char *line)
{
    uart_write_string(DEBUG_UART_INDEX, line);
}

// Print boot reset reason using SCU reset status register.
static void Boot_Reset_Print(void)
{
    Ifx_SCU_RSTSTAT rst = SCU_RSTSTAT;
    uint32 v = rst.U;
    static char line[160];
    int len = sprintf(line,
                      "[BOOT-RESET] RSTSTAT=0x%08lX PORST=%u ESR0=%u ESR1=%u SW=%u SMU=%u SWD=%u STBYR=%u\r\n",
                      (unsigned long)v,
                      (unsigned)rst.B.PORST,
                      (unsigned)rst.B.ESR0,
                      (unsigned)rst.B.ESR1,
                      (unsigned)rst.B.SW,
                      (unsigned)rst.B.SMU,
                      (unsigned)rst.B.SWD,
                      (unsigned)rst.B.STBYR);
    if(len > 0)
    {
        Serial_Debug_Write(line);
    }
}

static void Rear_Motor_Serial_Telemetry_Update(void)
{
    static uint32 last_report_ms = 0;
    uint32 now_ms = system_getval_ms();

    if((uint32)(now_ms - last_report_ms) < REAR_MOTOR_TELEMETRY_PERIOD_MS)
    {
        return;
    }

    last_report_ms = now_ms;
    {
        char line[160];
        int len = sprintf(line,
                          "[REAR-SPEED] target=%.2f actual=%.2f speed=%.3fmps pwm=%d enc10=%d enc100=%ld totalPulse=%ld dist=%.3f\r\n",
                          rear_motor_get_target_mps(),
                          rear_motor_get_speed_mps(),
                          rear_motor_get_speed_mps(),
                          (int)rear_motor_get_pwm(),
                          (int)rear_motor_get_encoder_10ms(),
                          (long)rear_motor_get_encoder_100ms(),
                          (long)rear_motor_get_total_encoder_pulses(),
                          rear_motor_get_total_distance_m());
        if(len > 0)
        {
            uart_write_string(DEBUG_UART_INDEX, line);
        }
    }
}

static void Record_Idle_Encoder_Diag_Update(void)
{
    static uint32 last_diag_ms = 0;
    uint32 now_ms = system_getval_ms();

    if(main_mode != Guandao_Portion2_Recode)
    {
        return;
    }
    if(remote_control_is_active())
    {
        return;
    }
    if(conrtol_mode != IDLE)
    {
        return;
    }
    if((uint32)(now_ms - last_diag_ms) < RECORD_IDLE_ENCODER_DIAG_PERIOD_MS)
    {
        return;
    }

    last_diag_ms = now_ms;
    rear_motor_encoder_update_10ms();
    {
        char line[128];
        int len = sprintf(line,
                          "[ENC-DIAG] enc10=%d enc100=%ld totalPulse=%ld dist=%.3f actual=%.2f pwm=%d\r\n",
                          (int)rear_motor_get_encoder_10ms(),
                          (long)rear_motor_get_encoder_100ms(),
                          (long)rear_motor_get_total_encoder_pulses(),
                          rear_motor_get_total_distance_m(),
                          rear_motor_get_speed_mps(),
                          (int)rear_motor_get_pwm());
        if(len > 0)
        {
            uart_write_string(DEBUG_UART_INDEX, line);
        }
    }
}

double gk_d = 0;
double gk_a = 0;
uint8 port2_flag = 0;

static const dot_matrix_pattern_t portion2_aux_pattern_table[6] =
{
    DOT_MATRIX_PATTERN_DOUBLE_FLASH,
    DOT_MATRIX_PATTERN_TURN_LEFT,
    DOT_MATRIX_PATTERN_TURN_RIGHT,
    DOT_MATRIX_PATTERN_LOW_BEAM,
    DOT_MATRIX_PATTERN_HIGH_BEAM,
    DOT_MATRIX_PATTERN_FOG_LIGHT
};

#define PORTION2_ALL_LIGHT_PIN P22_0

static uint8 portion2_aux_mode = 0;
static uint32 portion2_aux_start_ms = 0;

static void Portion2_Aux_Stop(void)
{
    dot_matrix_screen_set_brightness(0);
    dot_matrix_screen_clear_pattern();
    servo_set_angle(90.0f);
    gpio_low(PORTION2_ALL_LIGHT_PIN);
    portion2_aux_mode = 0;
}

static void Portion2_Aux_Start(uint8 mode)
{
    Portion2_Aux_Stop();
    portion2_aux_mode = mode;
    portion2_aux_start_ms = system_getval_ms();

    if(mode >= 1 && mode <= 6)
    {
        dot_matrix_screen_set_brightness(5000);
        dot_matrix_screen_show_led_pattern(portion2_aux_pattern_table[mode - 1]);
    }
    else if(mode == 7)
    {
        gpio_high(PORTION2_ALL_LIGHT_PIN);
    }
}

static void Portion2_Aux_Task(void)
{
    if(portion2_aux_mode == 0)
    {
        return;
    }

    if((uint32)(system_getval_ms() - portion2_aux_start_ms) >= 10000)
    {
        Portion2_Aux_Stop();
        return;
    }

    if(portion2_aux_mode == 8)
    {
        servo_sweep();
    }
}

static void Portion2_Dot_Matrix_Scan_Update(void)
{
    static uint32 last_scan_ms = 0;
    uint32 now_ms = system_getval_ms();

    if((uint32)(now_ms - last_scan_ms) >= 1U)
    {
        last_scan_ms = now_ms;
        dot_matrix_screen_scan();
    }
}

static void Portion2_Drive_Mode_Stop(void)
{
    portion2_drive_target_mps = 0.0f;
    portion2_drive_full_power = 0;
    portion2_drive_active = 0;
    portion2_drive_k4_start_ms = 0;
    portion2_drive_k4_wait_release = 0;
    out_v_l = 0.0f;
    out_v_r = 0.0f;
    out_servo = 0.0f;
    rear_motor_stop();
}

static void Portion2_Drive_Mode_Enter(void)
{
    Portion2_Drive_Mode_Stop();
    portion2_drive_active = 1;
    if(gpio_get_level(KEY4) == 0)
    {
        portion2_drive_k4_wait_release = 1;
    }
    main_mode = Guandao_Drive;
    conrtol_mode = GUANDAO;
    ips200_clear();
}

static void Portion2_Drive_Encoder_Update_10ms(void)
{
    static uint32 last_encoder_ms = 0;
    uint32 now_ms = system_getval_ms();

    if((uint32)(now_ms - last_encoder_ms) < 10U)
    {
        return;
    }

    last_encoder_ms = now_ms;
    rear_motor_encoder_update_10ms();
}

static void Portion2_Drive_GPS_Serial_Update(void)
{
    static uint32 last_gps_ms = 0;
    uint32 now_ms = system_getval_ms();

    if(main_mode != Guandao_Drive)
    {
        return;
    }
    if((uint32)(now_ms - last_gps_ms) < PORTION2_DRIVE_GPS_SERIAL_PERIOD_MS)
    {
        return;
    }

    last_gps_ms = now_ms;
    {
        char line[128];
        int len = sprintf(line,
                          "[DRIVE-GPS] state=%u sats=%u hdop100=%u lat7=%ld lon7=%ld\r\n",
                          (unsigned)gnss.state,
                          (unsigned)gnss.satellite_used,
                          (unsigned)(gnss.hdop * 100.0f + 0.5f),
                          (long)(gnss.latitude * 10000000.0),
                          (long)(gnss.longitude * 10000000.0));
        if(len > 0)
        {
            Serial_Debug_Write(line);
        }
    }
}

static void Portion2_Drive_Key_Log(const char *key_name)
{
    char line[96];
    int len = sprintf(line,
                      "[DRIVE-KEY] %s full=%u target=%.2f\r\n",
                      key_name,
                      (unsigned int)portion2_drive_full_power,
                      portion2_drive_target_mps);
    if(len > 0)
    {
        Serial_Debug_Write(line);
    }
}

static uint8 portion2_drive_k4_long_event(void)
{
    uint32 now_ms = system_getval_ms();
    uint8 key_down = (gpio_get_level(KEY4) == 0);
    uint8 long_event = 0;

    if(key4_flag)
    {
        key4_flag = 0;
    }

    if(portion2_drive_k4_wait_release)
    {
        if(!key_down)
        {
            portion2_drive_k4_wait_release = 0;
            portion2_drive_k4_start_ms = 0;
        }
        return 0;
    }

    if(key_down)
    {
        if(portion2_drive_k4_start_ms == 0) portion2_drive_k4_start_ms = now_ms;
        if((uint32)(now_ms - portion2_drive_k4_start_ms) > 1500U)
        {
            long_event = 1;
            portion2_drive_k4_wait_release = 1;
        }
    }
    else
    {
        portion2_drive_k4_start_ms = 0;
    }

    return long_event;
}

static void Portion2_Drive_Mode_Key_Handle(void)
{
    if(key1_flag)
    {
        key1_flag = 0;
        if(portion2_drive_full_power)
        {
            portion2_drive_full_power = 0;
            portion2_drive_target_mps = PORTION2_DRIVE_SPEED_MAX_MPS;
        }
        else
        {
            portion2_drive_target_mps -= PORTION2_DRIVE_SPEED_STEP_MPS;
            if(portion2_drive_target_mps < 0.0f)
            {
                portion2_drive_target_mps = 0.0f;
            }
        }
        Buzzer_check(20);
        Portion2_Drive_Key_Log("K1");
    }

    if(key2_flag)
    {
        key2_flag = 0;
        if(portion2_drive_target_mps >= PORTION2_DRIVE_SPEED_MAX_MPS)
        {
            portion2_drive_full_power = 1;
        }
        else
        {
            portion2_drive_target_mps += PORTION2_DRIVE_SPEED_STEP_MPS;
            if(portion2_drive_target_mps > PORTION2_DRIVE_SPEED_MAX_MPS)
            {
                portion2_drive_target_mps = PORTION2_DRIVE_SPEED_MAX_MPS;
            }
        }
        Buzzer_check(20);
        Portion2_Drive_Key_Log("K2");
    }

    if(key3_flag)
    {
        key3_flag = 0;
    }

    if(portion2_drive_k4_long_event())
    {
        Portion2_Drive_Mode_Stop();
        portion2_mode_key_transition_lock();
        portion2_record_enter_mode();
        Key_Init();
        main_mode = Guandao_Portion2_Recode;
        conrtol_mode = IDLE;
        ips200_clear();
        Buzzer_check(80);
    }
}

static void Portion2_Drive_Mode_Task(void)
{
    if(!portion2_drive_active)
    {
        Portion2_Drive_Mode_Enter();
    }

    Portion2_Drive_Mode_Key_Handle();
    Portion2_Drive_Encoder_Update_10ms();
    Portion2_Drive_GPS_Serial_Update();
    out_servo = 0.0f;
    if(portion2_drive_full_power)
    {
        rear_motor_set_full_power();
    }
    else if(portion2_drive_target_mps <= 0.0f)
    {
        rear_motor_stop();
    }
    else
    {
        rear_motor_set_target_mps(portion2_drive_target_mps);
        rear_motor_pid_update_100ms();
    }
}

static void Portion2_Drive_Mode_UI_Update(void)
{
    static uint32 last_ui_ms = 0;
    uint32 now_ui_ms = system_getval_ms();
    if((uint32)(now_ui_ms - last_ui_ms) < 100U) return;
    last_ui_ms = now_ui_ms;

    ips200_show_string(X(1), Y(0), "MODE: DRIVE");
    ips200_show_string(X(1), Y(1), "SPEED:");
    if(portion2_drive_full_power)
    {
        ips200_show_string(X(9), Y(1), "FULL 100% ");
    }
    else
    {
        ips200_show_float(X(9), Y(1), portion2_drive_target_mps, 3, 1);
        ips200_show_string(X(14), Y(1), "m/s");
    }
    ips200_show_string(X(1), Y(2), "K1 -1m/s");
    ips200_show_string(X(1), Y(3), "K2 +1m/s");
    ips200_show_string(X(1), Y(4), "K4 HOLD REC");
}

static void Portion2_Run_Mode_Key_Handle(void)
{
    if(key1_flag) { key1_flag = 0; }
    if(key2_flag) { key2_flag = 0; }
    if(key3_flag)
    {
        key3_flag = 0;
        voice_drive_action_stop();
        portion2_run_stop();
        out_v_l = 0.0f;
        out_v_r = 0.0f;
        out_servo = 0.0f;
        rear_motor_stop();
        Buzzer_check(50);
    }
    if(portion2_mode_k4_short_event())
    {
        Portion2_Aux_Stop();
        voice_drive_action_stop();
        portion2_run_stop();
        out_v_l = 0.0f;
        out_v_r = 0.0f;
        out_servo = 0.0f;
        rear_motor_stop();
        portion2_mode_key_transition_lock();
        portion2_record_enter_mode();
        Key_Init();
        main_mode = Guandao_Portion2_Recode;
        conrtol_mode = IDLE;
        voice_inited = 0;
        ips200_clear();
        Buzzer_check(50);
    }
}

static void Portion2_Run_Mode_UI_Update(void)
{
    static uint32 last_ui_ms = 0;
    uint32 now_ui_ms = system_getval_ms();
    if((uint32)(now_ui_ms - last_ui_ms) < 100U) return;
    last_ui_ms = now_ui_ms;
    ips200_show_string(X(1),  Y(0), "MODE: RUN");
    ips200_show_string(X(1),  Y(1), "MOTOR: AUTO");
    ips200_show_string(X(1),  Y(2), (voice_drive_action_get_mode() != VOICE_DRIVE_ACTION_NONE) ? "VOICE: RUN" : "VOICE: WAIT");
    {
        char buf[16];
        uint8 cmd = offline_voice_get_last_cmd();
        if(cmd == 0) sprintf(buf, "--"); else sprintf(buf, "0x%02X", (unsigned)cmd);
        ips200_show_string(X(1),  Y(3), "CMD: ");
        ips200_show_string(X(6),  Y(3), buf);
    }
    {
        uint8 saved_route_count = 0;
        for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
        {
            if(portion2_route_length[i] > 0)
            {
                saved_route_count++;
            }
        }
        ips200_show_string(X(1),  Y(4), "ROUTES:");
        ips200_show_int(X(9),  Y(4), saved_route_count, 2);
        ips200_show_string(X(11), Y(4), "/");
        ips200_show_int(X(12), Y(4), PORTION2_ROUTE_COUNT, 2);
        ips200_show_string(X(14), Y(4), "      ");
        if(portion2_run_reject_reason == 2 || portion2_run_reject_reason == 3)
        {
            ips200_show_string(X(1),  Y(5), "ROUTE EMPTY");
        }
    }
}

static void Portion2_Fixed_Action_Start(voice_drive_action_mode_t mode)
{
    portion2_run_stop();
    voice_drive_action_start(mode);
}

static void Portion2_Ascii_Enter_Run_Mode(void)
{
    main_mode = Guandao_Voice;
    route_setting_choice = 3;
    conrtol_mode = GUANDAO;
    voice_inited = 0;
}

static void Portion2_Ascii_Command_Execute(uint8 data)
{
    static uint8 reverse_route_pending = 0;
    static uint8 dump_route_pending = 0;

    if(data == 'D')
    {
        reverse_route_pending = 0;
        dump_route_pending = 1;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        portion2_serial_dump_routes();
    }
    else if(data == 'T')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        portion2_serial_toggle_trace();
    }
    else if(data == 'S')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        voice_drive_action_stop();
        subject_2_gyro_route_stop("COMMAND");
        portion2_run_stop();
        out_v_l = 0.0f;
        out_v_r = 0.0f;
        out_servo = 0.0f;
        rear_motor_stop();
        uart_write_string(DEBUG_UART_INDEX, "[P2-RUN] stop\r\n");
    }
    else if(data == '-')
    {
        reverse_route_pending = 1;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
    }
    else if(data >= '1' && data <= '9')
    {
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        if(dump_route_pending)
        {
            uart_write_string(DEBUG_UART_INDEX, "\r\n");
            portion2_serial_dump_route(data - '1');
            dump_route_pending = 0;
        }
        else
        {
            voice_drive_action_stop();
            Portion2_Ascii_Enter_Run_Mode();
            if(reverse_route_pending && data >= '1' && data <= '5')
            {
                portion2_run_select_reverse_route(data - '1');
            }
            else if(reverse_route_pending && (data == '8' || data == '9'))
            {
                portion2_run_select_back_route(data - '1');
            }
            else
            {
                portion2_run_select_route(data - '1');
            }
        }
        reverse_route_pending = 0;
    }
    else if(data >= 'U' && data <= 'V')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        voice_drive_action_stop();
        Portion2_Ascii_Enter_Run_Mode();
        portion2_run_select_route(data - 'U' + PORTION2_ROUTE_RETURN_5);
    }
    else if(data == 'W')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        voice_drive_action_stop();
        Portion2_Ascii_Enter_Run_Mode();
        portion2_run_select_route(PORTION2_ROUTE_SNAKE);
    }
    else if(data >= 'u' && data <= 'w')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        voice_drive_action_stop();
        Portion2_Ascii_Enter_Run_Mode();
        portion2_run_select_route(data - 'u' + PORTION2_ROUTE_RETURN_5);
    }
    else if(data >= 'A' && data <= 'H')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);

        if(data >= 'A' && data <= 'G')
        {
            Portion2_Aux_Start(data - 'A' + 1);
        }
        else if(data == 'H')
        {
            Portion2_Aux_Start(8);
        }
    }
    else if(data >= 'a' && data <= 'h')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);

        if(data >= 'a' && data <= 'g')
        {
            Portion2_Aux_Start(data - 'a' + 1);
        }
        else if(data == 'h')
        {
            Portion2_Aux_Start(8);
        }
    }
    else if(data >= 'I' && data <= 'P')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        Portion2_Fixed_Action_Start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (data - 'I')));
    }
    else if(data >= 'i' && data <= 'p')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        Portion2_Fixed_Action_Start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (data - 'i')));
    }
    else if(data == 'Q' || data == 'q')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M);
    }
    else if(data == 'R' || data == 'r')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M);
    }
    else if(data == 'X' || data == 'x')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        voice_drive_action_stop();
        portion2_run_stop();
        subject_2_gyro_route_start(SUBJECT_2_GYRO_ROUTE_13, 1U);
    }
    else if(data == 'y')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        voice_drive_action_stop();
        portion2_run_stop();
        subject_2_gyro_route_start(SUBJECT_2_GYRO_ROUTE_14, 0U);
    }
    else if(data == 'Y')
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        uart_write_string(DEBUG_UART_INDEX, "\r\n");
        voice_drive_action_stop();
        portion2_run_stop();
        subject_2_gyro_route_start(SUBJECT_2_GYRO_ROUTE_14, 1U);
    }
    else
    {
        reverse_route_pending = 0;
        dump_route_pending = 0;
    }
}

static void Portion2_Voice_Command_Handle(uint8 cmd_id, void *user_data)
{
    (void)user_data;

    if(main_mode != Guandao_Voice)
    {
        return;
    }

    switch(cmd_id)
    {
        case OFFLINE_VOICE_CMD_DOUBLE_FLASH:
            Portion2_Aux_Start(1);
            break;

        case OFFLINE_VOICE_CMD_TURN_LEFT:
            Portion2_Aux_Start(2);
            break;

        case OFFLINE_VOICE_CMD_TURN_RIGHT:
            Portion2_Aux_Start(3);
            break;

        case OFFLINE_VOICE_CMD_LOW_BEAM:
            Portion2_Aux_Start(4);
            break;

        case OFFLINE_VOICE_CMD_HIGH_BEAM:
            Portion2_Aux_Start(5);
            break;

        case OFFLINE_VOICE_CMD_FOG_LIGHT:
            Portion2_Aux_Start(6);
            break;

        case OFFLINE_VOICE_CMD_INTERIOR_LIGHT:
            Portion2_Aux_Start(7);
            break;

        case OFFLINE_VOICE_CMD_WIPER:
            Portion2_Aux_Start(8);
            break;

        case OFFLINE_VOICE_CMD_HORN_1S:
        case OFFLINE_VOICE_CMD_HORN_2S:
        case OFFLINE_VOICE_CMD_HORN_3S:
        case OFFLINE_VOICE_CMD_HORN_2X:
        case OFFLINE_VOICE_CMD_HORN_3X:
        case OFFLINE_VOICE_CMD_HORN_4X:
        case OFFLINE_VOICE_CMD_HORN_LONG_SHORT:
        case OFFLINE_VOICE_CMD_HORN_RAPID:
        case OFFLINE_VOICE_CMD_HORN_ALARM:
            buzzer_action_from_voice_cmd(cmd_id);
            break;

        case OFFLINE_VOICE_CMD_GATE1_LEFT:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_1);
            break;

        case OFFLINE_VOICE_CMD_GATE1:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_2);
            break;

        case OFFLINE_VOICE_CMD_GATE2:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_3);
            break;

        case OFFLINE_VOICE_CMD_GATE3:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_4);
            break;

        case OFFLINE_VOICE_CMD_GATE3_RIGHT:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_5);
            break;

        case OFFLINE_VOICE_CMD_ROUTE_STRAIGHT:
            Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M);
            break;

        case OFFLINE_VOICE_CMD_ROUTE_SNAKE:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_SNAKE);
            break;

        case OFFLINE_VOICE_CMD_GATE1_RIGHT_BACK:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_RETURN_1);
            break;

        case OFFLINE_VOICE_CMD_GATE1_BACK:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_RETURN_2);
            break;

        case OFFLINE_VOICE_CMD_GATE2_BACK:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_RETURN_3);
            break;

        case OFFLINE_VOICE_CMD_GATE3_BACK:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_RETURN_4);
            break;

        case OFFLINE_VOICE_CMD_GATE3_LEFT_BACK:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_RETURN_5);
            break;

        case OFFLINE_VOICE_CMD_BACK_STRAIGHT:
            Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M);
            break;

        case OFFLINE_VOICE_CMD_BACK_SNAKE:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_STRAIGHT);
            break;

        case OFFLINE_VOICE_CMD_FORWARD_10M:
            Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M);
            break;

        case OFFLINE_VOICE_CMD_BACKWARD_10M:
            Portion2_Fixed_Action_Start(VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M);
            break;

        case OFFLINE_VOICE_CMD_SNAKE_FORWARD:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_SNAKE);
            break;

        case OFFLINE_VOICE_CMD_SNAKE_BACKWARD:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_STRAIGHT);
            break;

        case OFFLINE_VOICE_CMD_CCW_CIRCLE:
        case OFFLINE_VOICE_CMD_CW_CIRCLE:
        case OFFLINE_VOICE_CMD_TURN_LEFT_DRIVE:
        case OFFLINE_VOICE_CMD_TURN_RIGHT_DRIVE:
            Portion2_Fixed_Action_Start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (cmd_id - OFFLINE_VOICE_CMD_FORWARD_10M)));
            break;

        default:
            break;
    }
}

static void Portion2_Serial_Command_Update(void)
{
    uint8 data;
    uint8 buffer[64];
    uint32 len = debug_read_ring_buffer(buffer, sizeof(buffer));

    for(uint32 i = 0; i < len; i++)
    {
        data = buffer[i];
        if(serial_menu_handle_byte(data))
        {
            continue;
        }
        Portion2_Ascii_Command_Execute(data);
    }
}

int core0_main(void)
{
    clock_init();                   
    debug_init();                   
    Boot_Reset_Print();

    Init_All();                         
    rear_motor_init();                  
    /* UART1 is reserved for the TLD7002 dot-matrix driver in RUN serial
       light-pattern tests, so offline voice is left disabled below. */
    dot_matrix_screen_init();
    servo_init();
    gpio_init(PORTION2_ALL_LIGHT_PIN, GPO, GPIO_LOW, GPO_PUSH_PULL);
    remote_control_init();
    
   pit_ms_init(CCU61_CH0, 1);
   pit_ms_init(CCU61_CH1, 1);

    cpu_wait_event_ready();                                                          


    Flash_Main_Read();                                                                  
    portion2_record_mark_loaded_routes_saved();

    main_mode = Guandao_Portion2_Recode;
    route_setting_choice = 1;
    conrtol_mode = IDLE;

    Flash_Write_pid();                                                               

    Main_Key_Flag = 1;                                                            
//    while(Steer_Mid_Cheak());
    while (TRUE)
    {
        remote_control_task();
        Portion2_Serial_Command_Update();
        switch(main_mode)                                                    
        {
            case Guandao_Voice:                                             
                if(!voice_inited)
                {
                    screen_init();
                    voice_inited = 1;
                    ips200_clear();
                }
                Portion2_Run_Mode_Key_Handle();
                Portion2_Dot_Matrix_Scan_Update();
                Portion2_Aux_Task();
                if(subject_2_gyro_route_is_active())
                {
                    subject_2_gyro_route_task();
                }
                else if(voice_drive_action_get_mode() != VOICE_DRIVE_ACTION_NONE)
                {
                    voice_drive_action_task();
                }
                else
                {
                    portion2_run_task();
                }
                Portion2_Run_Mode_UI_Update();
                break;

            case Guandao_Portion2_Recode:
                voice_inited = 0;
                portion2_record_task();
                if(remote_control_is_active())
                {
                    continue;
                }
                break;

            case Guandao_Drive:
                voice_inited = 0;
                Portion2_Drive_Mode_Task();
                Portion2_Drive_Mode_UI_Update();
                break;

            default : break;

        }
        if(main_mode != Guandao_Drive)
        {
            Guandao_Rear_Motor_Update();
        }
//                    ips200_show_int(X(10),  Y(13),conrtol_mode ,5);
//                    ips200_show_float(X(10),  Y(12),angle_speed ,5 ,5);
//                    VeerMoter_Set(10000);

//        if(key1_flag ==1)
//        {
//            key1_flag=0;
//        }
//        if(key3_flag == 1)
//        {
//            key3_flag = 0;
//            GPS_Work_SHOW();
//            ips200_clear();
//        }

        
    }
}

#pragma section all restore
