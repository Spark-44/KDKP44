

#include "zf_common_headfile.h"

float speed_pid[6]={0.5f, 1.0f, 0.0f, 0.4f, 0.4f, 3.0f};
int16 control[5] = {10, -10, 2, 0, 0};
float kp;
float ki;
float kd;

#define FLASH_RECODE_THRESHOLD_DEFAULT   (0.4f)
#define FLASH_RECODE_THRESHOLD_MIN       (0.05f)
#define FLASH_RECODE_THRESHOLD_MAX       (2.0f)
#define FLASH_PURSUIT_THRESHOLD_DEFAULT  (0.4f)
#define FLASH_PURSUIT_THRESHOLD_MIN      (0.05f)
#define FLASH_PURSUIT_THRESHOLD_MAX      (3.0f)
#define FLASH_FINAL_DSTS_DEFAULT         (3.0f)
#define FLASH_FINAL_DSTS_MIN             (0.3f)
#define FLASH_FINAL_DSTS_MAX             (20.0f)
#define PORTION2_GPS_LAYOUT_INDEX         (1019)
#define PORTION2_GPS_LAYOUT_MAGIC         (0x50324746U)

static float flash_sanitize_float(float value, float fallback, float min_value, float max_value)
{
    if(!(value >= min_value && value <= max_value))
    {
        return fallback;
    }
    return value;
}

static void flash_sanitize_runtime_params(void)
{
    speed_pid[3] = flash_sanitize_float(speed_pid[3],
                                        FLASH_RECODE_THRESHOLD_DEFAULT,
                                        FLASH_RECODE_THRESHOLD_MIN,
                                        FLASH_RECODE_THRESHOLD_MAX);
    speed_pid[4] = flash_sanitize_float(speed_pid[4],
                                        FLASH_PURSUIT_THRESHOLD_DEFAULT,
                                        FLASH_PURSUIT_THRESHOLD_MIN,
                                        FLASH_PURSUIT_THRESHOLD_MAX);
    speed_pid[5] = flash_sanitize_float(speed_pid[5],
                                        FLASH_FINAL_DSTS_DEFAULT,
                                        FLASH_FINAL_DSTS_MIN,
                                        FLASH_FINAL_DSTS_MAX);
}

void Flash_Read_pid(void)
{
    if(flash_check(FLASH_SECTION_INDEX,SPEED_PID_PAGE_INDEX))
    {
        flash_buffer_clear();
        flash_read_page_to_buffer(FLASH_SECTION_INDEX, SPEED_PID_PAGE_INDEX);
        for(uint8 i = 0  ; i<6 ; i++)
        {
            speed_pid[i] = flash_union_buffer[i].float_type;
        }
        speed_pid[3] = FLASH_RECODE_THRESHOLD_DEFAULT;
        flash_sanitize_runtime_params();
        MoterPID_L.Kp = speed_pid[0];
        MoterPID_R.Kp = speed_pid[0];
        MoterPID_L.Ki = speed_pid[1];
        MoterPID_R.Ki = speed_pid[1];
        MoterPID_L.Kd = speed_pid[2];
        MoterPID_R.Kd = speed_pid[2];
        recode_threshold = speed_pid[3];
        persuit_threshold = speed_pid[4];
        final_dsts = speed_pid[5];
        for(uint8 i = 12  ,j =0; i<16 ; i++ ,j++)
        {
            control[j] = flash_union_buffer[i].int16_type;
        }
        control[0] = 10;
        base_speed = (float)control[0];
        daoche_speed = (float)control[1];
        preview_spets = control[2];
    }

}

void Flash_Write_pid(void)
{
    flash_buffer_clear();
    speed_pid[3] = FLASH_RECODE_THRESHOLD_DEFAULT;
    flash_sanitize_runtime_params();

    MoterPID_L.Kp = speed_pid[0];
    MoterPID_R.Kp = speed_pid[0];
    MoterPID_L.Ki = speed_pid[1];
    MoterPID_R.Ki = speed_pid[1];
    MoterPID_L.Kd = speed_pid[2];
    MoterPID_R.Kd = speed_pid[2];
    recode_threshold = speed_pid[3];
    persuit_threshold = speed_pid[4];
    final_dsts = speed_pid[5];
    control[0] = 10;
    base_speed = (float)control[0];
    daoche_speed = (float)control[1];
    preview_spets = control[2];

    for(uint8 i = 0  ; i<6 ; i++)
    {
        flash_union_buffer[i].float_type = speed_pid[i];
    }
    for(uint8 i = 12 ,j=0 ; i<16 ; i++ ,j++)
    {
        flash_union_buffer[i].int16_type =control[j] ;
    }
      if(flash_check(FLASH_SECTION_INDEX,SPEED_PID_PAGE_INDEX))
      {
          flash_erase_page(FLASH_SECTION_INDEX,SPEED_PID_PAGE_INDEX) ;
      }
      flash_write_page_from_buffer(FLASH_SECTION_INDEX,SPEED_PID_PAGE_INDEX);

}

void Flash_Store_Mode(uint8 route_choice)
{
    switch(route_choice)
    {
        case 1:
            if(portion2_points_build()){ Key_Recode_Point(&passage); Flash_Write_passage_points();}
            break;
        case 2:
            Key_Recode_Point(&portion_3);
            if(portion3_points_switch()){  Flash_Write_portion_3points();}
            break;
        default :break;
    }

}

void Flash_Main_Read(void)
{
    Flash_Read_pid();
    Flash_Read_passage_points();
    Flash_Read_portion_3points();

}

void Flash_Write_passage_points(void)
{
    int max_storage = 2 * passage.length_index +2 ;
    int route_info_index;
    int gps_info_index;
    if (max_storage >=1020) max_storage = 1020;
    flash_buffer_clear();

    flash_union_buffer[0].int16_type = passage.length_index;
    for(int i = 2 , j = 0;i < max_storage ; i += 2 , j++)
    {
        flash_union_buffer[i].float_type = passage.recode_map[j].x;
    }
    for(int i = 3 , j = 0;i < max_storage ; i += 2 , j++)
    {
        flash_union_buffer[i].float_type = passage.recode_map[j].y;
    }

    route_info_index = max_storage;
    if(route_info_index + PORTION2_ROUTE_COUNT * 2 + 1 + PORTION2_TOTAL_GPS_COUNT * 4 < 1020)
    {
        for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
        {
            flash_union_buffer[route_info_index + i].int16_type = portion2_route_length[i];
            flash_union_buffer[route_info_index + PORTION2_ROUTE_COUNT + i].int16_type = portion2_route_gps_count[i];
        }
        flash_union_buffer[route_info_index + PORTION2_ROUTE_COUNT * 2].int16_type = passage.gps_recode_length;
        gps_info_index = route_info_index + PORTION2_ROUTE_COUNT * 2 + 1;
        for(uint8 i = 0; i < PORTION2_TOTAL_GPS_COUNT; i++)
        {
            flash_union_buffer[gps_info_index + i * 4].int32_type = double_to_int32(passage.recode_gpsmap[i].lat);
            flash_union_buffer[gps_info_index + i * 4 + 1].int32_type = double_to_int32(passage.recode_gpsmap[i].lon);
            flash_union_buffer[gps_info_index + i * 4 + 2].float_type = passage.recode_gpsmap[i].theta;
            flash_union_buffer[gps_info_index + i * 4 + 3].int16_type = passage.recode_gpsmap[i].cheak_flag;
        }
        flash_union_buffer[PORTION2_GPS_LAYOUT_INDEX].uint32_type = PORTION2_GPS_LAYOUT_MAGIC;
    }

    if(flash_check(FLASH_SECTION_INDEX,RECODE_PASSAGE))
    {
        flash_erase_page(FLASH_SECTION_INDEX,RECODE_PASSAGE) ;
    }
    flash_write_page_from_buffer(FLASH_SECTION_INDEX,RECODE_PASSAGE);
}

void Flash_Read_passage_points(void)
{
    int get_max_storage = 0;
    int route_info_index;
    int gps_info_index;
    uint8 gps_layout_valid = 0;
    if(flash_check(FLASH_SECTION_INDEX,RECODE_PASSAGE))
    {
        flash_buffer_clear();
        flash_read_page_to_buffer(FLASH_SECTION_INDEX, RECODE_PASSAGE);
        gps_layout_valid = (flash_union_buffer[PORTION2_GPS_LAYOUT_INDEX].uint32_type == PORTION2_GPS_LAYOUT_MAGIC) ? 1 : 0;
        passage.length_index = flash_union_buffer[0].int16_type;
        get_max_storage =2 * passage.length_index +2;
        for(int i = 2 , j = 0;i < get_max_storage ; i += 2 , j++)
        {
            passage.recode_map[j].x = flash_union_buffer[i].float_type;
        }
        for(int i = 3 , j = 0;i < get_max_storage ; i += 2 , j++)
        {
            passage.recode_map[j].y = flash_union_buffer[i].float_type;
        }
        route_info_index = get_max_storage;
        if(passage.length_index == PORTION2_ROUTE_COUNT * PORTION2_ROUTE_MAX_POINTS &&
           route_info_index + PORTION2_ROUTE_COUNT * 2 + 1 + PORTION2_TOTAL_GPS_COUNT * 4 < 1020)
        {
            for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
            {
                portion2_route_length[i] = flash_union_buffer[route_info_index + i].int16_type;
                portion2_route_gps_count[i] = flash_union_buffer[route_info_index + PORTION2_ROUTE_COUNT + i].int16_type;
                if(portion2_route_length[i] > PORTION2_ROUTE_MAX_POINTS) portion2_route_length[i] = 0;
                if(!gps_layout_valid || portion2_route_gps_count[i] > PORTION2_GPS_PER_ROUTE) portion2_route_gps_count[i] = 0;
            }
            passage.gps_recode_length = gps_layout_valid ? flash_union_buffer[route_info_index + PORTION2_ROUTE_COUNT * 2].int16_type : 0;
            gps_info_index = route_info_index + PORTION2_ROUTE_COUNT * 2 + 1;
            for(uint8 i = 0; gps_layout_valid && i < PORTION2_TOTAL_GPS_COUNT; i++)
            {
                passage.recode_gpsmap[i].lat = int32_to_double(flash_union_buffer[gps_info_index + i * 4].int32_type);
                passage.recode_gpsmap[i].lon = int32_to_double(flash_union_buffer[gps_info_index + i * 4 + 1].int32_type);
                passage.recode_gpsmap[i].theta = flash_union_buffer[gps_info_index + i * 4 + 2].float_type;
                passage.recode_gpsmap[i].cheak_flag = flash_union_buffer[gps_info_index + i * 4 + 3].int16_type;
            }
        }
    }
}

void Flash_Write_portion_3points(void)
{
    int max_storage = 2 * portion_3.length_index +2 ;
    if (max_storage >=1020) max_storage = 1020;
    flash_buffer_clear();

    flash_union_buffer[0].int16_type = portion_3.length_index;
    for(int i = 2 , j = 0;i < max_storage ; i += 2 , j++)
    {
        flash_union_buffer[i].float_type = portion_3.recode_map[j].x;
    }
    for(int i = 3 , j = 0;i < max_storage ; i += 2 , j++)
    {
        flash_union_buffer[i].float_type = portion_3.recode_map[j].y;
    }

    if(flash_check(FLASH_SECTION_INDEX,RECODE_PORTION_THREE))
    {
        flash_erase_page(FLASH_SECTION_INDEX,RECODE_PORTION_THREE) ;
    }
    flash_write_page_from_buffer(FLASH_SECTION_INDEX,RECODE_PORTION_THREE);

}

void Flash_Read_portion_3points(void)
{
    int get_max_storage = 0;
    if(flash_check(FLASH_SECTION_INDEX,RECODE_PORTION_THREE))
    {
        flash_buffer_clear();
        flash_read_page_to_buffer(FLASH_SECTION_INDEX, RECODE_PORTION_THREE);
        portion_3.length_index = flash_union_buffer[0].int16_type -1 ;
        get_max_storage =2 * portion_3.length_index +2;
        for(int i = 2 , j = 0;i < get_max_storage ; i += 2 , j++)
        {
            portion_3.recode_map[j].x = flash_union_buffer[i].float_type;
        }
        for(int i = 3 , j = 0;i < get_max_storage ; i += 2 , j++)
        {
            portion_3.recode_map[j].y = flash_union_buffer[i].float_type;
        }
    }
}
