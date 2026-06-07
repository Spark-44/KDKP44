

#include "zf_common_headfile.h"

gyro_param_t Gyro_Offset;
IMU_param_t  IMU_Data;
Imu_gyro_param_t gyro_offset;
Imu_data_param_t imu_data;
euler_param_t euler_angle;
quater_param_t q_info = {1, 0, 0, 0};

float Yaw_1 = 0;
float Roll_1 = 0;
float Picth_1 = 0;

int IMU_1_Open_flag = 0;
static float param_kp = 2.12f;
static float param_ki = 0.0028f;
static float i_error_x = 0.0f;
static float i_error_y = 0.0f;
static float i_error_z = 0.0f;

#define IMU_AHRS_DELTA_T     0.0051f
#define IMU_AHRS_ACC_ALPHA   0.3f

static float Sqrt_Fast(float x)
{
    float halfx = 0.5f * x;
    float y = x;
    long i = *(long *) &y;
    i = 0x5f3759df - (i >> 1);
    y = *(float *) &i;
    y = y * (1.5f - (halfx * y * y));
    return y;
}

void IMU_init(void)
{
    imu963ra_init();  
    IMU_gyro_Offset_Init();
}

void IMU_gyro_Offset_Init(void)
{

    Gyro_Offset.Zdata = 0;
    for (uint16_t i = 0; i < 1000; i++)
    {
        imu963ra_get_gyro();
        Gyro_Offset.Zdata += imu963ra_gyro_z;
        system_delay_ms(5);   
    }

    Gyro_Offset.Zdata /= 1000.0;
}

void IMU_GetValues(void)
{

    IMU_Data.gyro_z = ((float) imu963ra_gyro_z - Gyro_Offset.Zdata)* PI / 180.0f/ 14.3f;

    if(IMU_Data.gyro_z<0.025&&IMU_Data.gyro_z>-0.025)
    {
        Yaw_1-=0;
    }
    else
    {
        IMU_Handle_180();
     }

}

void IMU_Handle_180(void)
{

    Yaw_1-=RAD_TO_ANGLE(IMU_Data.gyro_z*0.00916  );

   if(Yaw_1>180 && Yaw_1<=360)
    {
        Yaw_1-=360;
    }
    else if(Yaw_1<(-180) && Yaw_1>=(-360))
    {
        Yaw_1+=360;
    }

}

void IMU_data_get(void)
{
    imu963ra_get_gyro();

    if(IMU_1_Open_flag==1)
    {
        IMU_GetValues();
    }
}

//void IMU_text(void)
//{
//    IMU_1_Open_flag=1;
//    while(1)
//    {
//        ips_show_string(8*0, 16*0, "Yaw:");      ips_show_float(8*5,16*0,Yaw_1,3,6);
//    }
//}

void Init_Gyro_Offset(void)
{
    unsigned int i;

    gyro_offset.x_data = 0.0f;
    gyro_offset.y_data = 0.0f;
    gyro_offset.z_data = 0.0f;

    for(i = 0; i < 100;)
    {
        imu660ra_get_gyro();
        if(imu660ra_gyro_x > -10 && imu660ra_gyro_x < 10
                && imu660ra_gyro_y > -10 && imu660ra_gyro_y < 10
                && imu660ra_gyro_z > -10 && imu660ra_gyro_z < 10)
        {
            i++;
            gyro_offset.x_data += imu660ra_gyro_x;
            gyro_offset.y_data += imu660ra_gyro_y;
            gyro_offset.z_data += imu660ra_gyro_z;
        }
        system_delay_ms(1);
    }

    gyro_offset.x_data /= 100.0f;
    gyro_offset.y_data /= 100.0f;
    gyro_offset.z_data /= 100.0f;
}

static void Get_Values_ICM(void)
{
    static double last_acc[3] = {0, 0, 0};

    imu_data.acc_x = (((float)imu660ra_acc_x) * IMU_AHRS_ACC_ALPHA) + last_acc[0] * (1.0f - IMU_AHRS_ACC_ALPHA);
    imu_data.acc_y = (((float)imu660ra_acc_y) * IMU_AHRS_ACC_ALPHA) + last_acc[1] * (1.0f - IMU_AHRS_ACC_ALPHA);
    imu_data.acc_z = (((float)imu660ra_acc_z) * IMU_AHRS_ACC_ALPHA) + last_acc[2] * (1.0f - IMU_AHRS_ACC_ALPHA);

    last_acc[0] = imu_data.acc_x;
    last_acc[1] = imu_data.acc_y;
    last_acc[2] = imu_data.acc_z;

    imu_data.gyro_x = ((float)imu660ra_gyro_x - gyro_offset.x_data) * PI / 180.0f / 16.4f;
    imu_data.gyro_y = ((float)imu660ra_gyro_y - gyro_offset.y_data) * PI / 180.0f / 16.4f;
    imu_data.gyro_z = ((float)imu660ra_gyro_z - gyro_offset.z_data) * PI / 180.0f / 16.4f;
}

static void Update_AHRS_ICM(float gx, float gy, float gz, float ax, float ay, float az)
{
    float half_t = 0.5f * IMU_AHRS_DELTA_T;
    float vx, vy, vz;
    float ex, ey, ez;
    float q0 = q_info.q0;
    float q1 = q_info.q1;
    float q2 = q_info.q2;
    float q3 = q_info.q3;
    float q0q0 = q0 * q0;
    float q0q1 = q0 * q1;
    float q0q2 = q0 * q2;
    float q1q1 = q1 * q1;
    float q1q3 = q1 * q3;
    float q2q2 = q2 * q2;
    float q2q3 = q2 * q3;
    float q3q3 = q3 * q3;
    float norm = Sqrt_Fast(ax * ax + ay * ay + az * az);

    ax = ax * norm;
    ay = ay * norm;
    az = az * norm;

    vx = 2.0f * (q1q3 - q0q2);
    vy = 2.0f * (q0q1 + q2q3);
    vz = q0q0 - q1q1 - q2q2 + q3q3;

    ex = ay * vz - az * vy;
    ey = az * vx - ax * vz;
    ez = ax * vy - ay * vx;

    i_error_x += half_t * ex;
    i_error_y += half_t * ey;
    i_error_z += half_t * ez;
    gx = gx + param_kp * ex + param_ki * i_error_x;
    gy = gy + param_kp * ey + param_ki * i_error_y;
    gz = gz + param_kp * ez + param_ki * i_error_z;

    q0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * half_t;
    q1 = q1 + (q0 * gx + q2 * gz - q3 * gy) * half_t;
    q2 = q2 + (q0 * gy - q1 * gz + q3 * gx) * half_t;
    q3 = q3 + (q0 * gz + q1 * gy - q2 * gx) * half_t;

    norm = Sqrt_Fast(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q_info.q0 = q0 * norm;
    q_info.q1 = q1 * norm;
    q_info.q2 = q2 * norm;
    q_info.q3 = q3 * norm;
}

void Get_Angles_ICM(void)
{
    float q0, q1, q2, q3;

    imu660ra_get_acc();
    imu660ra_get_gyro();
    Get_Values_ICM();
    Update_AHRS_ICM(imu_data.gyro_x, imu_data.gyro_y, imu_data.gyro_z, imu_data.acc_x, imu_data.acc_y, imu_data.acc_z);

    q0 = q_info.q0;
    q1 = q_info.q1;
    q2 = q_info.q2;
    q3 = q_info.q3;

    euler_angle.pitch = asin(-2.0f * q1 * q3 + 2.0f * q0 * q2) * 180.0f / PI - 180.0f;
    euler_angle.roll = atan2(2.0f * q2 * q3 + 2.0f * q0 * q1, -2.0f * q1 * q1 - 2.0f * q2 * q2 + 1.0f) * 180.0f / PI;
    euler_angle.yaw = atan2(2.0f * q1 * q2 + 2.0f * q0 * q3, -2.0f * q2 * q2 - 2.0f * q3 * q3 + 1.0f) * 180.0f / PI;

    if(euler_angle.pitch > 0.0f)
    {
        euler_angle.pitch = 180.0f - euler_angle.pitch;
    }
    else if(euler_angle.pitch < 0.0f)
    {
        euler_angle.pitch = -(180.0f + euler_angle.pitch);
    }

    if(euler_angle.roll > 0.0f)
    {
        euler_angle.roll = 180.0f - euler_angle.roll;
    }
    else if(euler_angle.roll < 0.0f)
    {
        euler_angle.roll = -(180.0f + euler_angle.roll);
    }

    if(euler_angle.yaw > 360.0f)
    {
        euler_angle.yaw -= 360.0f;
    }
    else if(euler_angle.yaw < 0.0f)
    {
        euler_angle.yaw += 360.0f;
    }
}
