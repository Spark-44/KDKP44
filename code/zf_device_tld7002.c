

#include "zf_common_fifo.h"
#include "zf_driver_delay.h"
#include "zf_driver_gpio.h"
#include "zf_driver_uart.h"
#include "TLD7002_driver/TLD7002FuncLayer.h"

#include "zf_device_tld7002.h"

TLD7002_NetworkInstance_t tld7002_device;
fifo_struct tld7002_fifo;           
uint8       tld7002_buffer[100];    
uint8       tld7002_init_flag;      
uint16      tld7002_duty[16];       

uint16 tld7002_otp_reg[40] =
{
    0x0D09, 0x0D0D, 0x0D0D, 0x0D0D, 0x0D0D, 0x0D0D, 0x0D0D, 0x0D0D,
    0x3933, 0x3333, 0x4035, 0x5A51, 0x9966, 0x9980, 0x9999, 0xB3B3,
    0xFFFF, 0x0820, 0x0820, 0x0820, 0x0820, 0x0820, 0x0820, 0x0820,
    0x1020, 0x0000, 0x80CF, 0x094A, 0x0000, 0x2020, 0x0000, 0x0000,
    0x0000, 0x8007, 0x0001, 0x159C, 0x0000, 0x000C, 0x0000, 0xCAFE
};

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void tld7002_send_buffer(uint8 *buffer, uint32 length)
{
    
    fifo_clear(&tld7002_fifo);
    
    uart_write_buffer(TLD7002_UART_INDEX, buffer, length);
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
boolean tld7002_read_buffer(uint8 *buffer, uint32 length)
{
    boolean copy_successful = 0;
    uint8   read_buffer[50];
    uint32  read_size = 0;
    uint8   read_position = 0;

    read_size = sizeof(read_buffer);
    fifo_read_buffer(&tld7002_fifo, read_buffer, &read_size, FIFO_READ_AND_CLEAN);

    if (read_size > length)
    {
        read_position = (uint8)(read_size - length);
    }

    if(read_position != 0)
    {
        for(; read_position < read_size; read_position++)
        {
            *buffer++ = read_buffer[read_position];
        }
        copy_successful = 1;
    }
    return copy_successful;
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void tld7002_clean_buffer(void)
{
    fifo_clear(&tld7002_fifo);
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void tld7002_gpin0_set_level(uint8 state)
{
    gpio_set_level(TLD7002_GPIN0_PIN, state);
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void tld7002_set_duty(uint8 tld7002_id)
{
    uint16 err_flag;
    //char send_buff[50];

    err_flag = TLD7002setDutyReadDiag((uint16 *)tld7002_duty, &tld7002_device, tld7002_id);
    if(err_flag)
    {
        //TLD7002_TRX_HWCR_ALL (&tld7002_device, send_buff,  tld7002_id);    /* clear all TLD7002 error flags */
    }
    system_delay_us_register(70);
    TLD7002broadcastDCsync(&tld7002_device);
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void tld7002_callback(void)
{
    uint32 temp_data;
    if(tld7002_init_flag)
    {
        temp_data = uart_read_byte(TLD7002_UART_INDEX);
        fifo_write_element(&tld7002_fifo, temp_data);
    }
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void tld7002_init(void)
{
    fifo_init(&tld7002_fifo, FIFO_DATA_8BIT, tld7002_buffer, sizeof(tld7002_buffer));
    gpio_init(TLD7002_GPIN0_PIN, GPO, 0, GPO_PUSH_PULL);
    uart_init (TLD7002_UART_INDEX, TLD7002_UART_BAUD, TLD7002_UART_RX, TLD7002_UART_HLSIL);     
    uart_rx_interrupt(TLD7002_UART_INDEX, 1);
    tld7002_init_flag = 1;

    
    TLD7002initDrivers(&tld7002_device);

    
    OTPemuComplete(tld7002_otp_reg, &tld7002_device, 1, 100);

    
    TLD7002initDevice(&tld7002_device, 1);
    system_delay_us_register(100);
}
