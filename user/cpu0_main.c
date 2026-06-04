
#include "zf_common_headfile.h"
#include "IfxScu_reg.h"
#include "screen.h"
#include "offline_voice.h"
#include "buzzer_action.h"
#include "rear_motor/rear_motor.h"
#include <stdio.h>
#pragma section all "cpu0_dsram"

extern int num;

static void Guandao_Rear_Motor_Update(void)
{
    float target_mps = 0.0f;

    if(conrtol_mode == GUANDAO)
    {
        target_mps = (out_v_l + out_v_r) * 0.5f * GUANDAO_SPEED_TO_MPS;
    }
    else if(conrtol_mode == DAOCHE)
    {
        target_mps = daoche_speed * GUANDAO_SPEED_TO_MPS;
    }
    else if(conrtol_mode == YAOKONG)
    {
        // YAOKONG disabled: ignore remote speed input
        target_mps = 0.0f;
    }
    else if(main_mode != Rack_Test_Mode)
    {
        rear_motor_stop();
        return;
    }
    else
    {
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

static uint8 portion2_aux_mode = 0;
static uint32 portion2_aux_start_ms = 0;

static void Portion2_Aux_Stop(void)
{
    dot_matrix_screen_set_brightness(0);
    dot_matrix_screen_clear_pattern();
    servo_set_angle(90.0f);
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

static void Portion2_Fixed_Action_Start(voice_drive_action_mode_t mode)
{
    portion2_run_stop();
    voice_drive_action_start(mode);
}

static void Portion2_Ascii_Command_Execute(uint8 data)
{
    static uint8 reverse_route_pending = 0;

    if(data == '-')
    {
        reverse_route_pending = 1;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
    }
    else if(data >= '1' && data <= '9')
    {
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        voice_drive_action_stop();
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
        reverse_route_pending = 0;
    }
    else if(data >= 'A' && data <= 'H')
    {
        reverse_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);

        if(data >= 'A' && data <= 'F')
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
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);

        if(data >= 'a' && data <= 'f')
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
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        Portion2_Fixed_Action_Start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (data - 'I')));
    }
    else if(data >= 'i' && data <= 'p')
    {
        reverse_route_pending = 0;
        portion2_run_last_rx = data;
        portion2_run_rx_count++;
        uart_write_byte(DEBUG_UART_INDEX, data);
        Portion2_Fixed_Action_Start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (data - 'i')));
    }
}

static void Portion2_Voice_Command_Handle(uint8 cmd_id, void *user_data)
{
    (void)user_data;

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

        case OFFLINE_VOICE_CMD_ROUTE_ARRIVE_CHANGE:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_ARRIVE_DIR_CHANGE);
            break;

        case OFFLINE_VOICE_CMD_ROUTE_START_CHANGE:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_START_DIR_CHANGE);
            break;

        case OFFLINE_VOICE_CMD_ROUTE_STRAIGHT:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_STRAIGHT);
            break;

        case OFFLINE_VOICE_CMD_ROUTE_SNAKE:
            voice_drive_action_stop();
            portion2_run_select_route(PORTION2_ROUTE_SNAKE);
            break;

        case OFFLINE_VOICE_CMD_GATE1_RIGHT_BACK:
            voice_drive_action_stop();
            portion2_run_select_reverse_route(PORTION2_ROUTE_1);
            break;

        case OFFLINE_VOICE_CMD_GATE1_BACK:
            voice_drive_action_stop();
            portion2_run_select_reverse_route(PORTION2_ROUTE_2);
            break;

        case OFFLINE_VOICE_CMD_GATE2_BACK:
            voice_drive_action_stop();
            portion2_run_select_reverse_route(PORTION2_ROUTE_3);
            break;

        case OFFLINE_VOICE_CMD_GATE3_BACK:
            voice_drive_action_stop();
            portion2_run_select_reverse_route(PORTION2_ROUTE_4);
            break;

        case OFFLINE_VOICE_CMD_GATE3_LEFT_BACK:
            voice_drive_action_stop();
            portion2_run_select_reverse_route(PORTION2_ROUTE_5);
            break;

        case OFFLINE_VOICE_CMD_BACK_STRAIGHT:
            voice_drive_action_stop();
            portion2_run_select_back_route(PORTION2_ROUTE_STRAIGHT);
            break;

        case OFFLINE_VOICE_CMD_BACK_SNAKE:
            voice_drive_action_stop();
            portion2_run_select_back_route(PORTION2_ROUTE_SNAKE);
            break;

        case OFFLINE_VOICE_CMD_FORWARD_10M:
        case OFFLINE_VOICE_CMD_BACKWARD_10M:
        case OFFLINE_VOICE_CMD_SNAKE_FORWARD:
        case OFFLINE_VOICE_CMD_SNAKE_BACKWARD:
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
    static uint8 voice_frame_remaining = 0;

    for(uint32 i = 0; i < len; i++)
    {
        data = buffer[i];
        if(voice_frame_remaining > 0)
        {
            offline_voice_feed_byte(data);
            voice_frame_remaining--;
        }
        else if(data == 0x6B)
        {
            offline_voice_feed_byte(data);
            voice_frame_remaining = 7;
        }
        else
        {
            Portion2_Ascii_Command_Execute(data);
        }
    }
}

int core0_main(void)
{
    clock_init();                   
    debug_init();                   
    Boot_Reset_Print();

    Init_All();                         
    rear_motor_init();                  
    dot_matrix_screen_init();
    servo_init();
    // uart_receiver_init();               // SBUS disabled: free UART2 for voice module
    
   pit_ms_init(CCU61_CH0, 1);           

    cpu_wait_event_ready();                                                          

    Flash_Main_Read();                                                                  

    Menu_Contral();                                                                      

    if(main_mode == Guandao_Voice)
    {
        screen_init();
        offline_voice_init(Portion2_Voice_Command_Handle, 0);
    }

    Flash_Write_pid();                                                               

    Main_Key_Flag = 1;                                                            
//    while(Steer_Mid_Cheak());
    while (TRUE)
    {
        

        

        switch(main_mode)                                                    
        {
            case Mode_IDLE:                                                   

                break;

            case Guandao_Voice:                                             
                Portion2_Serial_Command_Update();
                Portion2_Dot_Matrix_Scan_Update();
                Portion2_Aux_Task();
                if(voice_drive_action_get_mode() != VOICE_DRIVE_ACTION_NONE)
                {
                    voice_drive_action_task();
                }
                else
                {
                    portion2_run_task();
                }
                break;

            case Guandao_Portion2_Recode:
                portion2_record_task();
                break;

            case Guandao_portion_3:                                     
                guandao_trace(&INS);                                      
                break;

            case Rack_Test_Mode:
                Rack_Test_Run();
                break;

            case YaoKong_Mode:
                break;

            default : break;

        }
        Guandao_Rear_Motor_Update();

            
            if(main_mode == YaoKong_Mode)
            {
                ips200_show_string(X(1),  Y(8), "RC");
                ips200_show_string(X(1),  Y(9), "State");    ips200_show_int(X(8),  Y(9), uart_receiver.state, 2);
                ips200_show_string(X(1),  Y(10), "C1");      ips200_show_int(X(5),  Y(10), uart_receiver.channel[0], 4);
                ips200_show_string(X(11), Y(10), "C2");      ips200_show_int(X(15), Y(10), uart_receiver.channel[1], 4);
                ips200_show_string(X(1),  Y(11), "C3");      ips200_show_int(X(5),  Y(11), uart_receiver.channel[2], 4);
                ips200_show_string(X(11), Y(11), "C4");      ips200_show_int(X(15), Y(11), uart_receiver.channel[3], 4);
                ips200_show_string(X(1),  Y(12), "C5");      ips200_show_int(X(5),  Y(12), uart_receiver.channel[4], 4);
                ips200_show_string(X(11), Y(12), "C6");      ips200_show_int(X(15), Y(12), uart_receiver.channel[5], 4);
                ips200_show_string(X(1),  Y(13), "Steer");   ips200_show_float(X(8),  Y(13), hot_rc_steer, 3, 1);
                ips200_show_string(X(1),  Y(14), "Speed");   ips200_show_float(X(8),  Y(14), hot_rc_speed, 2, 1);
            }
            else if(main_mode == Guandao_portion_3)
            {
                static uint32 last_ui_ms = 0;
                uint32 now_ui_ms = system_getval_ms();
                if(now_ui_ms - last_ui_ms >= 100)
                {
                    last_ui_ms = now_ui_ms;
                    ips200_show_string(X(1),  Y(8), "Idx");      ips200_show_int(X(6),  Y(8), portion_3.current_point_index, 4);
                    ips200_show_string(X(12), Y(8), "Len");      ips200_show_int(X(17), Y(8), portion_3.length_index, 4);
                    ips200_show_string(X(1),  Y(9), "D");        ips200_show_float(X(6),  Y(9), guandao_debug_distance, 3, 2);
                    ips200_show_string(X(12), Y(9), "A");        ips200_show_float(X(16), Y(9), guandao_debug_angle_diff, 3, 1);
                    ips200_show_string(X(1),  Y(10), "Reason");  ips200_show_int(X(10), Y(10), guandao_debug_stop_reason, 2);
                    ips200_show_string(X(1),  Y(11), "VlVr");    ips200_show_float(X(7),  Y(11), out_v_l, 3, 1); ips200_show_float(X(15), Y(11), out_v_r, 3, 1);
                    ips200_show_string(X(1),  Y(12), "TgtAct");  ips200_show_float(X(9),  Y(12), rear_motor_get_target_mps(), 2, 1); ips200_show_float(X(16), Y(12), rear_motor_get_speed_mps(), 2, 1);
                    ips200_show_string(X(1),  Y(13), "PWM");     ips200_show_int(X(7),  Y(13), rear_motor_get_pwm(), 5);
                    ips200_show_string(X(1),  Y(14), "Yaw");     ips200_show_float(X(7),  Y(14), Yaw_1, 4, 1);
                    ips200_show_string(X(1),  Y(15), "XY");      ips200_show_float(X(5),  Y(15), portion_3.current_state.x, 3, 1); ips200_show_float(X(13), Y(15), portion_3.current_state.y, 3, 1);
                }
            }
//                    ips200_show_int(X(10),  Y(13),conrtol_mode ,5);
//                    ips200_show_float(X(10),  Y(12),angle_speed ,5 ,5);
//                    VeerMoter_Set(10000);

//        hotRc_Show();
        if(0 && main_mode != Rack_Test_Mode && x6f_out[4] ==200)                                          
        {
            conrtol_mode =IDLE;
            Moter_Set(0 , 0 );
            VeerMoter_Set(0);

        }
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
