/*
 * display.h
 *
 *  Created on: 2025
 *      Author: 18905
 */

#ifndef CODE_DISPLAY_H_
#define CODE_DISPLAY_H_

typedef enum{
    Mode_IDLE,
    Guandao_Recode_Mode,
    Guandao_portion_1,
    Guandao_Voice,
    Guandao_Portion2_Recode,
    Guandao_portion_3,
    Rack_Test_Mode,
    YaoKong_Mode
}Mode_Choice;

extern Mode_Choice main_mode;

extern uint8 key_mode1;
extern uint8 key_mode2;
extern uint8 CarGo_Flag;
extern float *p;

#define X(x)                    8*(x)
#define Y(y)                    16*(y)

void Display_Init(void);
uint8 Key_Get(void);
void prompt(void);
void Menu_Contral(void);
int16 Menu_key_Operation_int16(int16 *param_t);
float Menu_key_Operation_float(float *param_t);
void Menu_Main(void);
void Menu_1(void);
void Menu_Parameter(void);
void Menu_2Value(void);
void Menu_Control_Value(void);
void Menu_Mode_Choice(void);
void Menu_Recode_Points(void);
void GPS_prompt(void);
void Menu_Show_Route(void);
void Show_Route(void);
void Menu_PID_P(void);
void Menu_Control_P(void);

#endif /* CODE_DISPLAY_H_ */
