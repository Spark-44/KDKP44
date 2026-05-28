

#include "zf_common_headfile.h"
#include "rear_motor/rear_motor.h"

guandao_state INS;                               //0 = route_setting_choice
guandao_state passage;                    //1 = route_setting_choice
guandao_state portion_3;                     //2 = route_setting_choice
guandao_state portion_2;                    //3 = route_setting_choice

SLIP_Cheak slip_state = NONE;         

uint8 route_setting_choice = 0;        

float base_speed = 10.0f;
float persuit_threshold = 0.4f;         
float recode_threshold = 0.4f;         
int16 preview_spets = 2;                  
float daoche_speed = -10.0;           
float final_dsts = 3.0f;                     
float guandao_debug_distance = 0.0f;
float guandao_debug_angle_diff = 0.0f;
float guandao_debug_dist_final = 0.0f;
uint8 guandao_debug_stop_reason = 0;
uint8 portion2_go_channel = 2;
uint8 portion2_back_channel = 2;
uint8 portion2_record_route = 0;
uint8 portion2_record_state = 0;
uint8 portion2_selected_route = 0;
uint8 portion2_run_last_rx = 0;
uint16 portion2_run_rx_count = 0;
uint8 portion2_run_reject_reason = 0;
uint16 portion2_route_length[PORTION2_ROUTE_COUNT] = {0};
uint8 portion2_route_gps_count[PORTION2_ROUTE_COUNT] = {0};
const uint8 portion2_route_required_gps_count[PORTION2_ROUTE_COUNT] = {3, 3, 3, 3, 3, 3, 3, 3, 5, 5};

int16 daoche_point_length = 0;    
uint8 daoche_flag =0;                    
uint8 daoche_flash_cheack =0;
static uint8 portion1_state_flag = 0;
static uint16 portion1_finally_length = 0;
static uint8 portion2_state_flag = 0;

#define GUANDAO_START_SEARCH_POINTS    10
#define GUANDAO_TRACE_SEARCH_POINTS    8
#define PORTION1_PARK_INDEX_WINDOW     40
#define PORTION1_PARK_ENTER_DISTANCE   2.0f
#define PORTION1_PARK_CRAWL_DISTANCE   0.8f
#define PORTION1_PARK_STOP_DISTANCE    0.35f
#define PORTION1_PARK_APPROACH_SPEED   5.0f
#define PORTION1_PARK_MIN_SPEED        2.0f
#define PORTION1_PARK_CRAWL_SPEED      3.0f
#define PORTION1_END_MIN_SPEED         3.0f

static int16 guandao_clamp_length(int16 length)
{
    if(length < 0) return 0;
    if(length > MAX_LENGTH_INDEX) return MAX_LENGTH_INDEX;
    return length;
}

static int16 guandao_route_length(guandao_state *state)
{
    if(state->plan_ready && state->planned_length > 0) return state->planned_length;
    return state->length_index;
}

static state_t guandao_route_point(guandao_state *state, int index)
{
    int16 route_length = guandao_route_length(state);
    if(route_length <= 0) return state->current_state;
    if(index < 0) index = 0;
    if(index >= route_length) index = route_length - 1;
    if(state->plan_ready && state->planned_length > 0) return state->planned_map[index];
    return state->recode_map[index];
}

static int guandao_find_closest_index(guandao_state *state, int start_index, int end_index)
{
    int best_index = start_index;
    float best_distance = 0.0f;
    int16 route_length = guandao_route_length(state);

    if(route_length <= 0) return 0;
    if(start_index < 0) start_index = 0;
    if(end_index >= route_length) end_index = route_length - 1;
    if(start_index > end_index) return start_index;

    best_distance = get_distance(state->current_state, guandao_route_point(state, start_index));
    for(int i = start_index + 1; i <= end_index; i++)
    {
        float distance = get_distance(state->current_state, guandao_route_point(state, i));
        if(distance + 0.05f < best_distance)
        {
            best_distance = distance;
            best_index = i;
        }
    }

    return best_index;
}

void guandao_state_init(guandao_state * e)
{
    e->current_point_index =0;
    e->current_state.theta =0.0f;
    e->current_state.x=0.0f;
    e->current_state.y=0.0f;
    e->length_index=0;
    e->planned_length=0;
    e->plan_ready=0;

    e ->gps_recode_length =0;

}

void guandao_chain_init(void)
{
    INS.next = &passage;
    passage.next = &portion_3;
    portion_3.next = &portion_2;
    portion_2.next = NULL;
}

float get_distance(state_t p1, state_t p2)
{
    return hypotf(p2.x - p1.x, p2.y - p1.y);
}

void update_state(guandao_state * state , Encoder_t * ecd)
{
    float delta_real_center = 0;
    float delta_real_l = 0;
    float delta_real_r = 0;
    Encoder_Get(ecd);
    switch(slip_state)
    {
        case NONE:
            delta_real_l = (float)ecd->delta_l*ONE_TICK_DISTANCE;
            delta_real_r = (float)ecd->delta_r*ONE_TICK_DISTANCE;

            break;

        case Left_Slip:
            delta_real_r = (float)ecd->delta_r*ONE_TICK_DISTANCE;
            delta_real_l = delta_real_r;

            break;

        case Right_Slip:
            delta_real_l = (float)ecd->delta_l*ONE_TICK_DISTANCE;
            delta_real_r = delta_real_l;

            break;

        default : break;
    }

    if(!daoche_flag)
    {
        delta_real_center  = (delta_real_l+delta_real_r)/2.0f;
        state->current_state.theta =Yaw_1;
    }
    else
    {
        delta_real_center  = -(delta_real_l+delta_real_r)/2.0f;
        state->current_state.theta =Yaw_1+180.0f;
        angle_plan(&state->current_state.theta);
    }

    state->current_state.x+=delta_real_center*sinf(state->current_state.theta/180.0f*M_PI);
    state->current_state.y+=delta_real_center*cosf(state->current_state.theta/180.0f*M_PI);

}

void portion_1_reset(void)
{
    portion1_state_flag = 0;
    portion1_finally_length = 0;
    INS.length_index = guandao_clamp_length(INS.length_index);
    INS.current_point_index = 0;
    INS.planned_length = 0;
    INS.plan_ready = 0;
    daoche_flag = 0;
    out_v_l = 0;
    out_v_r = 0;
    out_servo = 0;
    INS.current_state.x = 0.0f;
    INS.current_state.y = 0.0f;
    INS.current_state.theta = 0.0f;
    Encoder_count_init(&guandao_ecd);
    Encoder_count_init(&Speed_ecd);
    encoder_clear_count(ENCODER_QUADDEC);
    rear_motor_stop();

    if(INS.length_index > 1)
    {
        int end_index = INS.length_index - 1;
        if(end_index > GUANDAO_START_SEARCH_POINTS) end_index = GUANDAO_START_SEARCH_POINTS;
        INS.current_point_index = guandao_find_closest_index(&INS, 1, end_index);
    }
}

void portion_1(void)
{
    update_state(&INS,&guandao_ecd);                              
    if(portion1_state_flag == 0)                                   
    {
        portion1_finally_length = INS.length_index;
        if(daoche_point_length > 0 && daoche_point_length < portion1_finally_length)
        {
            INS.length_index = daoche_point_length;                
        }
        else
        {
            INS.length_index = portion1_finally_length;            
        }
        guandao_build_smooth_plan(&INS);
        INS.current_point_index = 0;
        if(INS.planned_length > 1)
        {
            int end_index = INS.planned_length - 1;
            if(end_index > GUANDAO_START_SEARCH_POINTS) end_index = GUANDAO_START_SEARCH_POINTS;
            INS.current_point_index = guandao_find_closest_index(&INS, 1, end_index);
        }
        portion1_state_flag = 1;
    }

    pursuit_contral_mode(&INS ,&out_v_l ,&out_v_r ,&out_servo);
    if(INS.current_point_index >= guandao_route_length(&INS))
    {
        out_v_l = 0;
        out_v_r = 0;
        out_servo = 0;
        conrtol_mode = GUANDAO;
    }
    follow_points_show(&INS);
}

void recode_waypoint(guandao_state * state)
{
    if(state ->length_index >=MAX_LENGTH_INDEX)return;

    if(state ->length_index ==0)
    {
        state->recode_map[state->length_index] =state->current_state;
        state->length_index++;
        return;
    }

    state_t last_recoded =  state->recode_map[state->length_index-1];
    float dist = get_distance(state->current_state, last_recoded );

    if(dist >=recode_threshold)
    {
        state->recode_map[state->length_index] =state->current_state;
        state->length_index++;
    }

    static uint8 dche_flag = 1;
    if(state->length_index >= MAX_LENGTH_INDEX)return;
    if(state == &INS && (key1_flag == 1|| x6f_out[3] ==200) && dche_flag ==1)  
    {
        key1_flag =0;
        dche_flag =0;
        state->recode_map[state->length_index] =state->current_state;
        daoche_point_length = state->length_index;
        state->length_index++;
        daoche_flag =1;
        daoche_flash_cheack =1;
    }
}
void guandao_build_smooth_plan(guandao_state * state)
{
    int16 source_length = guandao_clamp_length(state->length_index);
    state->planned_length = 0;
    state->plan_ready = 0;

    if(source_length <= 0) return;
    if(source_length < 3)
    {
        for(int i = 0; i < source_length; i++)
        {
            state->planned_map[i] = state->recode_map[i];
        }
        state->planned_length = source_length;
        state->plan_ready = 1;
        return;
    }

    int samples = (MAX_LENGTH_INDEX - 1) / (source_length - 1);
    if(samples < 1) samples = 1;
    if(samples > 6) samples = 6;

    for(int i = 0; i < source_length - 1 && state->planned_length < MAX_LENGTH_INDEX - 1; i++)
    {
        state_t p0 = state->recode_map[(i > 0) ? i - 1 : i];
        state_t p1 = state->recode_map[i];
        state_t p2 = state->recode_map[i + 1];
        state_t p3 = state->recode_map[(i + 2 < source_length) ? i + 2 : i + 1];

        for(int j = 0; j < samples && state->planned_length < MAX_LENGTH_INDEX - 1; j++)
        {
            float t = (float)j / (float)samples;
            float t2 = t * t;
            float t3 = t2 * t;
            state_t out;
            out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
            out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
            out.theta = p1.theta + (p2.theta - p1.theta) * t;
            state->planned_map[state->planned_length] = out;
            state->planned_length++;
        }
    }

    state->planned_map[state->planned_length] = state->recode_map[source_length - 1];
    state->planned_length++;
    state->plan_ready = 1;
}

void portion2_points_recode(void)
{
    static int16 p2p_r_flag1= 0 ;
//    static uint8 p2p_r_flag2= 0;
    if(key1_flag == 1)
    {
        key1_flag = 0;
        p2p_r_flag1 = passage.length_index;
        Key_Recode_Point(&passage);

    }
    if(passage.length_index - p2p_r_flag1<=PORTION_TWO_INDEX -1)
    {
        recode_waypoint(&passage);
    }

}

void pursuit_contral_mode(guandao_state * state,float * out_v_l,float * out_v_r,float *out_servo)
{
    float actual_ld = 0 , preview_alpha =0;
    float actual_ld2 = 0 , preview_alpha2 =0;
    float target_steering = 0;
    int16 route_length = guandao_route_length(state);

    guandao_debug_stop_reason = 0;
    if(route_length == 0 || state->current_point_index == route_length)
    {
        guandao_debug_stop_reason = 1;
        * out_v_l = 0;
        * out_v_r = 0;
        *out_servo = 0;
//        Buzzer_check(50);
        return;
    }

    state_t current_point = state->current_state;
    if(state->current_point_index < 0) state->current_point_index = 0;
    if(state->current_point_index >= route_length) state->current_point_index = route_length - 1;

    int search_end_index = state->current_point_index + GUANDAO_TRACE_SEARCH_POINTS;
    if(search_end_index >= route_length) search_end_index = route_length - 1;
    int closest_index = guandao_find_closest_index(state, state->current_point_index, search_end_index);
    if(closest_index > state->current_point_index)
    {
        state->current_point_index = closest_index;
        guandao_debug_stop_reason = 5;
    }

    state_t target_point = guandao_route_point(state, state->current_point_index);

    float dx = target_point.x - current_point.x;
    float dy = target_point.y - current_point.y;
    float distance_to_target = hypotf(dx, dy);
    guandao_debug_distance = distance_to_target;
    float angle_to_target = atan2f(dx,dy)/M_PI*180.0f;
    float angle_diff = angle_to_target - state->current_state.theta;

    while (angle_diff > 180.0f) angle_diff -= 360.0f;
    while (angle_diff < -180.0f) angle_diff += 360.0f;
    guandao_debug_angle_diff = angle_diff;

    
    
    if(distance_to_target <= persuit_threshold)
    {
        if(guandao_debug_stop_reason == 0) guandao_debug_stop_reason = 2;
        state->current_point_index++;
        if(state->current_point_index >= route_length )
        {   state->current_point_index = route_length;
            guandao_debug_stop_reason = 4;
            * out_v_l = 0;
            * out_v_r = 0;
            *out_servo = 0;
//            Buzzer_check(50);
            return;
        }
    }

    
    pursuit_midhandle(state , &current_point , preview_spets , &preview_alpha , &actual_ld);
    pursuit_midhandle(state , &current_point , 5 , &preview_alpha2 , &actual_ld2);

   float k = 0;
   k = -0.0038*fabs(preview_alpha2) + 1;
   Value_Limit_float(&k ,0.6 ,1);

   if(angle_diff >=90)
   {
       target_steering = 30.0f;

   }
   else if(angle_diff <= -90)
   {
       target_steering = -30.0f;
   }
   else if(fabsf(angle_diff) <=90)
   {
       target_steering = 3.0f*atan2f(2.0f * WHEEL_BASE * sinf(preview_alpha/180.0f*M_PI), actual_ld)/M_PI*180.0f;
   }
   ips200_show_float(X(10),  Y(9),target_steering ,5 ,5);

   
   Value_Limit_float(&target_steering ,-MAX_STEERING_RAD,MAX_STEERING_RAD);

//   slip_cheak(&guandao_ecd,target_steering);

   state_t final_point = guandao_route_point(state, route_length - 1);
   float dist_to_final = get_distance(state->current_state, final_point);
   guandao_debug_dist_final = dist_to_final;
   float v_center = base_speed;
   uint8 portion1_parking_zone = 0;

   if(main_mode == Guandao_portion_1 && state == &INS &&
      state->current_point_index >= route_length - PORTION1_PARK_INDEX_WINDOW &&
      dist_to_final < PORTION1_PARK_ENTER_DISTANCE)
   {
       portion1_parking_zone = 1;
   }

   if(portion1_parking_zone)
   {
       float final_dx = final_point.x - current_point.x;
       float final_dy = final_point.y - current_point.y;
       float final_angle = atan2f(final_dx, final_dy) / M_PI * 180.0f;
       float final_angle_diff = final_angle - state->current_state.theta;

       while(final_angle_diff > 180.0f) final_angle_diff -= 360.0f;
       while(final_angle_diff < -180.0f) final_angle_diff += 360.0f;

       guandao_debug_angle_diff = final_angle_diff;

       if(dist_to_final <= PORTION1_PARK_STOP_DISTANCE)
       {
           state->current_point_index = route_length;
           guandao_debug_stop_reason = 4;
           *out_v_l = 0;
           *out_v_r = 0;
           *out_servo = 0;
           return;
       }

       if(final_angle_diff >= 90.0f)
       {
           target_steering = 30.0f;
       }
       else if(final_angle_diff <= -90.0f)
       {
           target_steering = -30.0f;
       }
       else
       {
           float final_ld = dist_to_final;
           if(final_ld < 0.1f) final_ld = 0.1f;
           target_steering = 3.0f * atan2f(2.0f * WHEEL_BASE * sinf(final_angle_diff / 180.0f * M_PI), final_ld) / M_PI * 180.0f;
       }

       Value_Limit_float(&target_steering, -MAX_STEERING_RAD, MAX_STEERING_RAD);
       guandao_debug_stop_reason = 6;
   }

   switch(route_setting_choice)
   {
       case 0:
           azimuth_adjust(state , 1.3 , dist_to_final , &target_steering ,CORRECT_ANGLE_1);
           break;
       case 2:
           azimuth_adjust(state , 5.5 , dist_to_final , &target_steering ,CORRECT_ANGLE_3);
           break;
       case 3:

           break;
       default : break;
   }

   if(portion1_parking_zone)
   {
       v_center = base_speed * (dist_to_final / PORTION1_PARK_ENTER_DISTANCE);
       if(v_center > base_speed) v_center = base_speed;
       if(v_center > PORTION1_PARK_APPROACH_SPEED) v_center = PORTION1_PARK_APPROACH_SPEED;
       if(dist_to_final < PORTION1_PARK_CRAWL_DISTANCE && v_center > PORTION1_PARK_CRAWL_SPEED)
       {
           v_center = PORTION1_PARK_CRAWL_SPEED;
       }
       if(v_center < PORTION1_PARK_MIN_SPEED) v_center = PORTION1_PARK_MIN_SPEED;
   }
   else if (dist_to_final < final_dsts && state->current_point_index >= route_length - 30)
   {

       persuit_threshold = persuit_threshold*(dist_to_final / final_dsts);
       if(persuit_threshold < 0.3f){persuit_threshold = 0.3f;}
       v_center = base_speed * (dist_to_final / final_dsts);
       if (v_center < PORTION1_END_MIN_SPEED) v_center = PORTION1_END_MIN_SPEED;
   }

   
   float w = (v_center * tanf(target_steering/3.0f/180.0f*M_PI)) / WHEEL_BASE;
   *out_v_l = v_center + (w * TRACK_WIDTH / 2.0f);
   *out_v_r = v_center - (w * TRACK_WIDTH / 2.0f);

   *out_servo = -target_steering;

}

void azimuth_adjust(guandao_state * state ,float start_d , float dist_to_final , float * target_steering , float target_yaw )
{
    float angle_delta = 0;
    int16 route_length = guandao_route_length(state);
    if(dist_to_final < start_d && state->current_point_index >= route_length - 30)
    {
        if(!daoche_flag)angle_delta  = target_yaw - Yaw_1;
        else angle_delta  = -(target_yaw - Yaw_1);
        angle_plan(&angle_delta);

        float kp_d = (1 - ANGLE_CORRECT_KP)/start_d*dist_to_final +ANGLE_CORRECT_KP;
        float kp_y = (ANGLE_CORRECT_KP -1)*(dist_to_final/start_d - 1);

        * target_steering = kp_d*(* target_steering) + kp_y *angle_delta;
    }
}

void pursuit_midhandle(guandao_state * state ,state_t * current_state , int index ,float * angle , float * distanse)
{
    int preview_index = 0;
    int16 route_length = guandao_route_length(state);
    if(route_length <= 0)
    {
        *angle = 0.0f;
        *distanse = 0.1f;
        return;
    }
    if(main_mode == Guandao_Recode_Mode)
    {
        preview_index = state->length_index - 1;
    }
    else
    {
        preview_index = state->current_point_index+index;
    }

   if(preview_index >= route_length)preview_index = route_length - 1;

   state_t preview_point = guandao_route_point(state, preview_index);

   float p_dx = preview_point.x - current_state->x;
   float p_dy = preview_point.y - current_state->y;
    * angle = atan2f(p_dx,p_dy)/M_PI*180.0f - state->current_state.theta;

   while (* angle > 180.0f) * angle -= 360.0f;
   while (* angle < -180.0f) * angle += 360.0f;

    * distanse = hypotf(p_dx, p_dy);
    if(*distanse < 0.1f) *distanse = 0.1f;

}

void build_map_text(guandao_state * state)
{
    float length = 0.176f *6.0;
    for(int i = 0 ; i<=5 ;i++)
    {
        state->recode_map[i].x = length * i;
        state->recode_map[i].y = length * i;
        state->recode_map[i].theta = 45.0f;
    }
    for(int i = 6 ; i<=10 ;i++)
    {
        state->recode_map[i].x = length* 10 - length * i;
        state->recode_map[i].y = length * i;
        state->recode_map[i].theta = 45.0f;
    }
    for(int i = 11 ; i<=15 ;i++)
    {
        state->recode_map[i].x =length* 10 - length* i;
        state->recode_map[i].y = length* 20 - length * i;
        state->recode_map[i].theta = 45.0f;
    }
    for(int i = 16 ; i<=20 ;i++)
    {
        state->recode_map[i].x =-length* 20 + length * i;

        state->recode_map[i].y = length* 20 - length* i;
        state->recode_map[i].theta = 45.0f;
    }
    state->length_index = 20;
    daoche_point_length =20;

//    for(int i = 0 ; i<20 ;i++)
//    {
//        state->recode_map[i].x = 0;
//        state->recode_map[i].y = length * i;
//        state->recode_map[i].theta = 0.0f;
//    }

}

float out_v_l = 0;
float out_v_r = 0;
float out_servo = 0;

void guandao_recode(guandao_state * state)
{
    static uint8 flag0 = 1;                                                 
    static uint8 flag1 = 1;                                                 
    static uint32 key1_save_start_ms = 0;
    static uint8 key1_save_wait_release = 0;
    uint32 now_ms = 0;
    int choice_flag = 0;                                                    

    guandao_state * p = state;                                          
    while(choice_flag < route_setting_choice)               
                                                                                        // route_setting_choice: 0=INS, 1=passage, 2=portion_3, 3=portion_2
    {
        p = p->next;                                                        
        if(p == NULL)return;                                            
        choice_flag++;                                                  
    }

    if(flag0){  guandao_state_init(p); daoche_point_length = 0; daoche_flash_cheack = 0;  flag0 =0;}          
    update_state(p  , &guandao_ecd);                        

    
    
    if(gpio_get_level(KEY1) == 0)
    {
        key1_flag = 0;
        now_ms = system_getval_ms();
        if(key1_save_start_ms == 0) key1_save_start_ms = now_ms;
        if((uint32)(now_ms - key1_save_start_ms) > 1500 && flag1)
        {
            Flash_Store_Mode(route_setting_choice);
            Buzzer_check(50);
            flag1 = 0;
            key1_save_wait_release = 1;
        }
        return;
    }
    else
    {
        if(key1_save_start_ms != 0 && key1_save_wait_release == 0)
        {
            key1_flag = 1;                 
        }
        key1_save_start_ms = 0;
        if(key1_save_wait_release)
        {
            key1_save_wait_release = 0;
            return;
        }
    }
    if( p == &passage)portion2_points_recode();     
    else recode_waypoint(p);                                         

    if(GPS_WORK_FLAG){if(key2_flag == 1){ key2_flag = 0 ; recode_gps(p);  }}        
//     guandao_show();

    if((x6f_out[2] == 200)&&flag1){   Flash_Store_Mode(route_setting_choice);  Buzzer_check(50);  flag1 = 0; };    
    

}

uint8 portion2_points_build(void)
{

    for( uint8 i = PORTION_TWO_INDEX*5 , j =PORTION_TWO_INDEX*0 ; i <PORTION_TWO_INDEX*5 +PORTION_TWO_INDEX; i++ , j++)
    {
        passage.recode_map[i].x =  passage.recode_map[j].x;
        passage.recode_map[i].y =  passage.recode_map[j].y;
    }
    for( uint8 i = PORTION_TWO_INDEX*6  , j =PORTION_TWO_INDEX*2 ; i <PORTION_TWO_INDEX*6 +PORTION_TWO_INDEX; i++ ,j++)
    {
        passage.recode_map[i].x =  passage.recode_map[j].x;
        passage.recode_map[i].y =  passage.recode_map[j].y;
    }
    for( uint8 i = PORTION_TWO_INDEX*7 , j =PORTION_TWO_INDEX*4; i <PORTION_TWO_INDEX*7 +PORTION_TWO_INDEX; i++ , j++)
    {
        passage.recode_map[i].x =  passage.recode_map[j].x;
        passage.recode_map[i].y =  passage.recode_map[j].y;
    }
    for( uint8 i = PORTION_TWO_INDEX*0 , j =PORTION_TWO_INDEX*4-1; i <PORTION_TWO_INDEX*0 +PORTION_TWO_INDEX; i++ ,j--)
    {
        passage.recode_map[i].x =  passage.recode_map[j].x;
        passage.recode_map[i].y =  passage.recode_map[j].y;
    }
    for( uint8 i = PORTION_TWO_INDEX*3  , j =PORTION_TWO_INDEX*0 ; i <PORTION_TWO_INDEX*3 +PORTION_TWO_INDEX; i++ ,j++)
    {
        passage.recode_map[i].x =  passage.recode_map[j].x;
        passage.recode_map[i].y =  passage.recode_map[j].y;
    }
    for( uint8 i = PORTION_TWO_INDEX*1  , j =PORTION_TWO_INDEX*1; i <PORTION_TWO_INDEX*1 +PORTION_TWO_INDEX; i++,j++)
    {
        passage.recode_map[i].x =  passage.recode_map[j].x;
        passage.recode_map[i].y =  passage.recode_map[j].y;
    }
    float index = (passage.recode_map[PORTION_TWO_INDEX * 3].x - passage.recode_map[PORTION_TWO_INDEX].x)/2.0f;
    for(uint8 i = 0 ,j = PORTION_TWO_INDEX *2 , m = PORTION_TWO_INDEX *4; i<PORTION_TWO_INDEX ; i++ ,j++ , m++)
    {
        passage.recode_map[i].x =  passage.recode_map[PORTION_TWO_INDEX].x - index;
        passage.recode_map[i].y =  passage.recode_map[PORTION_TWO_INDEX].y +recode_threshold*i;
        passage.recode_map[j].x =  passage.recode_map[PORTION_TWO_INDEX].x + index;
        passage.recode_map[j].y =  passage.recode_map[PORTION_TWO_INDEX].y+recode_threshold*i;
        passage.recode_map[m].x =  passage.recode_map[3*PORTION_TWO_INDEX].x + index;
        passage.recode_map[m].y =  passage.recode_map[PORTION_TWO_INDEX].y+recode_threshold*i;
    }
    passage.length_index = PORTION_TWO_INDEX*8;
    return 1;

}

static void portion2_clear_route(void)
{
    portion_2.length_index = 0;
    portion_2.planned_length = 0;
    portion_2.plan_ready = 0;
    portion_2.current_point_index = 0;
    portion_2.current_state.x = 0.0f;
    portion_2.current_state.y = 0.0f;
    portion_2.current_state.theta = 0.0f;
}

static uint16 portion2_route_offset(uint8 route_id)
{
    return (uint16)route_id * PORTION2_ROUTE_MAX_POINTS;
}

static uint8 portion2_route_required_gps(uint8 route_id)
{
    if(route_id >= PORTION2_ROUTE_COUNT) return 0;
    return portion2_route_required_gps_count[route_id];
}

static uint8 portion2_route_gps_offset(uint8 route_id)
{
    uint8 offset = 0;

    if(route_id > PORTION2_ROUTE_COUNT) route_id = PORTION2_ROUTE_COUNT;
    for(uint8 i = 0; i < route_id; i++)
    {
        offset += portion2_route_required_gps(i);
    }
    return offset;
}

static void portion2_record_point(void)
{
    uint16 len = portion2_route_length[portion2_record_route];
    uint16 offset = portion2_route_offset(portion2_record_route);

    if(portion2_record_route >= PORTION2_ROUTE_COUNT) return;
    if(len >= PORTION2_ROUTE_MAX_POINTS) return;

    if(len == 0)
    {
        passage.recode_map[offset] = passage.current_state;
        portion2_route_length[portion2_record_route] = 1;
        return;
    }

    if(get_distance(passage.current_state, passage.recode_map[offset + len - 1]) >= recode_threshold)
    {
        passage.recode_map[offset + len] = passage.current_state;
        portion2_route_length[portion2_record_route]++;
    }
}

static void portion2_record_gps_point(void)
{
    uint8 gps_count = portion2_route_gps_count[portion2_record_route];
    uint8 gps_index = portion2_route_gps_offset(portion2_record_route) + gps_count;
    uint8 gps_required = portion2_route_required_gps(portion2_record_route);

    if(portion2_record_route >= PORTION2_ROUTE_COUNT) return;
    if(gps_count >= gps_required) return;
    if(gps_index >= MAX_GPS_RECODE) return;

    passage.recode_gpsmap[gps_index].lat = gnss.latitude;
    passage.recode_gpsmap[gps_index].lon = gnss.longitude;
    passage.recode_gpsmap[gps_index].theta = Yaw_1;
    passage.recode_gpsmap[gps_index].cheak_flag = portion2_route_length[portion2_record_route];
    portion2_route_gps_count[portion2_record_route]++;
    passage.gps_recode_length = portion2_route_gps_offset(portion2_record_route) + portion2_route_gps_count[portion2_record_route];
    Buzzer_check(30);
}

void portion2_reset(void)
{
    portion2_state_flag = 0;
    portion2_clear_route();
    out_v_l = 0;
    out_v_r = 0;
    out_servo = 0;
}

void portion2_set_go_channel(uint8 channel)
{
    if(channel > 4) channel = 4;
    portion2_go_channel = channel;
}

void portion2_set_back_channel(uint8 channel)
{
    if(channel > 4) channel = 4;
    portion2_back_channel = channel;
}

void guandao_trace_direct(guandao_state * p)
{
    update_state(p,&guandao_ecd);
    pursuit_contral_mode(p ,&out_v_l ,&out_v_r ,&out_servo);
    if(GPS_WORK_FLAG)trace_gps(p);
    follow_points_show(p);
}

void portion2_points_trace(uint8 channal1 , uint8 channal2 ,uint8 state )
{
    if(state == 0) return;
    portion2_run_select_route(channal1);
}

void portion2_record_reset(void)
{
    portion2_record_route = 0;
    portion2_record_state = 0;
    guandao_state_init(&passage);
    for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
    {
        portion2_route_length[i] = 0;
        portion2_route_gps_count[i] = 0;
    }
    passage.length_index = PORTION2_ROUTE_COUNT * PORTION2_ROUTE_MAX_POINTS;
    passage.gps_recode_length = 0;
}

void portion2_record_task(void)
{
    if(portion2_record_route >= PORTION2_ROUTE_COUNT)
    {
        portion2_record_state = 3;
    }

    switch(portion2_record_state)
    {
        case 0:
        case 2:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            if(key1_flag)
            {
                key1_flag = 0;
                guandao_state_init(&passage);
                passage.current_state.theta = Yaw_1;
                Encoder_Get(&guandao_ecd);
                passage.length_index = PORTION2_ROUTE_COUNT * PORTION2_ROUTE_MAX_POINTS;
                portion2_route_length[portion2_record_route] = 0;
                portion2_route_gps_count[portion2_record_route] = 0;
                portion2_record_point();
                portion2_record_state = 1;
                Buzzer_check(50);
            }
            break;
        case 1:
            update_state(&passage, &guandao_ecd);
            portion2_record_point();
            if(key3_flag)
            {
                key3_flag = 0;
                if(GPS_WORK_FLAG) portion2_record_gps_point();
            }
            if(key2_flag)
            {
                key2_flag = 0;
                if(portion2_route_gps_count[portion2_record_route] >= portion2_route_required_gps(portion2_record_route))
                {
                    portion2_record_route++;
                    portion2_record_state = (portion2_record_route >= PORTION2_ROUTE_COUNT) ? 3 : 2;
                    Buzzer_check(100);
                }
                else
                {
                    Buzzer_check(20);
                }
            }
            break;
        case 3:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            if(key4_flag)
            {
                key4_flag = 0;
                Flash_Write_passage_points();
                Buzzer_check(200);
            }
            break;
        default:
            portion2_record_reset();
            break;
    }

    ips200_show_string(X(1), Y(8), "P2REC");
    ips200_show_int(X(8), Y(8), portion2_record_route + 1, 2);
    ips200_show_string(X(1), Y(9), "State");
    ips200_show_int(X(8), Y(9), portion2_record_state, 2);
    ips200_show_string(X(1), Y(10), "Len");
    ips200_show_int(X(6), Y(10), portion2_route_length[portion2_record_route < PORTION2_ROUTE_COUNT ? portion2_record_route : PORTION2_ROUTE_COUNT - 1], 4);
    ips200_show_string(X(1), Y(11), "GPS");
    ips200_show_int(X(6), Y(11), portion2_route_gps_count[portion2_record_route < PORTION2_ROUTE_COUNT ? portion2_record_route : PORTION2_ROUTE_COUNT - 1], 2);
}

void portion2_run_select_route(uint8 route_id)
{
    portion2_run_reject_reason = 0;
    if(route_id >= PORTION2_ROUTE_COUNT)
    {
        portion2_run_reject_reason = 1;
        return;
    }
    if(portion2_route_length[route_id] == 0)
    {
        portion2_run_reject_reason = 2;
        return;
    }
    portion2_selected_route = route_id;
    portion2_state_flag = 1;
}

void portion2_run_stop(void)
{
    portion2_state_flag = 0;
    out_v_l = 0;
    out_v_r = 0;
    out_servo = 0;
}

void portion2_run_task(void)
{
    uint16 offset;
    uint16 len;

    switch(portion2_state_flag)
    {
        case 0:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            break;
        case 1:
            portion2_clear_route();
            Encoder_Get(&guandao_ecd);
            offset = portion2_route_offset(portion2_selected_route);
            len = portion2_route_length[portion2_selected_route];
            if(len > PORTION2_ROUTE_MAX_POINTS) len = PORTION2_ROUTE_MAX_POINTS;
            for(uint16 i = 0; i < len && i < MAX_LENGTH_INDEX; i++)
            {
                portion_2.recode_map[i] = passage.recode_map[offset + i];
            }
            portion_2.length_index = len;
            portion_2.gps_recode_length = portion2_route_gps_count[portion2_selected_route];
            if(portion_2.gps_recode_length > portion2_route_required_gps(portion2_selected_route))
            {
                portion_2.gps_recode_length = portion2_route_required_gps(portion2_selected_route);
            }
            for(uint8 i = 0; i < portion_2.gps_recode_length; i++)
            {
                portion_2.recode_gpsmap[i] = passage.recode_gpsmap[portion2_route_gps_offset(portion2_selected_route) + i];
            }
            guandao_build_smooth_plan(&portion_2);
            portion2_state_flag = 2;
            break;
        case 2:
            guandao_trace_direct(&portion_2);
            if(portion_2.current_point_index >= guandao_route_length(&portion_2))
            {
                out_v_l = 0;
                out_v_r = 0;
                out_servo = 0;
                portion2_state_flag = 3;
            }
            break;
        case 3:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            portion2_state_flag = 0;
            break;
        default:
            portion2_reset();
            break;
    }

    ips200_show_string(X(1), Y(8), "P2RUN");
    ips200_show_string(X(1), Y(9), "Route");
    ips200_show_int(X(8), Y(9), portion2_selected_route + 1, 2);
    ips200_show_string(X(1), Y(10), "State");
    ips200_show_int(X(8), Y(10), portion2_state_flag, 2);
    ips200_show_string(X(1), Y(11), "Len");
    ips200_show_int(X(6), Y(11), portion_2.length_index, 4);
    ips200_show_string(X(1), Y(12), "Idx");
    ips200_show_int(X(6), Y(12), portion_2.current_point_index, 4);
    ips200_show_string(X(1), Y(13), "RX");
    ips200_show_int(X(5), Y(13), portion2_run_last_rx, 3);
    ips200_show_string(X(10), Y(13), "Cnt");
    ips200_show_int(X(15), Y(13), portion2_run_rx_count, 4);
    ips200_show_string(X(1), Y(14), "RLen");
    ips200_show_int(X(7), Y(14), portion2_route_length[portion2_selected_route], 4);
    ips200_show_string(X(12), Y(14), "Rej");
    ips200_show_int(X(17), Y(14), portion2_run_reject_reason, 2);
    ips200_show_string(X(1), Y(15), "Scn");
    ips200_show_int(X(6), Y(15), dot_matrix_screen_scan_count, 6);
}

void guandao_trace(guandao_state * state)
{
//    static uint8 flag2 = 1;
    int choice_flag = 0;                           

    guandao_state * p = state;                
    while(choice_flag < route_setting_choice)
    {
        p = p->next;                             
        if(p == NULL)return;              
        choice_flag++;
    }
    update_state(p,&guandao_ecd); 

    
    pursuit_contral_mode(p ,&out_v_l ,&out_v_r ,&out_servo);
    if(GPS_WORK_FLAG)trace_gps(p);                              
    follow_points_show(p);
//    follow_points_show();

}

float speed_calculate(Encoder_t * ecd , float time_tick)
{
    float v_speed = (ecd->delta_l +ecd->delta_r)*ONE_TICK_DISTANCE/time_tick/2.0f;
            return v_speed;
}

void slip_cheak(Encoder_t * ecd,float steer_angle)
{
    static int flag = 0;

    float v_speed = 0 ;
    v_speed =speed_calculate(ecd , 0.007);
    float w = (v_speed * tanf(steer_angle/180.0f*M_PI)) / WHEEL_BASE;

    float slip_index = fabs(w - IMU_Data.gyro_z);

//    ips200_show_int(X(1),  Y(1) ,flag, 5);
//    ips200_show_float(X(1),  Y(2) ,v_speed,3, 2);
//    ips200_show_float(X(1),  Y(3) ,w,3, 2);
//    ips200_show_float(X(1),  Y(4) ,slip_index,3, 2);

    if(v_speed >= 1.0 &&  slip_index >= SLIP_CHEAK_INDEX)
    {
        flag++;

        if(ecd->delta_l > ecd->delta_r){ slip_state =Left_Slip; }
        else if(ecd->delta_r >=ecd->delta_l ) {slip_state =Right_Slip;}
    }
    else slip_state =NONE;
}

uint16 portion3_foint_flag = 0;

uint8 portion3_points_switch(void)
{
    float x_delta = portion_3.recode_map[portion_3.length_index -1 ].x ;
    int8 cut_length =(uint8)(WHEEL_BASE/recode_threshold);
    if(cut_length < 1)cut_length = 1;

    portion3_foint_flag = cut_length;

    for(uint8 i = 1 ; i<=cut_length ; i++ )
    {
        portion_3.recode_map[portion_3.length_index].x = portion_3.recode_map[portion_3.length_index - 1].x ;
        portion_3.recode_map[portion_3.length_index].y = portion_3.recode_map[portion_3.length_index - 1].y - i*recode_threshold;
        portion_3.length_index ++;

        if(portion_3.length_index >=MAX_LENGTH_INDEX)return 0;
    }

    for(int i =cut_length ; i < portion_3.length_index ; i++)
    {
        portion_3.recode_map[i].x -= x_delta;
    }

    float * p = (float *)malloc(sizeof(portion_3.recode_map[0].x)*portion_3.length_index*2);
    for( int i = portion_3.length_index -1 , j = 1 ; i >= cut_length ; i-- , j++)
    {
        p[2*j - 2] =  portion_3.recode_map[i].x;
        p[2*j -1] =  portion_3.recode_map[i].y;
    }
    portion_3.length_index -=cut_length;
    for(int i = 0  , j = 1; i < portion_3.length_index-1 ; i++ , j++)
    {
        portion_3.recode_map[i].x = p[2*j - 2];
        portion_3.recode_map[i].y = p[2*j -1];
    }
    free(p);

    return 1;
}

void Guandao_Points_Show(guandao_state * e)
{
    int choice_flag = 0;

    guandao_state * p = e;
    while(choice_flag < route_setting_choice)
    {
        p = p->next;
        if(p == NULL)return;
        choice_flag++;
    }
    if(p->length_index == 0)return;

    float Max_R_Line = -10000.0f , Max_D_Line = -10000.0f , Min_L_Line = 10000.0f , Min_U_Line = 10000.0f;
    float Center_H = 0 ,  Center_W = 0,  INDEX_H = 0,  INDEX_W = 0 , INDEX_Progress = 0;
    uint16 GD_Show [2][p->length_index] ; uint16 Progress_Show [2][p->length_index] ;
    for( int i = 0 ; i< p->length_index ;i ++)
    {
        if(p->recode_map[i].x < Min_L_Line)Min_L_Line =p->recode_map[i].x;
        if(p->recode_map[i].x > Max_R_Line)Max_R_Line =p->recode_map[i].x;
        if(p->recode_map[i].y < Min_U_Line)Min_U_Line =p->recode_map[i].y;
        if(p->recode_map[i].y > Max_D_Line)Max_D_Line =p->recode_map[i].y;
    }
    Center_W = (Max_R_Line + Min_L_Line)/2.0f;
    Center_H = (Max_D_Line + Min_U_Line)/2.0f;
    INDEX_W = Max_R_Line - Min_L_Line;
    INDEX_H = Max_D_Line - Min_U_Line;

    for(  int i = 0 ; i< p->length_index ; i ++ )
    {
        GD_Show[0][i] = 110.0f + (p->recode_map[i].x - Center_W)*(200.0f/INDEX_W);
        GD_Show[1][i] = 150.0f - (p->recode_map[i].y - Center_H)*(280.0f/INDEX_H);

    }

    INDEX_Progress = 1040.0f/p->length_index;
    for(int i = 0 ; i <p->length_index*300/1040 ; i++){Progress_Show [0][i] = 0;Progress_Show [1][i]= 300 - i*INDEX_Progress; }
    for(int i = p->length_index*300/1040 , j =0 ; i <p->length_index/2 ; i++ , j++){Progress_Show [0][i] = j*INDEX_Progress ;Progress_Show [1][i] = 0; }
    for(int i = p->length_index/2 , j =0 ; i <p->length_index*820/1040 ; i++ , j++){Progress_Show [0][i] = 220 ;Progress_Show [1][i] = j *INDEX_Progress; }
    for(int i = p->length_index*820/1040 , j =0 ; i <p->length_index ; i++ , j++){Progress_Show [0][i] = 220 - j*INDEX_Progress ;Progress_Show [1][i] = 300; }

    for(int i = 0 ; i< p->length_index - 1  ; i ++ )
    {
        ips200_draw_point(GD_Show[0][i],GD_Show[1][i],RGB565_WHITE);
        ips200_draw_line(GD_Show[0][i] ,GD_Show[1][i] ,GD_Show[0][i+1] ,GD_Show[1][i+1] , RGB565_WHITE);
        ips200_draw_line(Progress_Show[0][i] ,Progress_Show[1][i] ,Progress_Show[0][i+1] ,Progress_Show[1][i+1] , RGB565_GREEN);
        system_delay_ms(20);

    }

}

void guandao_show(guandao_state * p)
{

    ips200_show_int(X(1),  Y(0) ,p->length_index, 5);                                              ips200_show_float(X(10),Y(0),p->current_state.theta,3,2);
    ips200_show_float(X(1),  Y(3) ,p->recode_map[INS.length_index-1].x, 5,2);     ips200_show_float(X(10),  Y(3) ,p->recode_map[INS.length_index-1].y, 5,2);
    ips200_show_float(X(1),  Y(4) ,p->current_state.x, 5,2);                                     ips200_show_float(X(10),  Y(4) ,p->current_state.y, 5,2);

}

void follow_points_show(guandao_state * p)
{

    ips200_show_float(X(10),Y(10),p->recode_map[INS.current_point_index].x,3,2); ips200_show_float(X(15),Y(10),p->recode_map[INS.current_point_index].y,3,2);
    ips200_show_float(X(10),Y(11),p->current_state.x,3,2);                                         ips200_show_float(X(15),Y(11),p->current_state.y,3,2);
    ips200_show_int(X(10),  Y(15) ,p->current_point_index, 5);                                   ips200_show_int(X(15),  Y(15) ,p->length_index, 5);
    ips200_show_float(X(15),Y(16),p->current_state.theta,3,2);
}

void Key_Recode_Point(guandao_state * e)
{
    if(e->length_index >=MAX_LENGTH_INDEX) return;
    e->recode_map[e->length_index] =e->current_state;
    e->length_index  ++;

}

//void guandao_mode_recode(void)
//{
//    static uint8 flag0 = 1;
//    static uint8 flag1 = 1;
//    update_state(&INS,&guandao_ecd);
//    if(flag0){  guandao_state_init(&INS);   flag0 =0;}
//
//     recode_waypoint(&INS);
////     guandao_show();
//
//
////    if(gpio_get_level(SWITCH1)&&flag1){   Flash_Write_mappoints();  Buzzer_check(50);  flag1 = 0; };
//
//}

//void guandao_mode_trace(void)
//{
//
////    static uint8 flag2 = 1;

//
//     update_state(&INS,&guandao_ecd);
//     pursuit_contral_mode(&INS ,&out_v_l ,&out_v_r ,&out_servo);
//     follow_points_show();
////        Steer_UpPID(&SteerUpPID ,out_servo);
//      Steer_PID(&SteerPID,out_servo);
//}
