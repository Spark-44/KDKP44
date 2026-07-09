/*********************************************************************************************************************
* TC264 Opensourec Library 即（TC264 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
*
* 本文件是 TC264 开源库的一部分
*
* TC264 开源库 是免费软件
* 您可以根据自由软件基金会发布的 GPL（GNU General Public License，即 GNU通用公共许可证）的条款
* 即 GPL 的第3版（即 GPL3.0）或（您选择的）任何后来的版本，重新发布和/或修改它
*
* 文件名称          zf_device_uart_receiver
* 公司名称          成都逐飞科技有限公司
* 适用平台          TC264D
********************************************************************************************************************/

#include "zf_device_type.h"
#include "zf_driver_uart.h"
#include "zf_driver_timer.h"
#include "zf_device_uart_receiver.h"
#include "string.h"

uart_receiver_struct uart_receiver;

static uint8 uart_receiver_data[REV_DATA_LEN] = {0};

typedef enum
{
    UART_RECEIVER_SYNC_WAIT_HEADER = 0,
    UART_RECEIVER_SYNC_IN_FRAME
} uart_receiver_sync_state_enum;

static volatile uart_receiver_sync_state_enum uart_receiver_sync_state = UART_RECEIVER_SYNC_WAIT_HEADER;
static volatile uint8 uart_receiver_data_length = 0;

static uint32 uart_receiver_interval_time(void)
{
    static uint32 time_last = 0;
    uint32 time;
    uint32 interval_time;
    static uint32 stm_clk = 0;

    if(stm_clk == 0)
    {
        stm_clk = IfxStm_getFrequency(IfxStm_getAddress((IfxStm_Index)(IfxCpu_getCoreId())));
    }
    time = IfxStm_getLower(IfxStm_getAddress((IfxStm_Index)(IfxCpu_getCoreId())));
    interval_time = time - time_last;
    time_last = time;
    interval_time = (uint32)((uint64)interval_time * 1000000 / stm_clk);

    return interval_time;
}

static void uart_receiver_analysis(uart_receiver_struct *remote_data, uint8 *buffer)
{
    uint8 num = 0;

    remote_data->channel[num++] = (buffer[1] | buffer[2] << 8) & 0x07FF;
    remote_data->channel[num++] = (buffer[2] >> 3 | buffer[3] << 5) & 0x07FF;
    remote_data->channel[num++] = (buffer[3] >> 6 | buffer[4] << 2 | buffer[5] << 10) & 0x07FF;
    remote_data->channel[num++] = (buffer[5] >> 1 | buffer[6] << 7) & 0x07FF;
    remote_data->channel[num++] = (buffer[6] >> 4 | buffer[7] << 4) & 0x07FF;
    remote_data->channel[num++] = (buffer[7] >> 7 | buffer[8] << 1 | buffer[9] << 9) & 0x07FF;
    remote_data->state = (SBUS_ABNORMAL_STATE == (buffer[23] & SBUS_ABNORMAL_STATE)) ? 0 : 1;
    remote_data->frame_count++;
    uart_receiver.finsh_flag = 1;
}

void uart_receiver_reset_parser(void)
{
    uart_receiver_sync_state = UART_RECEIVER_SYNC_WAIT_HEADER;
    uart_receiver_data_length = 0;
}

void uart_receiver_note_uart_error(void)
{
    uart_receiver.uart_error_count++;
    uart_receiver.resync_count++;
    uart_receiver_reset_parser();
}

static void uart_receiver_process_byte(uint8 data)
{
    if(uart_receiver_interval_time() > 3000)
    {
        if(uart_receiver_data_length != 0 || uart_receiver_sync_state != UART_RECEIVER_SYNC_WAIT_HEADER)
        {
            uart_receiver.resync_count++;
        }
        uart_receiver_reset_parser();
    }

    if(uart_receiver_sync_state == UART_RECEIVER_SYNC_WAIT_HEADER)
    {
        if(data != FRAME_STAR)
        {
            uart_receiver.drop_count++;
            return;
        }

        uart_receiver_data[0] = data;
        uart_receiver_data_length = 1;
        uart_receiver_sync_state = UART_RECEIVER_SYNC_IN_FRAME;
        return;
    }

    uart_receiver_data[uart_receiver_data_length++] = data;
    if((REV_DATA_LEN == uart_receiver_data_length)
       && (FRAME_STAR == uart_receiver_data[0])
       && (FRAME_END == uart_receiver_data[24]))
    {
        uart_receiver_analysis(&uart_receiver, uart_receiver_data);
        uart_receiver_reset_parser();
    }
    else if(uart_receiver_data_length >= REV_DATA_LEN)
    {
        uart_receiver.resync_count++;
        if(uart_receiver_data[REV_DATA_LEN - 1U] == FRAME_STAR)
        {
            uart_receiver_data[0] = FRAME_STAR;
            uart_receiver_data_length = 1;
            uart_receiver_sync_state = UART_RECEIVER_SYNC_IN_FRAME;
        }
        else
        {
            uart_receiver_reset_parser();
        }
    }
}

static void uart_receiver_callback(void)
{
    uint8 data;

    while(uart_query_byte(UART_RECEVIER_UART_INDEX, &data))
    {
        uart_receiver_process_byte(data);
    }
}

void uart_receiver_init(void)
{
    uint8 i;

    uart_receiver.state = 0;
    uart_receiver.finsh_flag = 0;
    uart_receiver.frame_count = 0;
    uart_receiver.drop_count = 0;
    uart_receiver.resync_count = 0;
    uart_receiver.uart_error_count = 0;
    for(i = 0; i < UART_RECEVIER_CHANNEL_NUM; i++)
    {
        uart_receiver.channel[i] = 0;
    }
    uart_receiver_reset_parser();

    uart_sbus_init(UART_RECEVIER_UART_INDEX,
                   SBUS_UART_BAUDRATE,
                   UART_RECEVIER_TX_PIN,
                   UART_RECEVIER_RX_PIN);

    set_wireless_type(RECEIVER_UART, uart_receiver_callback);
}
