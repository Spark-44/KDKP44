

#ifndef CODE_IMU_2_H_
#define CODE_IMU_2_H_

#define DELTA_T     0.0051f     
#define alpha           0.3f    

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
extern  euler_param_t    euler_angle ;

void Init_Gyro_Offset(void);

void Get_Angles_ICM(void);

void IMU_init(void);

#endif /* CODE_IMU_2_H_ */
