
#include "zf_common_headfile.h"
#include "rear_motor/rear_motor.h"
#include <stdio.h>
#pragma section all "cpu0_dsram"

extern int num;

#define SERIAL_DEBUG_PERIOD_MS  (200)

static guandao_state* Get_Record_Display_State(void)
{
    switch(route_setting_choice)
    {
        case 0: return &INS;
        case 1: return &passage;
        case 2: return &portion_3;
        case 3: return &portion_2;
        default: return &INS;
    }
}

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
        target_mps = hot_rc_speed * GUANDAO_SPEED_TO_MPS;
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

// Convert float data to a scaled integer before printing.
// This avoids relying on floating-point printf support in the embedded C library.
static int32 Serial_Debug_Scale(float value, float scale)
{
    return (int32)(value * scale);
}

// Output one line through the downloader/debug UART.
// Hardware path: TC264 UART0, TX=P14_0, RX=P14_1, 115200 baud, initialized by debug_init().
static void Serial_Debug_Write(const char *line)
{
    uart_write_string(DEBUG_UART_INDEX, line);
}

// Periodic serial diagnostics for subject-one record and autonomous trace modes.
// REC lines are used while pushing the car to record points.
// AUTO lines are used while the car is tracking the saved INS route.
static void Serial_Debug_Update(void)
{
    static uint32 last_ms = 0;
    uint32 now_ms = system_getval_ms();
    static char line[320];
    int len;

    if(now_ms - last_ms < SERIAL_DEBUG_PERIOD_MS)
    {
        return;
    }
    last_ms = now_ms;

    if(main_mode == Guandao_Recode_Mode)
    {
        guandao_state *record_state = Get_Record_Display_State();

        len = sprintf(line,
                      "REC,t=%lu,route=%d,len=%d,full=%d,thr100=%ld,x100=%ld,y100=%ld,th10=%ld,encL=%d,encR=%d,key1=%d,gps=%d,sat=%d,gflag=%d\r\n",
                      (unsigned long)now_ms,
                      route_setting_choice,
                      record_state->length_index,
                      (record_state->length_index >= MAX_LENGTH_INDEX),
                      (long)Serial_Debug_Scale(recode_threshold, 100.0f),
                      (long)Serial_Debug_Scale(record_state->current_state.x, 100.0f),
                      (long)Serial_Debug_Scale(record_state->current_state.y, 100.0f),
                      (long)Serial_Debug_Scale(record_state->current_state.theta, 10.0f),
                      guandao_ecd.delta_l,
                      guandao_ecd.delta_r,
                      gpio_get_level(KEY1),
                      gnss.state,
                      gnss.satellite_used,
                      gnss_flag);
        if(len > 0)
        {
            Serial_Debug_Write(line);
        }
    }
    else if(main_mode == Guandao_portion_1)
    {
        len = sprintf(line,
                      "AUTO,t=%lu,idx=%d,rlen=%d,plen=%d,ready=%d,D100=%ld,A10=%ld,fd100=%ld,reason=%d,pth100=%ld,pv=%d,x100=%ld,y100=%ld,yaw10=%ld,vl10=%ld,vr10=%ld,servo10=%ld,tgt100=%ld,act100=%ld,pwm=%d,enc10=%d,enc100=%ld\r\n",
                      (unsigned long)now_ms,
                      INS.current_point_index,
                      INS.length_index,
                      INS.planned_length,
                      INS.plan_ready,
                      (long)Serial_Debug_Scale(guandao_debug_distance, 100.0f),
                      (long)Serial_Debug_Scale(guandao_debug_angle_diff, 10.0f),
                      (long)Serial_Debug_Scale(guandao_debug_dist_final, 100.0f),
                      guandao_debug_stop_reason,
                      (long)Serial_Debug_Scale(persuit_threshold, 100.0f),
                      preview_spets,
                      (long)Serial_Debug_Scale(INS.current_state.x, 100.0f),
                      (long)Serial_Debug_Scale(INS.current_state.y, 100.0f),
                      (long)Serial_Debug_Scale(Yaw_1, 10.0f),
                      (long)Serial_Debug_Scale(out_v_l, 10.0f),
                      (long)Serial_Debug_Scale(out_v_r, 10.0f),
                      (long)Serial_Debug_Scale(out_servo, 10.0f),
                      (long)Serial_Debug_Scale(rear_motor_get_target_mps(), 100.0f),
                      (long)Serial_Debug_Scale(rear_motor_get_speed_mps(), 100.0f),
                      rear_motor_get_pwm(),
                      rear_motor_get_encoder_10ms(),
                      (long)rear_motor_get_encoder_100ms());
        if(len > 0)
        {
            Serial_Debug_Write(line);
        }
    }
    else if(conrtol_mode == YAOKONG)
    {
        len = sprintf(line,
                      "RC,t=%lu,flag=%d,state=%d,ch1=%d,ch2=%d,ch3=%d,ch4=%d,ch5=%d,ch6=%d,steer10=%ld,speed100=%ld\r\n",
                      (unsigned long)now_ms,
                      uart_receiver.finsh_flag,
                      uart_receiver.state,
                      uart_receiver.channel[0],
                      uart_receiver.channel[1],
                      uart_receiver.channel[2],
                      uart_receiver.channel[3],
                      uart_receiver.channel[4],
                      uart_receiver.channel[5],
                      (long)Serial_Debug_Scale(hot_rc_steer, 10.0f),
                      (long)Serial_Debug_Scale(hot_rc_speed, 100.0f));
        if(len > 0)
        {
            Serial_Debug_Write(line);
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

static void Portion2_Serial_Command_Update(void)
{
    uint8 data = 0;
    uint8 buffer[8];
    uint32 len = debug_read_ring_buffer(buffer, 8);

    for(uint32 i = 0; i < len; i++)
    {
        data = buffer[i];
        if(data >= '1' && data <= '9')
        {
            portion2_run_last_rx = data;
            portion2_run_rx_count++;
            uart_write_byte(DEBUG_UART_INDEX, data);
            voice_drive_action_stop();
            portion2_run_select_route(data - '1');
        }
        else if(data == '0')
        {
            portion2_run_last_rx = data;
            portion2_run_rx_count++;
            uart_write_byte(DEBUG_UART_INDEX, data);
            voice_drive_action_stop();
            portion2_run_select_route(9);
        }
        else if(data >= 'A' && data <= 'H')
        {
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
            ips200_show_string(X(1), Y(15), "Aux");
            ips200_show_int(X(6), Y(15), portion2_aux_mode, 2);
            ips200_show_string(X(10), Y(15), "Act");
            ips200_show_int(X(15), Y(15), dot_matrix_screen_is_pattern_active(), 1);
            ips200_show_string(X(1), Y(14), "Scn");
            ips200_show_int(X(6), Y(14), dot_matrix_screen_scan_count, 6);
        }
        else if(data >= 'a' && data <= 'h')
        {
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
            ips200_show_string(X(1), Y(15), "Aux");
            ips200_show_int(X(6), Y(15), portion2_aux_mode, 2);
            ips200_show_string(X(10), Y(15), "Act");
            ips200_show_int(X(15), Y(15), dot_matrix_screen_is_pattern_active(), 1);
        }
        else if(data >= 'I' && data <= 'P')
        {
            portion2_run_last_rx = data;
            portion2_run_rx_count++;
            uart_write_byte(DEBUG_UART_INDEX, data);
            portion2_run_stop();
            voice_drive_action_start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (data - 'I')));
        }
        else if(data >= 'i' && data <= 'p')
        {
            portion2_run_last_rx = data;
            portion2_run_rx_count++;
            uart_write_byte(DEBUG_UART_INDEX, data);
            portion2_run_stop();
            voice_drive_action_start((voice_drive_action_mode_t)(VOICE_DRIVE_ACTION_FORWARD_10M + (data - 'i')));
        }
    }
}

int core0_main(void)
{
    clock_init();                   
    debug_init();                   

    Init_All();                         
    rear_motor_init();                  
    dot_matrix_screen_init();
    servo_init();
    uart_receiver_init();               // SBUS receiver on UART2 RX=P10_6 for YAOKONG mode
    
   pit_ms_init(CCU61_CH0, 1);           

    cpu_wait_event_ready();                                                          

    Flash_Main_Read();                                                                  

    Menu_Contral();                                                                      

    if(main_mode == Guandao_Voice)
    {
        gpio_init(P11_2, GPO, 1, GPO_PUSH_PULL);
        system_delay_ms(100);
        dot_matrix_screen_init();
    }

    Flash_Write_pid();                                                               

    if(GPS_WORK_FLAG){GPS_WorkMap_Copy(&INS);}       
    Main_Key_Flag = 1;                                                            
//    while(Steer_Mid_Cheak());
    while (TRUE)
    {
        

        sbus_rc_control();

        switch(main_mode)                                                    
        {
            case Mode_IDLE:                                                   

                break;

            case Guandao_Recode_Mode:                             

            case Guandao_portion_1:                                     
                portion_1();                                                        
                break;

            case Guandao_Voice:                                             
                Portion2_Aux_Task();
                dot_matrix_screen_scan();
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
        Serial_Debug_Update();
//        ips200_show_float(X(1),  Y(8) ,INS.recode_gpsmap[INS.gps_recode_length -1].lat, 3,6);
//        ips200_show_float(X(11),  Y(8) ,INS.recode_gpsmap[INS.gps_recode_length -1].lon, 3,6);
//        ips200_show_float(X(1),  Y(9) ,INS.recode_gpsmap[INS.gps_recode_length -1].cheak_flag, 3,6);
//        ips200_show_float(X(11),  Y(9) ,INS.recode_gpsmap[INS.gps_recode_length -1].theta, 3,6);
//        ips200_show_float(X(11),  Y(10) ,INS.gps_recode_length, 3,6);
//        ips200_show_float(X(1),  Y(10) ,gnss.satellite_used, 3,6);

            
            if(main_mode == Guandao_Recode_Mode)
            {
                guandao_state *record_state = Get_Record_Display_State();
                ips200_show_string(X(1),  Y(8), "REC");      ips200_show_int(X(6),  Y(8), route_setting_choice, 2);
                ips200_show_string(X(10), Y(8), "Len");      ips200_show_int(X(15), Y(8), record_state->length_index, 4);
                ips200_show_string(X(1),  Y(9), "X");        ips200_show_float(X(4),  Y(9), record_state->current_state.x, 4, 2);
                ips200_show_string(X(12), Y(9), "Y");        ips200_show_float(X(15), Y(9), record_state->current_state.y, 4, 2);
                ips200_show_string(X(1),  Y(10), "Theta");   ips200_show_float(X(8),  Y(10), record_state->current_state.theta, 4, 1);
                ips200_show_string(X(1),  Y(11), "Enc");     ips200_show_int(X(6),  Y(11), guandao_ecd.delta_l, 5); ips200_show_int(X(14), Y(11), guandao_ecd.delta_r, 5);
                ips200_show_string(X(1),  Y(12), "KEY1");    ips200_show_int(X(7),  Y(12), gpio_get_level(KEY1), 1);
                ips200_show_string(X(10), Y(12), "GPS");     ips200_show_int(X(15), Y(12), gnss.state, 1);
                ips200_show_string(X(1),  Y(13), "Sat");     ips200_show_int(X(6),  Y(13), gnss.satellite_used, 3);
                ips200_show_string(X(10), Y(13), "GFlag");   ips200_show_int(X(17), Y(13), gnss_flag, 1);
                if(record_state->length_index >= MAX_LENGTH_INDEX)
                {
                    ips200_show_string(X(1),  Y(14), "Route FULL");
                }
            }
            else if(main_mode == YaoKong_Mode)
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
            else
            {
                ips200_show_string(X(1),  Y(8), "Idx");      ips200_show_int(X(6),  Y(8), INS.current_point_index, 4);
                ips200_show_string(X(12), Y(8), "Len");      ips200_show_int(X(17), Y(8), INS.length_index, 4);
                ips200_show_string(X(1),  Y(9), "D");        ips200_show_float(X(6),  Y(9), guandao_debug_distance, 3, 2);
                ips200_show_string(X(12), Y(9), "A");        ips200_show_float(X(16), Y(9), guandao_debug_angle_diff, 3, 1);
                ips200_show_string(X(1),  Y(10), "Reason");  ips200_show_int(X(10), Y(10), guandao_debug_stop_reason, 2);
                ips200_show_string(X(1),  Y(11), "VlVr");    ips200_show_float(X(7),  Y(11), out_v_l, 3, 1); ips200_show_float(X(15), Y(11), out_v_r, 3, 1);
                ips200_show_string(X(1),  Y(12), "TgtAct");  ips200_show_float(X(9),  Y(12), rear_motor_get_target_mps(), 2, 1); ips200_show_float(X(16), Y(12), rear_motor_get_speed_mps(), 2, 1);
                ips200_show_string(X(1),  Y(13), "PWM");     ips200_show_int(X(7),  Y(13), rear_motor_get_pwm(), 5);
                ips200_show_string(X(1),  Y(14), "Yaw");     ips200_show_float(X(7),  Y(14), Yaw_1, 4, 1);
                ips200_show_string(X(1),  Y(15), "XY");      ips200_show_float(X(5),  Y(15), INS.current_state.x, 3, 1); ips200_show_float(X(13), Y(15), INS.current_state.y, 3, 1);
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

