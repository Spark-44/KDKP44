

#ifndef CODE_FORMULA_H_
#define CODE_FORMULA_H_

typedef struct
{
    float target_val;               
    float actual_val;               

    float err;                      
    float err_last;                 //  e(k-1)
    float err_previous;             //  e(k-2)

    float Kp;               
    float Ki;               
    float Kd;               
    float limit;            
    float p_result;         
    float i_result;         
    float d_result;         

    float out;                 
    float out_max;                 
    float out_min;                 
} _pid;

typedef struct {
    
    double x;      
    double P;      

    
    double Q;      
    double R;      

    
    double last_x;     
    double velocity;   
    double dt;         

    
    double comp_ratio; 
} KalmanWithComp;

extern KalmanWithComp klm_lat;
extern KalmanWithComp klm_lon;

void PID_Place(_pid*p,float now);

void PID_Up(_pid*p,float now);

void Value_Limit_float(float* value , float min ,float max);

void Value_Limit_int(int * value , int min ,int max);

int32_t calculate_delta(int16_t current, int16_t last);

void KWC_Init(KalmanWithComp *kf, double Q, double R, double dt, double comp_ratio);

double KWC_UpdateFast(KalmanWithComp *kf, double z);
#endif /* CODE_FORMULA_H_ */
