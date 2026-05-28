

#include "isr_config.h"
#include "isr.h"
#include "rear_motor/rear_motor.h"

IFX_INTERRUPT(cc60_pit_ch1_isr, 0, CCU6_0_CH1_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     
    pit_clear_flag(CCU60_CH1);

}

IFX_INTERRUPT(cc61_pit_ch0_isr, 0, CCU6_1_CH0_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     
    pit_clear_flag(CCU61_CH0);
    TIM_FLAG2 ++;
    TIM_FLAG3 ++;

    if(TIM_FLAG3 > 10)
    {
        TIM_FLAG3 = 0 ;
//        Steer_Moter_Contral(30);
        switch(conrtol_mode)
        {
            case IDLE:
                break;

            case YAOKONG:
                Steer_Moter_Contral(hot_rc_steer);
                break;

            case GUANDAO:
                Steer_Moter_Contral(out_servo);
                break;
            case DAOCHE :
                Steer_Moter_Contral(-out_servo);
                break;
            case RACK_TEST:
                if(rack_test_stage == 1)Steer_Moter_Contral((float)rack_test_steer_target);
                else if(rack_test_stage == 3)
                {
                    Rack_Straight_Update();
                    Steer_Moter_Contral(rack_straight_steer_target);
                }
                else VeerMoter_Set(0);
                break;

            default : break;
        }

    }

    
    if(conrtol_mode == RACK_TEST || conrtol_mode == GUANDAO || conrtol_mode == DAOCHE || conrtol_mode == YAOKONG)
    {
        static uint8 rear_tick = 0;
        rear_tick++;
        if(rear_tick >= 10)
        {
            rear_tick = 0;
            rear_motor_encoder_update_10ms();
        }
    }

    if(GPS_WORK_FLAG)
    {
        if(TIM_FLAG2 > 100 )
        {
            TIM_FLAG2 = 0;
            if(gnss_flag)
            {
                gnss_flag = 0 ;
                gnss_data_parse();           
//                klm_handle();
                
                if(Main_Key_Flag ){update_gpsinformation();}
            }
        }
    }

}

IFX_INTERRUPT(cc61_pit_ch1_isr, 0, CCU6_1_CH1_ISR_PRIORITY)
{
    interrupt_global_enable(0);                     
    pit_clear_flag(CCU61_CH1);
    TIM_FLAG1++;
//    TIM_FLAG2++;
    if(TIM_FLAG1 > 7 )
    {
        TIM_FLAG1 = 0;
//        Speed_Control(10 , 10);
        if(Main_Key_Flag == 1)
        {
                Key_Scan();

                imu963ra_get_gyro();
                IMU_GetValues();

                switch(conrtol_mode)
                {
                    case IDLE:
                        break;
                    case YAOKONG:
                        break;
                    case GUANDAO:
                        break;
                    case DAOCHE:
                        break;
                    case RACK_TEST:
                        break;
                    default : break;
                }

        }
        if(Main_Key_Flag == 0)
        {

                Speed_Control((float)0.0 ,(float) 0.0);
                rear_motor_stop();

        }
            }

    }

IFX_INTERRUPT(exti_ch0_ch4_isr, 0, EXTI_CH0_CH4_INT_PRIO)
{
    interrupt_global_enable(0);                     
    if(exti_flag_get(ERU_CH4_REQ8_P33_7))           
    {
        exti_flag_clear(ERU_CH4_REQ8_P33_7);
        camera_vsync_handler_1();                   
    }

    if(exti_flag_get(ERU_CH0_REQ0_P15_4))           
    {
        exti_flag_clear(ERU_CH0_REQ0_P15_4);
        
        
        

    }

}

IFX_INTERRUPT(exti_ch1_ch5_isr, 0, EXTI_CH1_CH5_INT_PRIO)
{
    interrupt_global_enable(0);                     

    if(exti_flag_get(ERU_CH1_REQ10_P14_3))          
    {
        exti_flag_clear(ERU_CH1_REQ10_P14_3);
        
        
        

    }

    if(exti_flag_get(ERU_CH5_REQ1_P15_8))           
    {
        exti_flag_clear(ERU_CH5_REQ1_P15_8);
        
        
        

    }
}

IFX_INTERRUPT(exti_ch2_ch6_isr, 0, EXTI_CH2_CH6_INT_PRIO)
{
    interrupt_global_enable(0);                     
    if(exti_flag_get(ERU_CH2_REQ7_P00_4))           
    {
        exti_flag_clear(ERU_CH2_REQ7_P00_4);
        
        
        

    }
    if(exti_flag_get(ERU_CH6_REQ9_P20_0))           
    {
        exti_flag_clear(ERU_CH6_REQ9_P20_0);
        
        
        

    }
}

IFX_INTERRUPT(exti_ch3_ch7_isr, 0, EXTI_CH3_CH7_INT_PRIO)
{
    interrupt_global_enable(0);                     

    if(exti_flag_get(ERU_CH3_REQ3_P10_3))           
    {
        exti_flag_clear(ERU_CH3_REQ3_P10_3);
        camera_vsync_handler_2();                   
    }

    if(exti_flag_get(ERU_CH7_REQ16_P15_1))          
    {
        exti_flag_clear(ERU_CH7_REQ16_P15_1);
        
        
        

    }
}

IFX_INTERRUPT(dma_ch6_isr, 0, DMA_INT_PRIO_1)
{
    interrupt_global_enable(0);                     
    camera_dma_handler_1();                         
}

IFX_INTERRUPT(dma_ch7_isr, 0, DMA_INT_PRIO_2)
{
    interrupt_global_enable(0);                     
    camera_dma_handler_2();                         
}

IFX_INTERRUPT(uart0_tx_isr, 0, UART0_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     

}
IFX_INTERRUPT(uart0_rx_isr, 0, UART0_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     

#if DEBUG_UART_USE_INTERRUPT                        
        debug_interrupr_handler();                  
#endif                                              
}

IFX_INTERRUPT(uart1_tx_isr, 0, UART1_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     

}
IFX_INTERRUPT(uart1_rx_isr, 0, UART1_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     
    tld7002_callback();
    camera_uart_handler_1();                        
}

IFX_INTERRUPT(uart2_tx_isr, 0, UART2_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     

}

IFX_INTERRUPT(uart2_rx_isr, 0, UART2_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     
    wireless_module_uart_handler();                 

}

IFX_INTERRUPT(uart3_tx_isr, 0, UART3_TX_INT_PRIO)
{
    interrupt_global_enable(0);                     

}

IFX_INTERRUPT(uart3_rx_isr, 0, UART3_RX_INT_PRIO)
{
    interrupt_global_enable(0);                     

    gnss_uart_callback();                           

}

IFX_INTERRUPT(uart0_er_isr, 0, UART0_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     
    IfxAsclin_Asc_isrError(&uart0_handle);
}
IFX_INTERRUPT(uart1_er_isr, 0, UART1_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     
    IfxAsclin_Asc_isrError(&uart1_handle);
}
IFX_INTERRUPT(uart2_er_isr, 0, UART2_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     
    IfxAsclin_Asc_isrError(&uart2_handle);
}
IFX_INTERRUPT(uart3_er_isr, 0, UART3_ER_INT_PRIO)
{
    interrupt_global_enable(0);                     
    IfxAsclin_Asc_isrError(&uart3_handle);
}

