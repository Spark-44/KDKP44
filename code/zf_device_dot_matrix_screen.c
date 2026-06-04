

#include "zf_common_interrupt.h"
#include "zf_driver_delay.h"
#include "zf_driver_gpio.h"
#include "zf_driver_exti.h"
#include "zf_device_tld7002.h"

#include "zf_device_dot_matrix_screen.h"

const uint32 dot_matrix_screen_row_pin[] = {DOT_MATRIX_SCREEN_SR0_PIN, DOT_MATRIX_SCREEN_SR1_PIN, DOT_MATRIX_SCREEN_SR2_PIN, DOT_MATRIX_SCREEN_SR3_PIN, DOT_MATRIX_SCREEN_SR4_PIN, DOT_MATRIX_SCREEN_SR5_PIN, DOT_MATRIX_SCREEN_SR6_PIN};

int8    dot_matrix_screen_data[3];          
int8    dot_matrix_screen_data_backup[3];   
uint16  dot_matrix_screen_brightness;       
uint32  dot_matrix_screen_scan_count;

static uint8 pattern_active = 0;            
static uint8 pattern_index = 0;             

const uint8 tld7002_ascii_font_5x7[][7] = {{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},  //    0
        {0x08, 0x08, 0x08, 0x08, 0x00, 0x08, 0x00},  // !  1
        {0x14, 0x14, 0x14, 0x00, 0x00, 0x00, 0x00},  // "  2
        {0x14, 0x14, 0x3E, 0x14, 0x3E, 0x14, 0x14},  // #  3
        {0x08, 0x3C, 0x0A, 0x1C, 0x28, 0x1E, 0x08},  // $  4
        {0x30, 0x32, 0x04, 0x08, 0x10, 0x26, 0x06},  // %  5
        {0x0C, 0x12, 0x0A, 0x04, 0x2A, 0x12, 0x2C},  // &  6
        {0x0C, 0x08, 0x04, 0x00, 0x00, 0x00, 0x00},  // '  7
        {0x10, 0x08, 0x04, 0x04, 0x04, 0x08, 0x10},  // (  8
        {0x04, 0x08, 0x10, 0x10, 0x10, 0x08, 0x04},  // )  9
        {0x00, 0x08, 0x2A, 0x1C, 0x2A, 0x08, 0x00},  // * 10
        {0x00, 0x08, 0x08, 0x3E, 0x08, 0x08, 0x00},  // + 11
        {0x00, 0x00, 0x00, 0x00, 0x18, 0x10, 0x08},  // , 12
        {0x00, 0x00, 0x00, 0x3E, 0x00, 0x00, 0x00},  // - 13
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C},  // . 14
        {0x00, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01},  // / 15
        {0x1C, 0x22, 0x32, 0x2A, 0x26, 0x22, 0x1C},  // 0 16
        {0x08, 0x0C, 0x08, 0x08, 0x08, 0x08, 0x1C},  // 1 17
        {0x1C, 0x22, 0x20, 0x10, 0x08, 0x04, 0x3E},  // 2 18
        {0x3E, 0x10, 0x08, 0x10, 0x20, 0x22, 0x1C},  // 3 19
        {0x10, 0x18, 0x14, 0x12, 0x3E, 0x10, 0x10},  // 4 20
        {0x3E, 0x02, 0x1E, 0x20, 0x20, 0x22, 0x1C},  // 5 21
        {0x18, 0x04, 0x02, 0x1E, 0x22, 0x22, 0x1C},  // 6 22
        {0x3E, 0x20, 0x10, 0x08, 0x04, 0x04, 0x04},  // 7 23
        {0x1C, 0x22, 0x22, 0x1C, 0x22, 0x22, 0x1C},  // 8 24
        {0x1C, 0x22, 0x22, 0x3C, 0x20, 0x10, 0x0C},  // 9 25
        {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x0C, 0x00},  // : 26
        {0x00, 0x0C, 0x0C, 0x00, 0x0C, 0x08, 0x04},  // ; 27
        {0x10, 0x08, 0x04, 0x02, 0x04, 0x08, 0x10},  // < 28
        {0x00, 0x00, 0x3E, 0x00, 0x3E, 0x00, 0x00},  // = 29
        {0x04, 0x08, 0x10, 0x20, 0x10, 0x08, 0x04},  // > 30
        {0x1C, 0x22, 0x20, 0x10, 0x08, 0x00, 0x08},  // ? 31
        {0x1C, 0x22, 0x20, 0x2C, 0x3A, 0x22, 0x1C},  // @ 32
        {0x08, 0x14, 0x22, 0x22, 0x3E, 0x22, 0x22},  // A 33
        {0x1E, 0x22, 0x22, 0x1E, 0x22, 0x22, 0x1E},  // B 34
        {0x1C, 0x22, 0x02, 0x02, 0x02, 0x22, 0x1C},  // C 35
        {0x0E, 0x12, 0x22, 0x22, 0x22, 0x12, 0x0E},  // D 36
        {0x3E, 0x02, 0x02, 0x1E, 0x02, 0x02, 0x3E},  // E 37
        {0x3E, 0x02, 0x02, 0x1E, 0x02, 0x02, 0x02},  // F 38
        {0x1C, 0x22, 0x02, 0x3A, 0x22, 0x22, 0x3C},  // G 39
        {0x22, 0x22, 0x22, 0x3E, 0x22, 0x22, 0x22},  // H 40
        {0x1C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C},  // I 41
        {0x38, 0x10, 0x10, 0x10, 0x10, 0x12, 0x0C},  // J 42
        {0x22, 0x12, 0x0A, 0x06, 0x0A, 0x12, 0x22},  // K 43
        {0x02, 0x02, 0x02, 0x02, 0x02, 0x02, 0x3E},  // L 44
        {0x22, 0x36, 0x2A, 0x2A, 0x22, 0x22, 0x22},  // M 45
        {0x22, 0x22, 0x26, 0x2A, 0x32, 0x22, 0x22},  // N 46
        {0x1C, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C},  // O 47
        {0x1E, 0x22, 0x22, 0x1E, 0x02, 0x02, 0x02},  // P 48
        {0x1C, 0x22, 0x22, 0x22, 0x2A, 0x12, 0x2C},  // Q 49
        {0x1E, 0x22, 0x22, 0x1E, 0x0A, 0x12, 0x22},  // R 50
        {0x3C, 0x02, 0x02, 0x1C, 0x20, 0x20, 0x1E},  // S 51
        {0x3E, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},  // T 52
        {0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x1C},  // U 53
        {0x22, 0x22, 0x22, 0x22, 0x22, 0x14, 0x08},  // V 54
        {0x22, 0x22, 0x22, 0x2A, 0x2A, 0x2A, 0x14},  // W 55
        {0x22, 0x22, 0x14, 0x08, 0x14, 0x22, 0x22},  // X 56
        {0x22, 0x22, 0x22, 0x14, 0x08, 0x08, 0x08},  // Y 57
        {0x3E, 0x20, 0x10, 0x08, 0x04, 0x02, 0x3E},  // Z 58
        {0x1C, 0x04, 0x04, 0x04, 0x04, 0x04, 0x1C},  // [ 59
        {0x00, 0x01, 0x02, 0x04, 0x08, 0x10, 0x20},  // \ 60
        {0x1C, 0x10, 0x10, 0x10, 0x10, 0x10, 0x1C},  // ] 61
        {0x08, 0x14, 0x22, 0x00, 0x00, 0x00, 0x00},  // ^ 62
        {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3E},  // _ 63
        {0x04, 0x08, 0x10, 0x00, 0x00, 0x00, 0x00},  // ` 64
        {0x00, 0x00, 0x1C, 0x20, 0x3C, 0x22, 0x3C},  // a 65
        {0x02, 0x02, 0x1A, 0x26, 0x22, 0x22, 0x1E},  // b 66
        {0x00, 0x00, 0x1C, 0x02, 0x02, 0x22, 0x1C},  // c 67
        {0x20, 0x20, 0x2C, 0x32, 0x22, 0x22, 0x3C},  // d 68
        {0x00, 0x00, 0x1C, 0x22, 0x3E, 0x02, 0x1C},  // e 69
        {0x18, 0x24, 0x04, 0x0E, 0x04, 0x04, 0x04},  // f 70
        {0x00, 0x3C, 0x22, 0x22, 0x3C, 0x20, 0x1C},  // g 71
        {0x02, 0x02, 0x1A, 0x26, 0x22, 0x22, 0x22},  // h 72
        {0x08, 0x00, 0x0C, 0x08, 0x08, 0x08, 0x1C},  // i 73
        {0x10, 0x00, 0x18, 0x10, 0x10, 0x12, 0x0C},  // j 74
        {0x02, 0x02, 0x12, 0x0A, 0x06, 0x0A, 0x12},  // k 75
        {0x0C, 0x08, 0x08, 0x08, 0x08, 0x08, 0x1C},  // l 76
        {0x00, 0x00, 0x16, 0x2A, 0x2A, 0x22, 0x22},  // m 77
        {0x00, 0x00, 0x1A, 0x26, 0x22, 0x22, 0x22},  // n 78
        {0x00, 0x00, 0x1C, 0x22, 0x22, 0x22, 0x1C},  // o 79
        {0x00, 0x1E, 0x22, 0x22, 0x1E, 0x02, 0x02},  // p 80
        {0x00, 0x2C, 0x32, 0x32, 0x2C, 0x20, 0x20},  // q 81
        {0x00, 0x00, 0x1A, 0x26, 0x02, 0x02, 0x02},  // r 82
        {0x00, 0x00, 0x1C, 0x02, 0x1C, 0x20, 0x1E},  // s 83
        {0x04, 0x04, 0x0E, 0x04, 0x04, 0x24, 0x18},  // t 84
        {0x00, 0x00, 0x22, 0x22, 0x22, 0x32, 0x2C},  // u 85
        {0x00, 0x00, 0x22, 0x22, 0x22, 0x14, 0x08},  // v 86
        {0x00, 0x00, 0x22, 0x22, 0x2A, 0x2A, 0x14},  // w 87
        {0x00, 0x00, 0x22, 0x14, 0x08, 0x14, 0x22},  // x 88
        {0x00, 0x22, 0x22, 0x22, 0x3C, 0x20, 0x1C},  // y 89
        {0x00, 0x00, 0x3E, 0x10, 0x08, 0x04, 0x3E},  // z 90
        {0x0C, 0x04, 0x06, 0x02, 0x06, 0x04, 0x0C},  // { 91
        {0x08, 0x08, 0x08, 0x08, 0x08, 0x08, 0x08},  // | 92
        {0x0C, 0x08, 0x18, 0x10, 0x18, 0x08, 0x0C},  // } 93]
};

const uint8 LED_DoubleFlash_Left[7]  = {0x20, 0x30, 0x38, 0x3C, 0x38, 0x30, 0x20};
const uint8 LED_DoubleFlash_Mid[7]   = {0x00, 0x00, 0x08, 0x08, 0x08, 0x00, 0x00};
const uint8 LED_DoubleFlash_Right[7] = {0x02, 0x06, 0x0E, 0x1E, 0x0E, 0x06, 0x02};

const uint8 LED_TurnLeft_Left[7]  = {0x20, 0x30, 0x38, 0x3C, 0x38, 0x30, 0x20};
const uint8 LED_TurnLeft_Mid[7]   = {0x00, 0x00, 0x3E, 0x3E, 0x3E, 0x00, 0x00};
const uint8 LED_TurnLeft_Right[7] = {0x00, 0x00, 0x1E, 0x1E, 0x1E, 0x00, 0x00};

const uint8 LED_TurnRight_Left[7]  = {0x00, 0x00, 0x3C, 0x3C, 0x3C, 0x00, 0x00};
const uint8 LED_TurnRight_Mid[7]   = {0x00, 0x00, 0x3E, 0x3E, 0x3E, 0x00, 0x00};
const uint8 LED_TurnRight_Right[7] = {0x02, 0x06, 0x0E, 0x1E, 0x0E, 0x06, 0x02};

const uint8 LED_LowBeam_Left[7]  = {0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20};
const uint8 LED_LowBeam_Mid[7]   = {0x3E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E};
const uint8 LED_LowBeam_Right[7] = {0x04, 0x08, 0x14, 0x08, 0x14, 0x08, 0x10};

const uint8 LED_HighBeam_Left[7]  = {0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20};
const uint8 LED_HighBeam_Mid[7]   = {0x3E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E};
const uint8 LED_HighBeam_Right[7] = {0x00, 0x1C, 0x00, 0x1C, 0x00, 0x1C, 0x00};

const uint8 LED_Fog_Left[7]  = {0x20, 0x10, 0x08, 0x08, 0x08, 0x10, 0x20};
const uint8 LED_Fog_Mid[7]   = {0x3E, 0x20, 0x20, 0x20, 0x20, 0x20, 0x3E};
const uint8 LED_Fog_Right[7] = {0x08, 0x1C, 0x08, 0x1C, 0x08, 0x1C, 0x08};

typedef struct {
    const uint8 *left;
    const uint8 *mid;
    const uint8 *right;
} LedPattern_t;

const LedPattern_t g_LedPatterns[] = {
    {LED_DoubleFlash_Left,  LED_DoubleFlash_Mid,  LED_DoubleFlash_Right},  
    {LED_TurnLeft_Left,     LED_TurnLeft_Mid,     LED_TurnLeft_Right},     
    {LED_TurnRight_Left,    LED_TurnRight_Mid,    LED_TurnRight_Right},    
    {LED_LowBeam_Left,      LED_LowBeam_Mid,      LED_LowBeam_Right},      
    {LED_HighBeam_Left,     LED_HighBeam_Mid,     LED_HighBeam_Right},     
    {LED_Fog_Left,          LED_Fog_Mid,          LED_Fog_Right}           
};

void dot_matrix_screen_show_led_pattern(dot_matrix_pattern_t pattern)
{
    if(pattern >= DOT_MATRIX_PATTERN_MAX)
    {
        return;
    }

    pattern_index = (uint8)pattern;
    pattern_active = 1;
}
//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void dot_matrix_screen_scan(void)
{

    uint8 i;
    static uint8 entry_num; 
    uint8 display_row_now;  

    display_row_now = entry_num / 2;
    if(0 == (entry_num % 2))
    {
        if(0 == display_row_now)
        {
            gpio_low(dot_matrix_screen_row_pin[DOT_MATRIX_SCREEN_ROW_NUM - 1]);
        }
        else
        {
            gpio_low(dot_matrix_screen_row_pin[display_row_now - 1]);
        }

        
        if(7 > display_row_now)
        {
            
            if(pattern_active)
            {
                
                const uint8 *left = g_LedPatterns[pattern_index].left;
                const uint8 *mid  = g_LedPatterns[pattern_index].mid;
                const uint8 *right= g_LedPatterns[pattern_index].right;

                
                for(i = 0; 5 > i; i++)
                {
                    if((left[display_row_now]) & ((uint16)1 << (i + 1)))
                    {
                        tld7002_duty[i] = dot_matrix_screen_brightness;
                    }
                    else
                    {
                        tld7002_duty[i] = 0;
                    }
                }

                
                for(; 10 > i; i++)
                {
                    if((mid[display_row_now]) & ((uint16)1 << (i - 5 + 1)))
                    {
                        tld7002_duty[i] = dot_matrix_screen_brightness;
                    }
                    else
                    {
                        tld7002_duty[i] = 0;
                    }
                }

                
                for(; 15 > i; i++)
                {
                    if((right[display_row_now]) & ((uint16)1 << (i - 10 + 1)))
                    {
                        tld7002_duty[i] = dot_matrix_screen_brightness;
                    }
                    else
                    {
                        tld7002_duty[i] = 0;
                    }
                }
            }
            else
            {
                
                for(i = 0; 5 > i; i++)
                {
                    if((tld7002_ascii_font_5x7[dot_matrix_screen_data_backup[0] - ' '][display_row_now]) & ((uint16)1 << (i + 1))) 
                    {
                        tld7002_duty[i] = dot_matrix_screen_brightness;
                    }
                    else
                    {
                        tld7002_duty[i] = 0;
                    }
                }

                
                for(; 10 > i; i++)
                {
                    if((tld7002_ascii_font_5x7[dot_matrix_screen_data_backup[1] - ' '][display_row_now]) & ((uint16)1 << (i - 5 + 1))) 
                    {
                        tld7002_duty[i] = dot_matrix_screen_brightness;
                    }
                    else
                    {
                        tld7002_duty[i] = 0;
                    }
                }

                
                for(; 15 > i; i++)
                {
                    if((tld7002_ascii_font_5x7[dot_matrix_screen_data_backup[2] - ' '][display_row_now]) & ((uint16)1 << (i - 10 + 1))) 
                    {
                        tld7002_duty[i] = dot_matrix_screen_brightness;
                    }
                    else
                    {
                        tld7002_duty[i] = 0;
                    }
                }
            }
        }
        tld7002_set_duty(1);
    }
    else
    {
        
        gpio_high(dot_matrix_screen_row_pin[display_row_now]);

        
        
        
    }

    dot_matrix_screen_scan_count++;
    entry_num++;
    if((DOT_MATRIX_SCREEN_ROW_NUM * 2) <= entry_num)
    {
        
        entry_num = 0;
        
        memcpy((uint8 *)dot_matrix_screen_data_backup, (uint8 *)dot_matrix_screen_data, sizeof(dot_matrix_screen_data_backup));
    }
}

//void dot_matrix_screen_show_pattern(uint8 index)
//{
//    if(index >= (sizeof(g_LedPatterns) / sizeof(g_LedPatterns[0])))
//    {

//    }
//    pattern_index = index;
//    pattern_active = 1;
//}

void dot_matrix_screen_clear_pattern(void)
{
    pattern_active = 0;
}

uint8 dot_matrix_screen_is_pattern_active(void)
{
    return pattern_active;
}

uint8 dot_matrix_screen_get_pattern_index(void)
{
    return pattern_index;
}

void dot_matrix_screen_get_pattern_ptrs(uint8 index, const uint8 **left, const uint8 **mid, const uint8 **right)
{
    if(index >= (sizeof(g_LedPatterns) / sizeof(g_LedPatterns[0])))
    {
        *left = NULL;
        *mid  = NULL;
        *right= NULL;
        return;
    }
    *left  = g_LedPatterns[index].left;
    *mid   = g_LedPatterns[index].mid;
    *right = g_LedPatterns[index].right;
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void dot_matrix_screen_show_string (const char *str)
{
    uint8 i;
    
    pattern_active = 0;
    for(i = 0; i < 3; i++)
    {
        if(0 == str[i])
        {
            break;
        }
        dot_matrix_screen_data[i] = str[i];
    }
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void dot_matrix_screen_set_brightness (uint16 brightness)
{
    (void)brightness;
    dot_matrix_screen_brightness = 0;
}

//-------------------------------------------------------------------------------------------------------------------

//-------------------------------------------------------------------------------------------------------------------
void dot_matrix_screen_init (void)
{
    uint8 i;

    gpio_init(dot_matrix_screen_row_pin[0], GPO, 0, GPO_PUSH_PULL);
    gpio_init(dot_matrix_screen_row_pin[1], GPO, 0, GPO_PUSH_PULL);
    gpio_init(dot_matrix_screen_row_pin[2], GPO, 0, GPO_PUSH_PULL);
    gpio_init(dot_matrix_screen_row_pin[3], GPO, 0, GPO_PUSH_PULL);
    gpio_init(dot_matrix_screen_row_pin[4], GPO, 0, GPO_PUSH_PULL);
    gpio_init(dot_matrix_screen_row_pin[5], GPO, 0, GPO_PUSH_PULL);
    gpio_init(dot_matrix_screen_row_pin[6], GPO, 0, GPO_PUSH_PULL);

    gpio_low(dot_matrix_screen_row_pin[0]);
    gpio_low(dot_matrix_screen_row_pin[1]);
    gpio_low(dot_matrix_screen_row_pin[2]);
    gpio_low(dot_matrix_screen_row_pin[3]);
    gpio_low(dot_matrix_screen_row_pin[4]);
    gpio_low(dot_matrix_screen_row_pin[5]);
    gpio_low(dot_matrix_screen_row_pin[6]);

    tld7002_init();

    exti_init(DOT_MATRIX_SCREEN_SYNC_PIN, EXTI_TRIGGER_FALLING);

    for(i = 0; i < 16; i++)
    {
        tld7002_duty[i] = 0;
    }
    tld7002_set_duty(1);
    system_delay_ms(10);

    
    dot_matrix_screen_set_brightness(0);
    dot_matrix_screen_data[0] = ' ';
    dot_matrix_screen_data[1] = ' ';
    dot_matrix_screen_data[2] = ' ';
    dot_matrix_screen_data_backup[0] = ' ';
    dot_matrix_screen_data_backup[1] = ' ';
    dot_matrix_screen_data_backup[2] = ' ';
    dot_matrix_screen_show_string("   ");
}
