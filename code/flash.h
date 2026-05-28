

#ifndef CODE_FLASH_H_
#define CODE_FLASH_H_

extern float speed_pid[6];
extern int16 control[5];
extern float kp ;
extern float ki ;
extern float kd ;

#define RECODE_PASSAGE                  (9)   
#define RECODE_PASSAGE_TWO                  (8)   
#define RECODE_PASSAGE_THREE                  (7)   
#define RECODE_PASSAGE_FOUR                  (6)   
#define RECODE_PASSAGE_FIF                  (5)   
#define RECODE_PORTION_THREE                  (4)   
#define GPS_CHEAK_FLAG                            (3)   

void Flash_Read_gpscheak(void);

void Flash_Write_gpscheak(void);

void Flash_Read_pid(void);

void Flash_Write_pid(void);

void Flash_Write_INSpoints(void);

void Flash_Read_INSpoints(void);

void Flash_Write_passage_points(void);

void Flash_Read_passage_points(void);

void Flash_Write_portion_3points(void);

void Flash_Read_portion_3points(void);

void Flash_Main_Read(void);

void Flash_Store_Mode(uint8 route_setting_choice);
#endif /* CODE_FLASH_H_ */

