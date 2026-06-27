

#ifndef CODE_IMU_H_
#define CODE_IMU_H_

typedef struct{
    float Xdata;   
    float Ydata;   
    float Zdata;   
}gyro_param_t ;

typedef struct{
    float acc_x;   
    float acc_y;   
    float acc_z;   

    float gyro_x;  
    float gyro_y;  
    float gyro_z;  
}IMU_param_t ;

typedef struct {
    float x_data;
    float y_data;
    float z_data;
} Imu_gyro_param_t;

typedef struct {
    float gyro_x;
    float gyro_y;
    float gyro_z;
    float acc_x;
    float acc_y;
    float acc_z;
} Imu_data_param_t;

typedef struct {
    float q0;
    float q1;
    float q2;
    float q3;
} quater_param_t;

typedef struct {
    float pitch;
    float roll;
    float yaw;
} euler_param_t;

extern int IMU_1_Open_flag;
extern float Yaw_1;
extern float Yaw_Straight_1;
extern float Roll_1;
extern float Picth_1;
extern IMU_param_t  IMU_Data;
extern euler_param_t euler_angle;

void IMU_init(void);

void IMU_gyro_Offset_Init(void);

void IMU_GetValues(void);

void IMU_Handle_180(void);

void IMU_data_get(void);

void IMU_text(void);

void Init_Gyro_Offset(void);

void Get_Angles_ICM(void);

#endif /* CODE_IMU_H_ */
