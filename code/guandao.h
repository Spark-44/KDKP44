

#ifndef CODE_GUANDAO_H_
#define CODE_GUANDAO_H_

#define ONE_TICK_DISTANCE                      0.000378f
#define PORTION2_ROUTE_COUNT                   9
#define PORTION2_ROUTE_MAX_POINTS              49
#define PORTION2_TOTAL_ROUTE_POINTS            441
#define MAX_LENGTH_INDEX                       441
#define PORTION2_GPS_PER_ROUTE                 20
#define PORTION2_TOTAL_GPS_COUNT               180
#define MAX_GPS_RECODE                         180
#if PORTION2_TOTAL_ROUTE_POINTS != PORTION2_ROUTE_COUNT * PORTION2_ROUTE_MAX_POINTS
#error "PORTION2_TOTAL_ROUTE_POINTS must match route count and per-route capacity"
#endif
#if MAX_LENGTH_INDEX < PORTION2_TOTAL_ROUTE_POINTS
#error "MAX_LENGTH_INDEX is too small for portion-2 routes"
#endif
#if PORTION2_TOTAL_GPS_COUNT != PORTION2_ROUTE_COUNT * PORTION2_GPS_PER_ROUTE
#error "PORTION2_TOTAL_GPS_COUNT must match route count and per-route capacity"
#endif
#if MAX_GPS_RECODE < PORTION2_TOTAL_GPS_COUNT
#error "MAX_GPS_RECODE is too small for portion-2 GPS records"
#endif
#define PORTION2_ROUTE_1                       0
#define PORTION2_ROUTE_2                       1
#define PORTION2_ROUTE_3                       2
#define PORTION2_ROUTE_4                       3
#define PORTION2_ROUTE_5                       4
#define PORTION2_ROUTE_ARRIVE_DIR_CHANGE      5
#define PORTION2_ROUTE_START_DIR_CHANGE       6
#define PORTION2_ROUTE_STRAIGHT               7
#define PORTION2_ROUTE_SNAKE                  8
#define PORTION_TWO_INDEX                      3
#define M_PI                                   3.14159265358979323846f
#define WHEEL_BASE                             0.724f     
#define TRACK_WIDTH                            0.594f     
#define GUANDAO_SPEED_TO_MPS                   (0.1f)
#define MIN_SPEED                              4.0f      
#define MAX_STEERING_RAD                       90.0f      
#define SLIP_CHEAK_INDEX                       4.0f       
#define START_GPS_FLAG                         1          
#define ANGLE_CORRECT_KP                       0.0f
#define CORRECT_ANGLE_1                        -90.0f     
#define CORRECT_ANGLE_3                        180.0f

typedef struct {
        float x;
        float y;
        float theta;
}state_t;

typedef struct {
        double lat;
        double lon;
        float theta;
        int16 cheak_flag;
}GPS_state;

typedef struct guandao{
        state_t current_state;              
        state_t recode_map[MAX_LENGTH_INDEX]; 
        state_t planned_map[MAX_LENGTH_INDEX];
        GPS_state recode_gpsmap[MAX_GPS_RECODE]; 

        int16 length_index;              
        int16 planned_length;
        int current_point_index;        
        uint8 plan_ready;

        int16 gps_recode_length;        
        struct guandao * next;
}guandao_state;

typedef enum {
    NONE,
    Left_Slip,
    Right_Slip,
}SLIP_Cheak;

extern SLIP_Cheak slip_state;
extern guandao_state INS;                         
extern guandao_state passage;                    //1 = route_setting_choice
extern guandao_state portion_3;                     //2 = route_setting_choice
extern guandao_state portion_2;                    //3 = route_setting_choice
extern float out_v_l ;
extern float out_v_r ;
extern float out_servo ;
extern uint8 route_setting_choice;
extern int16 daoche_point_length ;
extern float daoche_speed ;
extern float base_speed ;
extern uint16 portion3_foint_flag ;
extern uint8 daoche_flash_cheack ;
extern float persuit_threshold ;  //PURSUIT_THRESHOLD
extern float recode_threshold ;//RECORD_THRESHOLD
extern int16 preview_spets ;       //PREVIEW_SPETS
extern float final_dsts ;
extern float guandao_debug_distance;             
extern float guandao_debug_angle_diff;
extern float guandao_debug_dist_final;
extern uint8 guandao_debug_stop_reason;          
extern uint8 portion2_back_channel;
extern uint8 portion2_record_route;
extern uint8 portion2_record_state;
extern uint8 portion2_selected_route;
extern uint8 portion2_run_reverse;
extern uint8 portion2_run_drive_reverse;
extern uint8 portion2_run_last_rx;
extern uint16 portion2_run_rx_count;
extern uint8 portion2_run_reject_reason;
extern uint16 portion2_route_length[PORTION2_ROUTE_COUNT];
extern uint8 portion2_route_gps_count[PORTION2_ROUTE_COUNT];
extern const uint8 portion2_route_required_gps_count[PORTION2_ROUTE_COUNT];

void guandao_state_init(guandao_state * e);

void guandao_chain_init(void);

void update_state(guandao_state * state , Encoder_t * ecd);            
float get_distance(state_t p1, state_t p2);                                          

void recode_waypoint(guandao_state * state);                                 
void guandao_build_smooth_plan(guandao_state * state);

void pursuit_contral_mode(guandao_state * state,float * out_v_l,float * out_v_r,float *out_servo);            

void guandao_show(guandao_state * p);                                                                

void follow_points_show(guandao_state * p);                                                       

void build_map_text(guandao_state * state);                                  
float speed_calculate(Encoder_t * ecd , float time_tick);                  
void slip_cheak(Encoder_t * ecd,float steer_angle);                          
void pursuit_midhandle(guandao_state * state ,state_t * current_state , int index ,float * angle , float * distanse);

void guandao_recode(guandao_state * state);

void guandao_trace(guandao_state * state);
void guandao_trace_direct(guandao_state * p);

void portion_1(void);                         

void portion_1_reset(void);                   
uint8 portion3_points_switch(void);

void Guandao_Points_Show(guandao_state * e);

void Key_Recode_Point(guandao_state * e);

void portion2_points_recode(void);

uint8 portion2_points_build(void);

void portion2_points_trace(uint8 channal1 , uint8 channal2 ,uint8 state );
void portion2_reset(void);
void portion2_set_go_channel(uint8 channel);
void portion2_set_back_channel(uint8 channel);
void portion2_record_reset(void);
void portion2_record_enter_mode(void);
void portion2_record_mark_loaded_routes_saved(void);
void portion2_record_task(void);
uint8 portion2_mode_k4_short_event(void);
void portion2_mode_key_transition_lock(void);
void portion2_run_select_route(uint8 route_id);
void portion2_run_select_reverse_route(uint8 route_id);
void portion2_run_select_back_route(uint8 route_id);
void portion2_run_stop(void);
void portion2_run_task(void);
void portion2_serial_dump_routes(void);
void portion2_serial_dump_route(uint8 route_id);
void portion2_serial_toggle_trace(void);
uint8 portion2_is_running(void);

void azimuth_adjust(guandao_state * state , float start_d , float dist_to_final , float * target_steering , float target_yaw );
#endif /* CODE_GUANDAO_H_ */
