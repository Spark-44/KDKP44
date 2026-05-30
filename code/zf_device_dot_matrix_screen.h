

#ifndef _zf_device_dot_matrix_screen_h_
#define _zf_device_dot_matrix_screen_h_

#include "zf_common_typedef.h"

#define DOT_MATRIX_SCREEN_SYNC_PIN      (ERU_CH7_REQ11_P20_9)   

#define DOT_MATRIX_SCREEN_SR0_PIN       (P20_0)
#define DOT_MATRIX_SCREEN_SR1_PIN       (P20_3)
#define DOT_MATRIX_SCREEN_SR2_PIN       (P00_8)
#define DOT_MATRIX_SCREEN_SR3_PIN       (P33_9)                 
#define DOT_MATRIX_SCREEN_SR4_PIN       (P14_4)
#define DOT_MATRIX_SCREEN_SR5_PIN       (P14_5)
#define DOT_MATRIX_SCREEN_SR6_PIN       (P14_6)

#define DOT_MATRIX_SCREEN_ROW_NUM       (7)

void dot_matrix_screen_scan             (void);
void dot_matrix_screen_show_string      (const char *str);
void dot_matrix_screen_set_brightness   (uint16 brightness);
void dot_matrix_screen_init             (void);
void dot_matrix_screen_show_pattern     (uint8 index);
void dot_matrix_screen_clear_pattern    (void);
uint8 dot_matrix_screen_is_pattern_active(void);
typedef enum
{
    DOT_MATRIX_PATTERN_DOUBLE_FLASH = 0,   
    DOT_MATRIX_PATTERN_TURN_LEFT,          
    DOT_MATRIX_PATTERN_TURN_RIGHT,         
    DOT_MATRIX_PATTERN_LOW_BEAM,           
    DOT_MATRIX_PATTERN_HIGH_BEAM,          
    DOT_MATRIX_PATTERN_FOG_LIGHT,          
    DOT_MATRIX_PATTERN_MAX
} dot_matrix_pattern_t;

void dot_matrix_screen_show_led_pattern(dot_matrix_pattern_t pattern);
extern uint32 dot_matrix_screen_scan_count;

#endif
