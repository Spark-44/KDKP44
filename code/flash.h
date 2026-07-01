

#ifndef CODE_FLASH_H_
#define CODE_FLASH_H_

extern float speed_pid[6];
extern int16 control[5];
extern float kp ;
extern float ki ;
extern float kd ;

#define FLASH_SECTION_INDEX             (0)
#define SPEED_PID_PAGE_INDEX            (11)
#define RECODE_PASSAGE                  (9)   
#define RECODE_PASSAGE_TWO                  (8)   
#define RECODE_PASSAGE_THREE                  (7)   
#define RECODE_PASSAGE_GPS_CONT              (6)
#define RECODE_PASSAGE_FIF                  (5)   
#define RECODE_PORTION_THREE                  (4)   

void Flash_Read_pid(void);

void Flash_Write_pid(void);

void Flash_Write_passage_points(void);

void Flash_Read_passage_points(void);

void Flash_Write_portion_3points(void);

void Flash_Read_portion_3points(void);

void Flash_Main_Read(void);

void Flash_Store_Mode(uint8 route_setting_choice);
#endif /* CODE_FLASH_H_ */
