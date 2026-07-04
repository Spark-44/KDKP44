

#include "zf_common_headfile.h"
#include "display.h"
#include "rear_motor/rear_motor.h"
#include "subject_2_gyro_route.h"

guandao_state INS;                               //0 = route_setting_choice
guandao_state passage;                    //1 = route_setting_choice
guandao_state portion_3;                     //2 = route_setting_choice
guandao_state portion_2;                    //3 = route_setting_choice

SLIP_Cheak slip_state = NONE;         

uint8 route_setting_choice = 0;        

float base_speed = 15.0f;
float persuit_threshold = 0.4f;         
float recode_threshold = 0.2f;         
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
uint8 portion2_run_reverse = 0;
uint8 portion2_run_drive_reverse = 0;
uint8 portion2_run_last_rx = 0;
uint16 portion2_run_rx_count = 0;
uint8 portion2_run_reject_reason = 0;
uint16 portion2_route_length[PORTION2_ROUTE_COUNT] = {0};
uint8 portion2_route_gps_count[PORTION2_ROUTE_COUNT] = {0};
float portion2_route_start_yaw[PORTION2_ROUTE_COUNT] = {0};
float portion2_route_final_yaw[PORTION2_ROUTE_COUNT] = {0};
const uint8 portion2_route_required_gps_count[PORTION2_ROUTE_COUNT] = {8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8};
static uint8 portion2_gps_auto_has_point[PORTION2_ROUTE_COUNT] = {0};
static state_t portion2_gps_auto_last_state[PORTION2_ROUTE_COUNT];
static state_t portion2_route_storage_extension[PORTION2_ROUTE_STORAGE_EXTENSION_POINTS];
static GPS_state portion2_gps_storage_extension[PORTION2_GPS_STORAGE_EXTENSION_POINTS];

state_t portion2_route_storage_get(uint16 index)
{
    state_t empty = {0};

    if(index >= PORTION2_TOTAL_ROUTE_POINTS) return empty;
    if(index < MAX_LENGTH_INDEX) return passage.recode_map[index];
    return portion2_route_storage_extension[index - MAX_LENGTH_INDEX];
}

void portion2_route_storage_set(uint16 index, state_t point)
{
    if(index >= PORTION2_TOTAL_ROUTE_POINTS) return;
    if(index < MAX_LENGTH_INDEX)
    {
        passage.recode_map[index] = point;
    }
    else
    {
        portion2_route_storage_extension[index - MAX_LENGTH_INDEX] = point;
    }
}

GPS_state portion2_gps_storage_get(uint16 index)
{
    GPS_state empty = {0};

    if(index >= PORTION2_TOTAL_GPS_COUNT) return empty;
    if(index < MAX_GPS_RECODE) return passage.recode_gpsmap[index];
    return portion2_gps_storage_extension[index - MAX_GPS_RECODE];
}

void portion2_gps_storage_set(uint16 index, GPS_state point)
{
    if(index >= PORTION2_TOTAL_GPS_COUNT) return;
    if(index < MAX_GPS_RECODE)
    {
        passage.recode_gpsmap[index] = point;
    }
    else
    {
        portion2_gps_storage_extension[index - MAX_GPS_RECODE] = point;
    }
}

int16 daoche_point_length = 0;    
uint8 daoche_flag =0;                    
uint8 daoche_flash_cheack =0;
static uint8 portion1_state_flag = 0;
static uint16 portion1_finally_length = 0;
static uint8 portion2_state_flag = 0;
static uint8 portion2_route_saved_flag[PORTION2_ROUTE_COUNT] = {0};
static uint8 portion2_serial_trace_enabled = 0;
static int16 portion2_run_last_report_point = -1;
static uint8 portion2_run_last_report_gps = 255;
static int16 portion2_track_first_bad_raw = -1;
static int16 portion2_track_max_raw = -1;
static float portion2_track_max_off = 0.0f;
static uint8 portion2_track_run_active = 0;
static uint8 portion2_track_summary_emitted = 0;
static uint32 portion2_final_yaw_align_start_ms = 0;
static float portion2_run_final_yaw = 0.0f;
static uint8 portion2_route12_overshoot_armed = 0;
static uint8 portion2_route12_overshoot_count = 0;
static float portion2_route12_min_final_dist = 0.0f;
static uint8 portion2_final_zone_armed = 0;
static uint8 portion2_final_zone_overshoot_count = 0;
static float portion2_final_zone_min_dist = 0.0f;
static state_t portion2_reference_smooth_buffer[MAX_LENGTH_INDEX];
static state_t portion2_raw_route_origin = {0};
static float portion2_raw_route_cos = 1.0f;
static float portion2_raw_route_sin = 0.0f;
static uint32 portion2_record_k1_start_ms = 0;
static uint32 portion2_record_k2_start_ms = 0;
static uint32 portion2_record_k3_start_ms = 0;
static uint32 portion2_record_k4_start_ms = 0;
static uint8 portion2_record_k1_wait_release = 0;
static uint8 portion2_record_k2_wait_release = 0;
static uint8 portion2_record_k3_wait_release = 0;
static uint8 portion2_record_k4_wait_release = 0;
static uint32 portion2_mode_k4_start_ms = 0;
static uint8 portion2_mode_k4_wait_release = 0;
static uint8 portion2_record_start_pending = 0;

#define PORTION2_GPS_ORIGIN_SAMPLE_COUNT 5U
#define PORTION2_GPS_FILTER_SAMPLE_COUNT 3U
#define PORTION2_GPS_ORIGIN_STABILITY_M  (1.0f)
#define PORTION2_GPS_FILTER_STALE_MS     1500U

typedef struct
{
    double latitude[PORTION2_GPS_ORIGIN_SAMPLE_COUNT];
    double longitude[PORTION2_GPS_ORIGIN_SAMPLE_COUNT];
    uint32 last_rmc_sequence;
    uint32 last_sample_ms;
    uint8 count;
    uint8 next;
} portion2_gps_record_filter_t;

static portion2_gps_record_filter_t portion2_gps_record_filter;
static uint32 portion2_gps_last_stored_rmc_sequence = 0;

typedef enum
{
    PORTION2_GPS_REJECT_NONE = 0,
    PORTION2_GPS_REJECT_NO_FIX,
    PORTION2_GPS_REJECT_LOW_SAT,
    PORTION2_GPS_REJECT_BAD_HDOP,
    PORTION2_GPS_REJECT_ZERO_COORD,
    PORTION2_GPS_REJECT_INTERVAL,
    PORTION2_GPS_REJECT_REPEAT,
    PORTION2_GPS_REJECT_JUMP,
    PORTION2_GPS_REJECT_CAPACITY,
    PORTION2_GPS_REJECT_INDEX,
    PORTION2_GPS_REJECT_NO_RMC,
    PORTION2_GPS_REJECT_STABILIZE,
    PORTION2_GPS_REJECT_STALE,
} portion2_gps_reject_reason_t;

static portion2_gps_reject_reason_t portion2_gps_reject_reason = PORTION2_GPS_REJECT_NONE;
static uint32 portion2_gps_reject_last_log_ms = 0;

#define GUANDAO_START_SEARCH_POINTS    10
#define GUANDAO_TRACE_SEARCH_POINTS    8
#define GUANDAO_FRONT_TARGET_ANGLE     100.0f
#define GUANDAO_STEERING_GAIN          2.2f
#define GUANDAO_STEERING_CMD_LIMIT     25.0f
#define GUANDAO_CURVE_TRIGGER_ANGLE    35.0f
#define GUANDAO_LARGE_CURVE_SPEED      11.0f
#define GUANDAO_SHARP_TURN_SPEED       8.0f
#define GUANDAO_HIGH_SPEED_THRESHOLD   5.0f
#define GUANDAO_HIGH_SPEED_GAIN        1.55f
#define GUANDAO_HIGH_SPEED_CMD_LIMIT   25.0f
#define GUANDAO_VERY_HIGH_SPEED_THRESHOLD 15.0f
#define GUANDAO_VERY_HIGH_SPEED_GAIN   1.20f
#define GUANDAO_VERY_HIGH_CMD_LIMIT    25.0f
#define GUANDAO_SHARP_TURN_ANGLE       45.0f
#define GUANDAO_STEER_RATE_LOW         3.0f
#define GUANDAO_STEER_RATE_HIGH        1.5f
#define PORTION2_STEERING_GAIN         0.90f
#define PORTION2_STEERING_CMD_LIMIT    12.0f
#define PORTION2_SHARP_STEERING_CMD_LIMIT 20.0f
#define PORTION2_ROUTE11_STEERING_GAIN 0.70f
#define PORTION2_ROUTE11_STEERING_CMD_LIMIT 18.0f
#define PORTION2_ROUTE11_SHARP_STEERING_CMD_LIMIT 25.0f
#define PORTION2_ROUTE11_MIN_PREVIEW_STEPS 20
#define PORTION2_ROUTE11_CURVE_PREVIEW_STEPS 32
#define PORTION2_ROUTE11_SPEED         6.0f
#define PORTION2_SNAKE_STEERING_CMD_LIMIT 20.0f
#define PORTION2_SNAKE_SHARP_STEERING_CMD_LIMIT 30.0f
#define PORTION2_SHARP_TURN_TRIGGER_DEG 4.0f
#define PORTION2_SHARP_TURN_RAW_LOOKAHEAD 6
#define PORTION2_STEER_RATE_LIMIT      0.5f
#define PORTION2_MIN_PREVIEW_STEPS     14
#define PORTION2_CURVE_PREVIEW_STEPS   24
#define PORTION2_REFERENCE_SMOOTH_PASSES 4
#define PORTION2_REFERENCE_SMOOTH_WEIGHT 0.35f
#define PORTION2_AUTO_GPS_RECORD_DIST  1.0f
#define PORTION2_GPS_RECORD_MIN_MOVE_M  0.20f
#define PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M 0.8f
#define PORTION2_GPS_RECORD_MIN_SATELLITES 6U
#define PORTION2_GPS_MAX_HDOP            (2.5f)
#define PORTION2_GPS_END_MAX_RAW_GAP    4
// Alias to align naming with example project
#define GUANDAO_AUTO_GPS_RECORD_DIST   PORTION2_AUTO_GPS_RECORD_DIST
#define PORTION1_PARK_INDEX_WINDOW     40
#define PORTION1_PARK_ENTER_DISTANCE   2.0f
#define PORTION1_PARK_CRAWL_DISTANCE   0.8f
#define PORTION1_PARK_STOP_DISTANCE    0.35f
#define PORTION1_PARK_APPROACH_SPEED   5.0f
#define PORTION1_PARK_MIN_SPEED        2.0f
#define PORTION1_PARK_CRAWL_SPEED      3.0f
#define PORTION1_END_MIN_SPEED         3.0f
#define PORTION3_PURSUIT_THRESHOLD     0.08f
#define PORTION3_FINAL_STOP_DIST       0.1f
#define PORTION2_FINAL_RAW_POINT_STOP_DIST 0.20f
#define PORTION2_SNAKE_FINAL_STOP_DIST 0.40f
#define PORTION2_SNAKE_OVERSHOOT_ARM_DIST 0.80f
#define PORTION2_SNAKE_OVERSHOOT_RISE_DIST 0.15f
#define PORTION2_SNAKE_OVERSHOOT_CONFIRM_CYCLES 3U
#define PORTION2_FINAL_OVERSHOOT_ARM_DIST 1.00f
#define PORTION2_FINAL_OVERSHOOT_RISE_DIST 0.15f
#define PORTION2_FINAL_OVERSHOOT_CONFIRM_CYCLES 3U
#define PORTION2_FINAL_YAW_TOLERANCE_DEG 5.0f
#define PORTION2_FINAL_YAW_ALIGN_SPEED 4.0f
#define PORTION2_FINAL_YAW_ALIGN_STEER_DEG 15.0f
#define PORTION2_FINAL_YAW_ALIGN_TIMEOUT_MS 4000U
#define PORTION2_FINAL_YAW_ALIGN_MAX_DIST 0.60f
#define PORTION2_TERMINAL_POSE_LENGTH_M 1.5f
#define PORTION2_RAW_TERMINAL_LENGTH_M 2.0f
#define PORTION2_TERMINAL_APPROACH_SPEED 6.0f
#define PORTION2_GUIDED_ROUTE_COUNT 10U
#define PORTION2_GUIDED_TERMINAL_BLEND_START_M 2.0f
#define PORTION2_GUIDED_TERMINAL_BLEND_FULL_M 1.0f
#define PORTION2_GUIDED_FINAL_STOP_DIST 0.35f
#define PORTION2_GUIDED_OVERSHOOT_ARM_DIST 1.50f
#define PORTION2_GUIDED_FINAL_YAW_GAIN 1.0f
#define PORTION2_GUIDED_FINAL_YAW_STEER_LIMIT 12.0f
#define PORTION2_GUIDED_YAW_SLOW_DIST_M 3.0f
#define PORTION2_GUIDED_YAW_SLOW_TRIGGER_DEG 20.0f
#define PORTION2_GUIDED_YAW_SLOW_SPEED 8.0f
#define PORTION2_TRACK_BAD_THRESHOLD_M 0.30f
#define PORTION2_TRACK_CENTER_EPSILON_M 0.01f

#define PORTION2_FINAL_YAW_ALIGN_RUNNING   0U
#define PORTION2_FINAL_YAW_ALIGN_DONE      1U
#define PORTION2_FINAL_YAW_ALIGN_REACQUIRE 2U

static int16 guandao_clamp_length(int length);
static int16 guandao_route_length(guandao_state *state);
static state_t guandao_route_point(guandao_state *state, int index);
static float guandao_normalize_angle(float angle);
static float guandao_segment_yaw(state_t from, state_t to);
static uint8 portion2_route_required_gps(uint8 route_id);
static uint16 portion2_route_offset(uint8 route_id);
static uint16 portion2_route_gps_offset(uint8 route_id);
static uint8 portion2_route_uses_gps(uint8 route_id);
static uint8 portion2_route_uses_reverse_drive(uint8 route_id);

typedef struct
{
    float signed_error;
    float abs_error;
    float heading_error;
} portion2_track_sample_t;

static int16 portion2_plan_index_from_raw_point(int16 raw_index, int16 raw_length, int16 plan_length)
{
    if(raw_length <= 1 || plan_length <= 1) return 0;
    if(raw_index <= 0) return 0;
    if(raw_index >= raw_length) raw_index = raw_length - 1;
    return (int16)(((int32)raw_index * (int32)(plan_length - 1) + (raw_length - 1) / 2) / (raw_length - 1));
}

static int16 portion2_human_point_number(int16 point_index, int16 total)
{
    if(total <= 0) return 0;
    if(point_index < 0) return 1;
    if(point_index >= total) return total;
    return point_index + 1;
}

static int16 portion2_raw_point_from_plan_index(int16 plan_index)
{
    int16 raw_length = guandao_clamp_length(portion_2.length_index);
    int16 plan_length = guandao_route_length(&portion_2);

    if(raw_length <= 1 || plan_length <= 1) return 0;
    if(plan_index <= 0) return 0;
    if(plan_index >= plan_length) return raw_length - 1;
    return (int16)(((int32)plan_index * (int32)(raw_length - 1) + (plan_length - 1) / 2) / (plan_length - 1));
}

static long portion2_serial_fixed100(float value)
{
    if(value >= 0.0f)
    {
        return (long)(value * 100.0f + 0.5f);
    }
    return (long)(value * 100.0f - 0.5f);
}

static void portion2_serial_append_fixed100(char *line, int *pos, int size, float value)
{
    long scaled = portion2_serial_fixed100(value);
    long whole;
    long frac;

    if(scaled < 0)
    {
        line[(*pos)++] = '-';
        scaled = -scaled;
    }

    whole = scaled / 100;
    frac = scaled % 100;
    *pos += sprintf(&line[*pos], "%ld.%02ld", whole, frac);
    if(*pos >= size) *pos = size - 1;
}

static void portion2_serial_write_state_point(const char *prefix, uint8 route_id, uint16 point_index, state_t point)
{
    char line[160];
    int pos = 0;

    pos += sprintf(line, "%s route=%u pt=%u x=", prefix, (unsigned)(route_id + 1), (unsigned)point_index);
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), point.x);
    pos += sprintf(&line[pos], " y=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), point.y);
    pos += sprintf(&line[pos], " theta=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), point.theta);
    pos += sprintf(&line[pos], " gps=%u/%u\r\n",
                   (unsigned)portion2_route_gps_count[route_id],
                   (unsigned)PORTION2_GPS_PER_ROUTE);
    if(pos > 0)
    {
        uart_write_string(DEBUG_UART_INDEX, line);
    }
}

static void portion2_serial_write_gps_point(const char *prefix, uint8 route_id, uint8 point_index, GPS_state point)
{
    char line[176];
    int pos = 0;
    int32 lat7 = double_to_int32(point.lat);
    int32 lon7 = double_to_int32(point.lon);

    pos += sprintf(line,
                   "%s route=%u gps=%u lat7=%ld lon7=%ld yaw=",
                   prefix,
                   (unsigned)(route_id + 1),
                   (unsigned)point_index,
                   (long)lat7,
                   (long)lon7);
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), point.theta);
    pos += sprintf(&line[pos], " bind_pt=%d gps=%u/%u\r\n",
                   (int)point.cheak_flag,
                   (unsigned)portion2_route_gps_count[route_id],
                   (unsigned)PORTION2_GPS_PER_ROUTE);
    if(pos > 0)
    {
        uart_write_string(DEBUG_UART_INDEX, line);
    }
}

static void portion2_serial_log_record_event(const char *event)
{
    char line[128];

    sprintf(line,
            "[P2-REC-%s] route=%u state=%u pts=%u/%u gps=%u/%u saved=%s\r\n",
            event,
            (unsigned)(portion2_record_route + 1),
            (unsigned)portion2_record_state,
            (unsigned)portion2_route_length[portion2_record_route],
            (unsigned)PORTION2_ROUTE_MAX_POINTS,
            (unsigned)portion2_route_gps_count[portion2_record_route],
            (unsigned)PORTION2_GPS_PER_ROUTE,
            portion2_route_saved_flag[portion2_record_route] ? "YES" : "NO");
    uart_write_string(DEBUG_UART_INDEX, line);
}

static uint8 portion2_run_gps_reached_count(void)
{
    uint8 count = 0;
    int16 run_index = guandao_clamp_length(portion_2.current_point_index);
    int16 route_len = guandao_route_length(&portion_2);

    if(run_index >= route_len)
    {
        return (uint8)portion_2.gps_recode_length;
    }

    for(uint8 i = 0; i < portion_2.gps_recode_length && i < MAX_GPS_RECODE; i++)
    {
        if(run_index >= portion_2.recode_gpsmap[i].cheak_flag)
        {
            count++;
        }
    }

    return count;
}

static portion2_track_sample_t portion2_track_sample_segment(state_t segment_a, state_t segment_b)
{
    portion2_track_sample_t sample = {0.0f, 0.0f, 0.0f};
    float segment_dx;
    float segment_dy;
    float segment_length_sq;
    float offset_x;
    float offset_y;
    float dot;
    float projection;
    float projected_x;
    float projected_y;
    float cross;
    float track_heading;

    segment_dx = segment_b.x - segment_a.x;
    segment_dy = segment_b.y - segment_a.y;
    segment_length_sq = segment_dx * segment_dx + segment_dy * segment_dy;
    if(segment_length_sq <= 0.000001f) return sample;

    offset_x = portion_2.current_state.x - segment_a.x;
    offset_y = portion_2.current_state.y - segment_a.y;
    dot = offset_x * segment_dx + offset_y * segment_dy;
    projection = dot / segment_length_sq;
    if(projection < 0.0f) projection = 0.0f;
    if(projection > 1.0f) projection = 1.0f;
    projected_x = segment_a.x + projection * segment_dx;
    projected_y = segment_a.y + projection * segment_dy;
    sample.abs_error = hypotf(portion_2.current_state.x - projected_x,
                              portion_2.current_state.y - projected_y);
    cross = segment_dx * offset_y - segment_dy * offset_x;
    sample.signed_error = (cross < 0.0f) ? -sample.abs_error : sample.abs_error;
    track_heading = portion2_run_drive_reverse ? Yaw_1 + 180.0f : Yaw_1;
    sample.heading_error = guandao_normalize_angle(guandao_segment_yaw(segment_a, segment_b) - track_heading);
    return sample;
}

static portion2_track_sample_t portion2_track_sample(int16 run_index)
{
    int16 route_len = guandao_route_length(&portion_2);
    int16 segment_start;

    if(route_len < 2)
    {
        portion2_track_sample_t empty = {0.0f, 0.0f, 0.0f};
        return empty;
    }
    if(run_index < 0) run_index = 0;
    if(run_index >= route_len) run_index = route_len - 1;
    segment_start = (run_index > 0) ? run_index - 1 : 0;
    if(segment_start >= route_len - 1) segment_start = route_len - 2;

    return portion2_track_sample_segment(guandao_route_point(&portion_2, segment_start),
                                         guandao_route_point(&portion_2, segment_start + 1));
}

static state_t portion2_aligned_raw_route_point(int16 raw_index)
{
    uint16 length = portion2_route_length[portion2_selected_route];
    uint16 offset = portion2_route_offset(portion2_selected_route);
    uint16 source_index;
    state_t point;
    float rel_x;
    float rel_y;

    if(length == 0)
    {
        state_t empty = {0};
        return empty;
    }
    if(length > PORTION2_ROUTE_MAX_POINTS) length = PORTION2_ROUTE_MAX_POINTS;
    if(raw_index < 0) raw_index = 0;
    if(raw_index >= (int16)length) raw_index = (int16)length - 1;
    source_index = portion2_run_reverse
            ? (uint16)(offset + length - 1U - (uint16)raw_index)
            : (uint16)(offset + (uint16)raw_index);
    point = portion2_route_storage_get(source_index);
    rel_x = point.x - portion2_raw_route_origin.x;
    rel_y = point.y - portion2_raw_route_origin.y;
    point.x = rel_x * portion2_raw_route_cos + rel_y * portion2_raw_route_sin;
    point.y = rel_y * portion2_raw_route_cos - rel_x * portion2_raw_route_sin;
    return point;
}

static portion2_track_sample_t portion2_raw_track_sample(int16 run_index)
{
    int16 raw_length = guandao_clamp_length(portion_2.length_index);
    int16 raw_point;
    int16 segment_start;

    if(raw_length < 2)
    {
        portion2_track_sample_t empty = {0.0f, 0.0f, 0.0f};
        return empty;
    }
    raw_point = portion2_raw_point_from_plan_index(run_index);
    if(raw_point < 0) raw_point = 0;
    if(raw_point >= raw_length) raw_point = raw_length - 1;
    segment_start = (raw_point > 0) ? raw_point - 1 : 0;
    if(segment_start >= raw_length - 1) segment_start = raw_length - 2;

    return portion2_track_sample_segment(portion2_aligned_raw_route_point(segment_start),
                                         portion2_aligned_raw_route_point(segment_start + 1));
}

static void portion2_track_reset(void)
{
    portion2_run_last_report_point = -1;
    portion2_track_first_bad_raw = -1;
    portion2_track_max_raw = -1;
    portion2_track_max_off = 0.0f;
    portion2_track_run_active = 1;
    portion2_track_summary_emitted = 0;
}

static void portion2_serial_log_track_summary(void)
{
    char line[128];
    int pos;

    if(!portion2_track_run_active || portion2_track_summary_emitted) return;
    pos = sprintf(line,
                  "[P2-TRACK-END] route=%u first_bad=%d max_raw=%d max_off=",
                  (unsigned)(portion2_selected_route + 1),
                  (portion2_track_first_bad_raw >= 0) ? (int)(portion2_track_first_bad_raw + 1) : 0,
                  (portion2_track_max_raw >= 0) ? (int)(portion2_track_max_raw + 1) : 0);
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_track_max_off);
    pos += sprintf(&line[pos], "\r\n");
    uart_write_string(DEBUG_UART_INDEX, line);
    portion2_track_summary_emitted = 1;
    portion2_track_run_active = 0;
}

static void portion2_serial_log_run_point_event(uint8 force)
{
    int16 route_len = guandao_route_length(&portion_2);
    int16 run_index = guandao_clamp_length(portion_2.current_point_index);
    char line[320];
    int pos = 0;
    state_t final_point;
    portion2_track_sample_t plan_sample;
    portion2_track_sample_t raw_sample;
    const char *side;
    const char *status;
    int16 raw_point;
    int16 raw_length;
    int16 raw_number;
    int16 plan_number;
    float final_dist;

    if(route_len <= 0) return;
    if(run_index > route_len) run_index = route_len;
    raw_point = portion2_raw_point_from_plan_index(run_index);
    raw_length = guandao_clamp_length(portion_2.length_index);

    if(run_index >= route_len)
    {
        state_t final_point = guandao_route_point(&portion_2, route_len - 1);
        if(portion2_state_flag != 0)
        {
            final_point.theta = portion2_run_final_yaw;
        }
        float yaw_error = guandao_normalize_angle(final_point.theta - Yaw_1);
        pos += sprintf(line,
                       "[P2-RUN-END] route=%u idx=%d/%d raw_pt=%d/%d gps=%u/%d reason=%u yaw=",
                       (unsigned)(portion2_selected_route + 1),
                       (int)run_index,
                       (int)route_len,
                       (int)raw_point,
                       (int)raw_length,
                       (unsigned)portion2_run_gps_reached_count(),
                       (int)portion_2.gps_recode_length,
                       (unsigned)guandao_debug_stop_reason);
        portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), Yaw_1);
        pos += sprintf(&line[pos], " final_yaw=");
        portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), final_point.theta);
        pos += sprintf(&line[pos], " yaw_err=");
        portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), yaw_error);
        pos += sprintf(&line[pos], "\r\n");
        uart_write_string(DEBUG_UART_INDEX, line);
        portion2_serial_log_track_summary();
        return;
    }

    if(!force && raw_point == portion2_run_last_report_point) return;
    portion2_run_last_report_point = raw_point;
    plan_sample = portion2_track_sample(run_index);
    raw_sample = portion2_raw_track_sample(run_index);
    if(portion2_track_max_raw < 0 || raw_sample.abs_error > portion2_track_max_off)
    {
        portion2_track_max_off = raw_sample.abs_error;
        portion2_track_max_raw = raw_point;
    }
    if(portion2_track_first_bad_raw < 0 && raw_sample.abs_error >= PORTION2_TRACK_BAD_THRESHOLD_M)
    {
        portion2_track_first_bad_raw = raw_point;
    }
    if(raw_sample.signed_error > PORTION2_TRACK_CENTER_EPSILON_M) side = "LEFT";
    else if(raw_sample.signed_error < -PORTION2_TRACK_CENTER_EPSILON_M) side = "RIGHT";
    else side = "CENTER";
    status = (raw_sample.abs_error >= PORTION2_TRACK_BAD_THRESHOLD_M) ? "BAD" : "OK";
    raw_number = portion2_human_point_number(raw_point, raw_length);
    plan_number = portion2_human_point_number(run_index, route_len);
    final_point = guandao_route_point(&portion_2, route_len - 1);
    final_dist = get_distance(portion_2.current_state, final_point);
    pos += sprintf(line,
                   "[P2-TRACK] route=%u raw=%d/%d plan=%d/%d side=%s off=",
                   (unsigned)(portion2_selected_route + 1),
                   (int)raw_number,
                   (int)raw_length,
                   (int)plan_number,
                   (int)route_len,
                   side);
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), raw_sample.abs_error);
    pos += sprintf(&line[pos], " off_raw=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), raw_sample.abs_error);
    pos += sprintf(&line[pos], " off_plan=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), plan_sample.abs_error);
    pos += sprintf(&line[pos], " head_err=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), raw_sample.heading_error);
    pos += sprintf(&line[pos], " steer=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), out_servo);
    pos += sprintf(&line[pos], " final=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), final_dist);
    pos += sprintf(&line[pos], " gps=%s gps_err=", portion2_gps_fusion_is_ready() ? "ON" : "OFF");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_gps_fusion_get_error());
    pos += sprintf(&line[pos], " status=%s\r\n", status);
    uart_write_string(DEBUG_UART_INDEX, line);
}

static void portion2_serial_log_run_gps_event(uint8 force)
{
    uint8 gps_done = portion2_run_gps_reached_count();
    char line[160];

    if(!force && gps_done == portion2_run_last_report_gps) return;
    portion2_run_last_report_gps = gps_done;

    sprintf(line,
            "[P2-RUN-GPS] route=%u gps=%u/%d idx=%d/%d raw_pt=%d/%d\r\n",
            (unsigned)(portion2_selected_route + 1),
            (unsigned)gps_done,
            (int)portion_2.gps_recode_length,
            (int)portion_2.current_point_index,
            (int)guandao_route_length(&portion_2),
            (int)portion2_raw_point_from_plan_index(guandao_clamp_length(portion_2.current_point_index)),
            (int)guandao_clamp_length(portion_2.length_index));
    uart_write_string(DEBUG_UART_INDEX, line);
}

static void portion2_serial_log_run(void)
{
    static uint32 last_ms = 0;
    uint32 now_ms;
    uint16 route_len;
    int16 run_index;
    int16 raw_point;
    int16 raw_length;
    state_t target_point;
    state_t final_point;
    float final_dist;
    int16 raw_number;
    int16 plan_number;
    uint8 gps_done;
    char line[400];
    int pos = 0;

    now_ms = system_getval_ms();
    if((uint32)(now_ms - last_ms) < 1000U) return;
    last_ms = now_ms;

    route_len = (uint16)guandao_route_length(&portion_2);
    run_index = guandao_clamp_length(portion_2.current_point_index);
    raw_point = portion2_raw_point_from_plan_index(run_index);
    raw_length = guandao_clamp_length(portion_2.length_index);
    raw_number = portion2_human_point_number(raw_point, raw_length);
    plan_number = portion2_human_point_number(run_index, (int16)route_len);
    gps_done = portion2_run_gps_reached_count();
    target_point = guandao_route_point(&portion_2, run_index);
    final_point = guandao_route_point(&portion_2, route_len > 0 ? (int)route_len - 1 : 0);
    final_dist = get_distance(portion_2.current_state, final_point);
    pos += sprintf(line,
                   "[P2-RUN] route=%u state=%u idx=%d/%u raw_pt=%d/%d gps=%u/%d raw_no=%d/%d plan_no=%d/%d gps_no=%u/%d x=",
                   (unsigned)(portion2_selected_route + 1),
                   (unsigned)portion2_state_flag,
                   portion_2.current_point_index,
                   (unsigned)route_len,
                   (int)raw_point,
                   (int)raw_length,
                   (unsigned)gps_done,
                   (int)portion_2.gps_recode_length,
                   (int)raw_number,
                   (int)raw_length,
                   (int)plan_number,
                   (int)route_len,
                   (unsigned)gps_done,
                   (int)portion_2.gps_recode_length);
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion_2.current_state.x);
    pos += sprintf(&line[pos], " y=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion_2.current_state.y);
    pos += sprintf(&line[pos], " yaw=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), Yaw_1);
    pos += sprintf(&line[pos], " target_x=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), target_point.x);
    pos += sprintf(&line[pos], " target_y=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), target_point.y);
    pos += sprintf(&line[pos], " vl=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), out_v_l);
    pos += sprintf(&line[pos], " vr=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), out_v_r);
    pos += sprintf(&line[pos], " servo=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), out_servo);
    pos += sprintf(&line[pos], " dist=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), guandao_debug_distance);
    pos += sprintf(&line[pos], " final_dist=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), final_dist);
    pos += sprintf(&line[pos], " reason=%u rev=%u\r\n",
                   (unsigned)guandao_debug_stop_reason,
                   (unsigned)(portion2_run_drive_reverse ? 2 : portion2_run_reverse));
    if(pos > 0)
    {
        uart_write_string(DEBUG_UART_INDEX, line);
    }
}

static int16 guandao_clamp_length(int length)
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

static float guandao_normalize_angle(float angle)
{
    while(angle > 180.0f) angle -= 360.0f;
    while(angle < -180.0f) angle += 360.0f;
    return angle;
}

static float portion2_guided_terminal_steering(float path_steering, float dist_to_final,
        float final_yaw, float current_yaw)
{
    float blend;
    float yaw_error;
    float yaw_steering;

    if(portion2_selected_route >= PORTION2_GUIDED_ROUTE_COUNT
            || dist_to_final >= PORTION2_GUIDED_TERMINAL_BLEND_START_M)
    {
        return path_steering;
    }

    blend = (PORTION2_GUIDED_TERMINAL_BLEND_START_M - dist_to_final)
            / (PORTION2_GUIDED_TERMINAL_BLEND_START_M - PORTION2_GUIDED_TERMINAL_BLEND_FULL_M);
    if(blend < 0.0f) blend = 0.0f;
    if(blend > 1.0f) blend = 1.0f;

    yaw_error = guandao_normalize_angle(final_yaw - current_yaw);
    yaw_steering = -yaw_error * PORTION2_GUIDED_FINAL_YAW_GAIN;
    Value_Limit_float(&yaw_steering,
            -PORTION2_GUIDED_FINAL_YAW_STEER_LIMIT,
            PORTION2_GUIDED_FINAL_YAW_STEER_LIMIT);
    return path_steering * (1.0f - blend) + yaw_steering * blend;
}

static void portion2_route12_overshoot_reset(void)
{
    portion2_route12_overshoot_armed = 0;
    portion2_route12_overshoot_count = 0;
    portion2_route12_min_final_dist = 0.0f;
}

static uint8 portion2_route12_overshoot_detect(int16 raw_point, int16 raw_length, float dist_to_final)
{
    if(portion2_selected_route != PORTION2_ROUTE_SNAKE)
    {
        portion2_route12_overshoot_reset();
        return 0;
    }
    if(raw_length <= 0 || raw_point < raw_length - 1) return 0;

    if(!portion2_route12_overshoot_armed)
    {
        if(dist_to_final <= PORTION2_SNAKE_OVERSHOOT_ARM_DIST)
        {
            portion2_route12_overshoot_armed = 1;
            portion2_route12_min_final_dist = dist_to_final;
        }
        return 0;
    }

    if(dist_to_final < portion2_route12_min_final_dist)
    {
        portion2_route12_min_final_dist = dist_to_final;
        portion2_route12_overshoot_count = 0;
        return 0;
    }
    if(dist_to_final >= portion2_route12_min_final_dist + PORTION2_SNAKE_OVERSHOOT_RISE_DIST)
    {
        if(portion2_route12_overshoot_count < PORTION2_SNAKE_OVERSHOOT_CONFIRM_CYCLES)
        {
            portion2_route12_overshoot_count++;
        }
        return (portion2_route12_overshoot_count >= PORTION2_SNAKE_OVERSHOOT_CONFIRM_CYCLES) ? 1U : 0U;
    }

    portion2_route12_overshoot_count = 0;
    return 0;
}

static void portion2_final_zone_reset(void)
{
    portion2_final_zone_armed = 0;
    portion2_final_zone_overshoot_count = 0;
    portion2_final_zone_min_dist = 0.0f;
}

static uint8 portion2_final_zone_overshoot_detect(int16 raw_point, int16 raw_length, float dist_to_final)
{
    float arm_dist;

    if(portion2_selected_route == PORTION2_ROUTE_SNAKE)
    {
        portion2_final_zone_reset();
        return 0;
    }
    if(raw_length <= 0 || raw_point < raw_length - 1) return 0;
    arm_dist = (portion2_selected_route < PORTION2_GUIDED_ROUTE_COUNT)
            ? PORTION2_GUIDED_OVERSHOOT_ARM_DIST : PORTION2_FINAL_OVERSHOOT_ARM_DIST;

    if(!portion2_final_zone_armed)
    {
        if(dist_to_final <= arm_dist)
        {
            portion2_final_zone_armed = 1;
            portion2_final_zone_min_dist = dist_to_final;
        }
        return 0;
    }

    if(dist_to_final < portion2_final_zone_min_dist)
    {
        portion2_final_zone_min_dist = dist_to_final;
        portion2_final_zone_overshoot_count = 0;
        return 0;
    }
    if(dist_to_final >= portion2_final_zone_min_dist + PORTION2_FINAL_OVERSHOOT_RISE_DIST)
    {
        if(portion2_final_zone_overshoot_count < PORTION2_FINAL_OVERSHOOT_CONFIRM_CYCLES)
        {
            portion2_final_zone_overshoot_count++;
        }
        return (portion2_final_zone_overshoot_count >= PORTION2_FINAL_OVERSHOOT_CONFIRM_CYCLES) ? 1U : 0U;
    }

    portion2_final_zone_overshoot_count = 0;
    return 0;
}

static uint8 portion2_final_yaw_align(state_t final_point, float dist_to_final, float *out_v_l, float *out_v_r, float *out_servo)
{
    uint32 now_ms;
    float yaw_error = guandao_normalize_angle(final_point.theta - Yaw_1);
    float max_align_dist = (portion2_selected_route == PORTION2_ROUTE_SNAKE)
            ? PORTION2_SNAKE_FINAL_STOP_DIST : PORTION2_FINAL_YAW_ALIGN_MAX_DIST;

    guandao_debug_angle_diff = yaw_error;
    if(fabsf(yaw_error) <= PORTION2_FINAL_YAW_TOLERANCE_DEG)
    {
        portion2_final_yaw_align_start_ms = 0;
        return PORTION2_FINAL_YAW_ALIGN_DONE;
    }

    now_ms = system_getval_ms();
    if(portion2_final_yaw_align_start_ms == 0)
    {
        portion2_final_yaw_align_start_ms = now_ms;
    }
    if(dist_to_final > max_align_dist)
    {
        portion2_final_yaw_align_start_ms = 0;
        guandao_debug_stop_reason = 11;
        return PORTION2_FINAL_YAW_ALIGN_REACQUIRE;
    }
    if((uint32)(now_ms - portion2_final_yaw_align_start_ms) >= PORTION2_FINAL_YAW_ALIGN_TIMEOUT_MS)
    {
        portion2_final_yaw_align_start_ms = 0;
        guandao_debug_stop_reason = 10;
        return PORTION2_FINAL_YAW_ALIGN_DONE;
    }

    guandao_debug_stop_reason = 9;
    *out_v_l = PORTION2_FINAL_YAW_ALIGN_SPEED;
    *out_v_r = PORTION2_FINAL_YAW_ALIGN_SPEED;
    *out_servo = (yaw_error > 0.0f) ? -PORTION2_FINAL_YAW_ALIGN_STEER_DEG : PORTION2_FINAL_YAW_ALIGN_STEER_DEG;
    return PORTION2_FINAL_YAW_ALIGN_RUNNING;
}

static float portion2_align_route_to_current_yaw(float run_start_theta)
{
    int16 length = guandao_clamp_length(portion_2.length_index);
    state_t start_point;
    float route_start_yaw;
    float yaw_delta;
    float yaw_rad;
    float cos_yaw;
    float sin_yaw;

    if(length <= 0) return 0.0f;

    angle_plan(&run_start_theta);
    start_point = portion_2.recode_map[0];
    route_start_yaw = start_point.theta;
    if(length > 1)
    {
        route_start_yaw = guandao_segment_yaw(portion_2.recode_map[0], portion_2.recode_map[1]);
    }
    yaw_delta = guandao_normalize_angle(run_start_theta - route_start_yaw);
    yaw_rad = yaw_delta / 180.0f * M_PI;
    cos_yaw = cosf(yaw_rad);
    sin_yaw = sinf(yaw_rad);

    for(int16 i = 0; i < length; i++)
    {
        float rel_x = portion_2.recode_map[i].x - start_point.x;
        float rel_y = portion_2.recode_map[i].y - start_point.y;

        portion_2.recode_map[i].x = rel_x * cos_yaw + rel_y * sin_yaw;
        portion_2.recode_map[i].y = rel_y * cos_yaw - rel_x * sin_yaw;
    }

    for(int16 i = 0; i < length - 1; i++)
    {
        portion_2.recode_map[i].theta = guandao_segment_yaw(portion_2.recode_map[i], portion_2.recode_map[i + 1]);
    }
    if(length > 1)
    {
        portion_2.recode_map[length - 1].theta = guandao_segment_yaw(portion_2.recode_map[length - 2], portion_2.recode_map[length - 1]);
    }
    else
    {
        portion_2.recode_map[0].theta = run_start_theta;
    }

    for(uint8 i = 0; i < portion_2.gps_recode_length && i < MAX_GPS_RECODE; i++)
    {
        portion_2.recode_gpsmap[i].theta = guandao_normalize_angle(portion_2.recode_gpsmap[i].theta + yaw_delta);
    }
    return yaw_delta;
}

static void portion2_capture_raw_transform(float yaw_delta)
{
    uint16 length = portion2_route_length[portion2_selected_route];
    uint16 offset = portion2_route_offset(portion2_selected_route);
    uint16 origin_index;
    float yaw_rad = yaw_delta / 180.0f * M_PI;

    if(length == 0)
    {
        portion2_raw_route_origin.x = 0.0f;
        portion2_raw_route_origin.y = 0.0f;
        portion2_raw_route_origin.theta = 0.0f;
        portion2_raw_route_cos = 1.0f;
        portion2_raw_route_sin = 0.0f;
        return;
    }
    if(length > PORTION2_ROUTE_MAX_POINTS) length = PORTION2_ROUTE_MAX_POINTS;
    origin_index = portion2_run_reverse ? (uint16)(offset + length - 1U) : offset;
    portion2_raw_route_origin = portion2_route_storage_get(origin_index);
    portion2_raw_route_cos = cosf(yaw_rad);
    portion2_raw_route_sin = sinf(yaw_rad);
}

static int16 portion2_terminal_raw_start_index(void)
{
    int16 length = guandao_clamp_length(portion_2.length_index);
    int16 start = length - 1;
    float covered = 0.0f;

    if(length <= 1) return 0;
    for(int16 i = length - 1; i > 0; i--)
    {
        float dx = portion_2.recode_map[i].x - portion_2.recode_map[i - 1].x;
        float dy = portion_2.recode_map[i].y - portion_2.recode_map[i - 1].y;
        covered += hypotf(dx, dy);
        start = i - 1;
        if(covered >= PORTION2_RAW_TERMINAL_LENGTH_M) break;
    }
    return start;
}

static void portion2_smooth_reference_route(void)
{
    int16 length = guandao_clamp_length(portion_2.length_index);
    int16 terminal_start = portion2_terminal_raw_start_index();

    if(length < 4) return;

    for(uint8 pass = 0; pass < PORTION2_REFERENCE_SMOOTH_PASSES; pass++)
    {
        portion2_reference_smooth_buffer[0] = portion_2.recode_map[0];
        portion2_reference_smooth_buffer[length - 1] = portion_2.recode_map[length - 1];

        for(int16 i = 1; i < length - 1; i++)
        {
            state_t prev = portion_2.recode_map[i - 1];
            state_t cur = portion_2.recode_map[i];
            state_t next = portion_2.recode_map[i + 1];
            float middle_weight = 1.0f - 2.0f * PORTION2_REFERENCE_SMOOTH_WEIGHT;

            if(i >= terminal_start)
            {
                portion2_reference_smooth_buffer[i] = cur;
                continue;
            }
            portion2_reference_smooth_buffer[i] = cur;
            portion2_reference_smooth_buffer[i].x =
                    prev.x * PORTION2_REFERENCE_SMOOTH_WEIGHT
                    + cur.x * middle_weight
                    + next.x * PORTION2_REFERENCE_SMOOTH_WEIGHT;
            portion2_reference_smooth_buffer[i].y =
                    prev.y * PORTION2_REFERENCE_SMOOTH_WEIGHT
                    + cur.y * middle_weight
                    + next.y * PORTION2_REFERENCE_SMOOTH_WEIGHT;
            portion2_reference_smooth_buffer[i].theta = guandao_segment_yaw(prev, next);
        }

        for(int16 i = 0; i < length; i++)
        {
            portion_2.recode_map[i] = portion2_reference_smooth_buffer[i];
        }
    }
}

static float guandao_segment_yaw(state_t from, state_t to)
{
    return atan2f(to.x - from.x, to.y - from.y) / M_PI * 180.0f;
}

static uint8 guandao_uses_portion3_trace_standard(guandao_state *state)
{
    return (route_setting_choice == 2 || state == &portion_2) ? 1 : 0;
}

static float guandao_max_route_turn(guandao_state *state, int start_index, int lookahead)
{
    float max_turn = 0.0f;
    int16 route_length = guandao_route_length(state);
    int end_index = start_index + lookahead;

    if(route_length < 3) return 0.0f;
    if(start_index < 1) start_index = 1;
    if(end_index > route_length - 2) end_index = route_length - 2;

    for(int i = start_index; i <= end_index; i++)
    {
        float yaw_in = guandao_segment_yaw(guandao_route_point(state, i - 1), guandao_route_point(state, i));
        float yaw_out = guandao_segment_yaw(guandao_route_point(state, i), guandao_route_point(state, i + 1));
        float turn = fabsf(guandao_normalize_angle(yaw_out - yaw_in));
        if(turn > max_turn) max_turn = turn;
    }

    return max_turn;
}

static float portion2_max_reference_turn(guandao_state *state, int plan_index, int raw_lookahead)
{
    float max_turn = 0.0f;
    int16 raw_length;
    int raw_start;
    int raw_end;

    if(state == 0 || state != &portion_2) return 0.0f;
    raw_length = guandao_clamp_length(state->length_index);
    if(raw_length < 3) return 0.0f;

    raw_start = portion2_raw_point_from_plan_index((int16)plan_index);
    raw_end = raw_start + raw_lookahead;
    if(raw_start < 1) raw_start = 1;
    if(raw_end > raw_length - 2) raw_end = raw_length - 2;

    for(int i = raw_start; i <= raw_end; i++)
    {
        float yaw_in = guandao_segment_yaw(state->recode_map[i - 1], state->recode_map[i]);
        float yaw_out = guandao_segment_yaw(state->recode_map[i], state->recode_map[i + 1]);
        float turn = fabsf(guandao_normalize_angle(yaw_out - yaw_in));
        if(turn > max_turn) max_turn = turn;
    }

    return max_turn;
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

static int guandao_find_front_index(guandao_state *state, int start_index, int end_index)
{
    int best_index = start_index;
    float best_score = 1000000.0f;
    int16 route_length = guandao_route_length(state);

    if(route_length <= 0) return 0;
    if(start_index < 0) start_index = 0;
    if(end_index >= route_length) end_index = route_length - 1;
    if(start_index > end_index) return start_index;

    for(int i = start_index; i <= end_index; i++)
    {
        state_t point = guandao_route_point(state, i);
        float dx = point.x - state->current_state.x;
        float dy = point.y - state->current_state.y;
        float distance = hypotf(dx, dy);
        float angle_to_point = atan2f(dx, dy) / M_PI * 180.0f;
        float angle_error = fabsf(guandao_normalize_angle(angle_to_point - state->current_state.theta));
        float score = distance + angle_error * 0.015f;

        if(angle_error <= GUANDAO_FRONT_TARGET_ANGLE && score < best_score)
        {
            best_score = score;
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
    if(state == &INS && key1_flag == 1 && dche_flag ==1)
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
    int16 terminal_start = (state == &portion_2) ? portion2_terminal_raw_start_index() : source_length;
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
        float turn_in = fabsf(guandao_normalize_angle(guandao_segment_yaw(p0, p1) - guandao_segment_yaw(p1, p2)));
        float turn_out = fabsf(guandao_normalize_angle(guandao_segment_yaw(p1, p2) - guandao_segment_yaw(p2, p3)));
        uint8 keep_terminal_linear = (state == &portion_2 && i >= terminal_start) ? 1U : 0U;
        uint8 keep_corner_linear = keep_terminal_linear ||
                (state != &portion_2 && (turn_in >= GUANDAO_SHARP_TURN_ANGLE || turn_out >= GUANDAO_SHARP_TURN_ANGLE));

        for(int j = 0; j < samples && state->planned_length < MAX_LENGTH_INDEX - 1; j++)
        {
            float t = (float)j / (float)samples;
            float t2 = t * t;
            float t3 = t2 * t;
            state_t out;
            if(keep_corner_linear)
            {
                out.x = p1.x + (p2.x - p1.x) * t;
                out.y = p1.y + (p2.y - p1.y) * t;
            }
            else
            {
                out.x = 0.5f * ((2.0f * p1.x) + (-p0.x + p2.x) * t + (2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 + (-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3);
                out.y = 0.5f * ((2.0f * p1.y) + (-p0.y + p2.y) * t + (2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 + (-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3);
            }
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
    float steering_gain = GUANDAO_STEERING_GAIN;
    float steering_limit = GUANDAO_STEERING_CMD_LIMIT;
    float steering_rate_limit = GUANDAO_STEER_RATE_LOW;
    static float last_target_steering = 0.0f;
    static uint32 last_steer_limit_ms = 0;
    int steer_preview_steps = preview_spets;
    int curve_preview_steps = 5;
    int16 route_length = guandao_route_length(state);
    float arrive_threshold = persuit_threshold;
    float upcoming_turn = 0.0f;
    float reference_turn = 0.0f;
    uint8 route11_reverse = (state == &portion_2
            && portion2_selected_route == PORTION2_ROUTE_STRAIGHT
            && portion2_run_drive_reverse) ? 1U : 0U;

    guandao_debug_stop_reason = 0;
    if(route_length == 0 || state->current_point_index == route_length)
    {
        guandao_debug_stop_reason = 1;
        * out_v_l = 0;
        * out_v_r = 0;
        *out_servo = 0;
        last_target_steering = 0.0f;
        last_steer_limit_ms = 0;
//        Buzzer_check(50);
        return;
    }

    state_t current_point = state->current_state;
    if(state->current_point_index < 0) state->current_point_index = 0;
    if(state->current_point_index >= route_length) state->current_point_index = route_length - 1;
    if(guandao_uses_portion3_trace_standard(state))
    {
        arrive_threshold = PORTION3_PURSUIT_THRESHOLD;
    }

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

    angle_diff = guandao_normalize_angle(angle_diff);
    if(fabsf(angle_diff) > GUANDAO_FRONT_TARGET_ANGLE)
    {
        int front_index = guandao_find_front_index(state, state->current_point_index, search_end_index);
        if(front_index > state->current_point_index)
        {
            state->current_point_index = front_index;
            target_point = guandao_route_point(state, state->current_point_index);
            dx = target_point.x - current_point.x;
            dy = target_point.y - current_point.y;
            distance_to_target = hypotf(dx, dy);
            guandao_debug_distance = distance_to_target;
            angle_to_target = atan2f(dx,dy)/M_PI*180.0f;
            angle_diff = guandao_normalize_angle(angle_to_target - state->current_state.theta);
            guandao_debug_stop_reason = 5;
        }
    }
    guandao_debug_angle_diff = angle_diff;

    if(state == &portion_2)
    {
        steering_gain = route11_reverse ? PORTION2_ROUTE11_STEERING_GAIN : PORTION2_STEERING_GAIN;
        steering_limit = route11_reverse ? PORTION2_ROUTE11_STEERING_CMD_LIMIT
                : (portion2_selected_route == PORTION2_ROUTE_SNAKE
                        ? PORTION2_SNAKE_STEERING_CMD_LIMIT : PORTION2_STEERING_CMD_LIMIT);
        steering_rate_limit = PORTION2_STEER_RATE_LIMIT;
        if(route11_reverse)
        {
            if(steer_preview_steps < PORTION2_ROUTE11_MIN_PREVIEW_STEPS) steer_preview_steps = PORTION2_ROUTE11_MIN_PREVIEW_STEPS;
            if(curve_preview_steps < PORTION2_ROUTE11_CURVE_PREVIEW_STEPS) curve_preview_steps = PORTION2_ROUTE11_CURVE_PREVIEW_STEPS;
        }
        else
        {
            if(steer_preview_steps < PORTION2_MIN_PREVIEW_STEPS) steer_preview_steps = PORTION2_MIN_PREVIEW_STEPS;
            if(curve_preview_steps < PORTION2_CURVE_PREVIEW_STEPS) curve_preview_steps = PORTION2_CURVE_PREVIEW_STEPS;
        }
    }

    if(distance_to_target <= arrive_threshold)
    {
        if(guandao_debug_stop_reason == 0) guandao_debug_stop_reason = 2;
        state->current_point_index++;
        if(state->current_point_index >= route_length )
        {   state->current_point_index = route_length;
            guandao_debug_stop_reason = 4;
            * out_v_l = 0;
            * out_v_r = 0;
            *out_servo = 0;
            last_target_steering = 0.0f;
            last_steer_limit_ms = 0;
//            Buzzer_check(50);
            return;
        }
    }

    if(base_speed > GUANDAO_HIGH_SPEED_THRESHOLD)
    {
        steering_gain = GUANDAO_HIGH_SPEED_GAIN;
        steering_limit = GUANDAO_HIGH_SPEED_CMD_LIMIT;
        if(base_speed >= GUANDAO_VERY_HIGH_SPEED_THRESHOLD)
        {
            steering_gain = GUANDAO_VERY_HIGH_SPEED_GAIN;
            steering_limit = GUANDAO_VERY_HIGH_CMD_LIMIT;
            if(steer_preview_steps < 8) steer_preview_steps = 8;
            steering_rate_limit = GUANDAO_STEER_RATE_HIGH;
        }
        else if(base_speed >= 10.0f)
        {
            if(steer_preview_steps < 4) steer_preview_steps = 4;
        }
    }
    upcoming_turn = guandao_max_route_turn(state, state->current_point_index, 12);
    if(upcoming_turn >= GUANDAO_SHARP_TURN_ANGLE)
    {
        if(steer_preview_steps > 3) steer_preview_steps = 3;
        if(curve_preview_steps > 6) curve_preview_steps = 6;
        steering_rate_limit = GUANDAO_STEER_RATE_LOW;
        steering_limit = GUANDAO_STEERING_CMD_LIMIT;
    }
    if(curve_preview_steps < steer_preview_steps + 3)
    {
        curve_preview_steps = steer_preview_steps + 3;
    }
    if(state == &portion_2)
    {
        steering_gain = route11_reverse ? PORTION2_ROUTE11_STEERING_GAIN : PORTION2_STEERING_GAIN;
        reference_turn = portion2_max_reference_turn(state, state->current_point_index,
                                                       PORTION2_SHARP_TURN_RAW_LOOKAHEAD);
        if(reference_turn >= PORTION2_SHARP_TURN_TRIGGER_DEG)
        {
            steering_limit = route11_reverse ? PORTION2_ROUTE11_SHARP_STEERING_CMD_LIMIT
                    : (portion2_selected_route == PORTION2_ROUTE_SNAKE
                            ? PORTION2_SNAKE_SHARP_STEERING_CMD_LIMIT : PORTION2_SHARP_STEERING_CMD_LIMIT);
        }
        else
        {
            steering_limit = route11_reverse ? PORTION2_ROUTE11_STEERING_CMD_LIMIT
                    : (portion2_selected_route == PORTION2_ROUTE_SNAKE
                            ? PORTION2_SNAKE_STEERING_CMD_LIMIT : PORTION2_STEERING_CMD_LIMIT);
        }
        steering_rate_limit = PORTION2_STEER_RATE_LIMIT;
        if(route11_reverse)
        {
            if(steer_preview_steps < PORTION2_ROUTE11_MIN_PREVIEW_STEPS) steer_preview_steps = PORTION2_ROUTE11_MIN_PREVIEW_STEPS;
            if(curve_preview_steps < PORTION2_ROUTE11_CURVE_PREVIEW_STEPS) curve_preview_steps = PORTION2_ROUTE11_CURVE_PREVIEW_STEPS;
        }
        else
        {
            if(steer_preview_steps < PORTION2_MIN_PREVIEW_STEPS) steer_preview_steps = PORTION2_MIN_PREVIEW_STEPS;
            if(curve_preview_steps < PORTION2_CURVE_PREVIEW_STEPS) curve_preview_steps = PORTION2_CURVE_PREVIEW_STEPS;
        }
    }

    pursuit_midhandle(state , &current_point , steer_preview_steps , &preview_alpha , &actual_ld);
    pursuit_midhandle(state , &current_point , curve_preview_steps , &preview_alpha2 , &actual_ld2);

   if(angle_diff >=90)
   {
       target_steering = steering_limit;

   }
   else if(angle_diff <= -90)
   {
       target_steering = -steering_limit;
   }
   else if(fabsf(angle_diff) <=90)
   {
       target_steering = steering_gain*atan2f(2.0f * WHEEL_BASE * sinf(preview_alpha/180.0f*M_PI), actual_ld)/M_PI*180.0f;
   }
   ips200_show_float(X(10),  Y(9),target_steering ,5 ,5);
   Value_Limit_float(&target_steering ,-MAX_STEERING_RAD,MAX_STEERING_RAD);

//   slip_cheak(&guandao_ecd,target_steering);

   state_t final_point = guandao_route_point(state, route_length - 1);
   if(state == &portion_2)
   {
       final_point.theta = portion2_run_final_yaw;
   }
   float dist_to_final = get_distance(state->current_state, final_point);
   guandao_debug_dist_final = dist_to_final;
   float v_center = base_speed;
   if(route11_reverse && v_center > PORTION2_ROUTE11_SPEED)
   {
       v_center = PORTION2_ROUTE11_SPEED;
   }
   uint8 portion1_parking_zone = 0;

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
           last_target_steering = 0.0f;
           last_steer_limit_ms = 0;
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
           break;
       case 3:

           break;
       default : break;
   }

   if(state == &portion_2)
   {
       target_steering = portion2_guided_terminal_steering(
               target_steering, dist_to_final, portion2_run_final_yaw, Yaw_1);
   }

   Value_Limit_float(&target_steering ,-steering_limit,steering_limit);

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

   if(upcoming_turn >= GUANDAO_SHARP_TURN_ANGLE)
   {
       if(v_center > GUANDAO_SHARP_TURN_SPEED) v_center = GUANDAO_SHARP_TURN_SPEED;
   }
   else if(fabsf(angle_diff) > 25.0f || fabsf(preview_alpha2) > GUANDAO_CURVE_TRIGGER_ANGLE)
   {
       if(v_center > GUANDAO_LARGE_CURVE_SPEED) v_center = GUANDAO_LARGE_CURVE_SPEED;
   }
   if(state == &portion_2 && portion2_selected_route < PORTION2_GUIDED_ROUTE_COUNT
           && dist_to_final <= PORTION2_GUIDED_YAW_SLOW_DIST_M
           && fabsf(angle_diff) >= PORTION2_GUIDED_YAW_SLOW_TRIGGER_DEG
           && v_center > PORTION2_GUIDED_YAW_SLOW_SPEED)
   {
       v_center = PORTION2_GUIDED_YAW_SLOW_SPEED;
   }
   if(v_center < MIN_SPEED) v_center = MIN_SPEED;

   if (!portion1_parking_zone && dist_to_final < final_dsts && state->current_point_index >= route_length - 30)
   {
       float terminal_speed = base_speed * (dist_to_final / final_dsts);

       arrive_threshold = persuit_threshold*(dist_to_final / final_dsts);
       if(arrive_threshold < 0.3f){arrive_threshold = 0.3f;}
       if(guandao_uses_portion3_trace_standard(state))
       {
           if(terminal_speed < MIN_SPEED) terminal_speed = MIN_SPEED;
       }
       else if(terminal_speed < PORTION1_END_MIN_SPEED) terminal_speed = PORTION1_END_MIN_SPEED;
       if(v_center > terminal_speed) v_center = terminal_speed;
   }

   if(state == &portion_2 && dist_to_final <= PORTION2_TERMINAL_POSE_LENGTH_M
           && v_center > PORTION2_TERMINAL_APPROACH_SPEED)
   {
       v_center = PORTION2_TERMINAL_APPROACH_SPEED;
   }

   if(portion1_parking_zone && v_center < PORTION1_PARK_MIN_SPEED) v_center = PORTION1_PARK_MIN_SPEED;
    if(state == &portion_2)
    {
        int16 raw_length = guandao_clamp_length(state->length_index);
        int16 raw_point = portion2_raw_point_from_plan_index(guandao_clamp_length(state->current_point_index));
        float final_stop_distance = portion2_selected_route == PORTION2_ROUTE_SNAKE
                ? PORTION2_SNAKE_FINAL_STOP_DIST
                : (portion2_selected_route < PORTION2_GUIDED_ROUTE_COUNT
                        ? PORTION2_GUIDED_FINAL_STOP_DIST : PORTION2_FINAL_RAW_POINT_STOP_DIST);
        if(portion2_route12_overshoot_detect(raw_point, raw_length, dist_to_final)
                || portion2_final_zone_overshoot_detect(raw_point, raw_length, dist_to_final))
        {
            state->current_point_index = route_length;
            guandao_debug_stop_reason = (portion2_selected_route == PORTION2_ROUTE_SNAKE) ? 12 : 13;
            *out_v_l = 0;
            *out_v_r = 0;
            *out_servo = 0;
            last_target_steering = 0.0f;
            last_steer_limit_ms = 0;
            portion2_route12_overshoot_reset();
            portion2_final_zone_reset();
            return;
        }
        if(raw_length > 0 && raw_point >= raw_length - 1
                && dist_to_final <= final_stop_distance)
        {
            uint8 align_status = portion2_final_yaw_align(final_point, dist_to_final, out_v_l, out_v_r, out_servo);
            if(align_status == PORTION2_FINAL_YAW_ALIGN_RUNNING)
            {
                last_target_steering = 0.0f;
                last_steer_limit_ms = 0;
                return;
            }
            if(align_status == PORTION2_FINAL_YAW_ALIGN_REACQUIRE)
            {
                if(portion2_final_zone_armed)
                {
                    state->current_point_index = route_length;
                    guandao_debug_stop_reason = 13;
                    *out_v_l = 0;
                    *out_v_r = 0;
                    *out_servo = 0;
                    last_target_steering = 0.0f;
                    last_steer_limit_ms = 0;
                    portion2_final_zone_reset();
                    return;
                }
                if(state->current_point_index >= route_length - 1) state->current_point_index = route_length - 2;
            }
            else
            {
                state->current_point_index = route_length;
                if(guandao_debug_stop_reason != 10) guandao_debug_stop_reason = 8;
                * out_v_l = 0;
                * out_v_r = 0;
                *out_servo = 0;
                last_target_steering = 0.0f;
                last_steer_limit_ms = 0;
                portion2_final_zone_reset();
                return;
            }
        }
   }
   if(guandao_uses_portion3_trace_standard(state) && state->current_point_index >= route_length - 1
           && dist_to_final <= PORTION3_FINAL_STOP_DIST)
   {
       if(state == &portion_2 && !portion2_final_yaw_align(final_point, dist_to_final, out_v_l, out_v_r, out_servo))
       {
           last_target_steering = 0.0f;
           last_steer_limit_ms = 0;
           return;
       }
       state->current_point_index = route_length;
       if(guandao_debug_stop_reason != 10) guandao_debug_stop_reason = 8;
       * out_v_l = 0;
       * out_v_r = 0;
       *out_servo = 0;
       last_target_steering = 0.0f;
       last_steer_limit_ms = 0;
       return;
   }
   Value_Limit_float(&target_steering ,-steering_limit,steering_limit);

   if(state->current_point_index <= 2)
   {
       last_target_steering = target_steering;
       last_steer_limit_ms = system_getval_ms();
   }
   uint32 steer_now_ms = system_getval_ms();
   uint32 steer_elapsed_ms = (last_steer_limit_ms == 0) ? 20u : (uint32)(steer_now_ms - last_steer_limit_ms);
   if(steer_elapsed_ms > 100u) steer_elapsed_ms = 20u;
   float steer_delta_limit = steering_rate_limit * ((float)steer_elapsed_ms / 20.0f);
   float steer_delta = target_steering - last_target_steering;
   Value_Limit_float(&steer_delta, -steer_delta_limit, steer_delta_limit);
   target_steering = last_target_steering + steer_delta;
   Value_Limit_float(&target_steering ,-steering_limit,steering_limit);
   last_target_steering = target_steering;
   last_steer_limit_ms = steer_now_ms;

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
    preview_index = state->current_point_index+index;

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

static uint8 portion2_route_uses_gps(uint8 route_id)
{
    return (route_id < PORTION2_ROUTE_COUNT) ? 1U : 0U;
}

static uint8 portion2_route_ready_for_run(uint8 route_id)
{
    uint8 required;
    uint8 gps_count;
    uint16 gps_offset;
    int16 last_gps_raw_point;

    if(route_id >= PORTION2_ROUTE_COUNT) return 0;

    required = portion2_route_required_gps(route_id);
    if(required == 0) return 0;
    if(portion2_route_length[route_id] < required) return 0;
    gps_count = portion2_route_gps_count[route_id];
    if(gps_count < required) return 0;

    gps_offset = portion2_route_gps_offset(route_id);
    last_gps_raw_point = portion2_gps_storage_get(gps_offset + gps_count - 1).cheak_flag;
    if(last_gps_raw_point < 0 || last_gps_raw_point >= (int16)portion2_route_length[route_id]) return 0;
    if(last_gps_raw_point < (int16)portion2_route_length[route_id] - 1 - PORTION2_GPS_END_MAX_RAW_GAP) return 0;

    return 1;
}

static uint16 portion2_route_gps_offset(uint8 route_id)
{
    if(route_id >= PORTION2_ROUTE_COUNT) return (uint16)PORTION2_TOTAL_GPS_COUNT;
    return (uint16)(route_id * PORTION2_GPS_PER_ROUTE);
}

static void portion2_translate_route_to_origin(void)
{
    int16 length = guandao_clamp_length(portion_2.length_index);
    state_t start_point;

    if(length <= 0) return;
    start_point = portion_2.recode_map[0];
    for(int16 i = 0; i < length; i++)
    {
        portion_2.recode_map[i].x -= start_point.x;
        portion_2.recode_map[i].y -= start_point.y;
    }
}

static float portion2_recorded_route_distance(uint8 route_id)
{
    uint16 length;
    uint16 offset;
    float distance = 0.0f;

    if(route_id >= PORTION2_ROUTE_COUNT) return 0.0f;
    length = portion2_route_length[route_id];
    if(length > PORTION2_ROUTE_MAX_POINTS) length = PORTION2_ROUTE_MAX_POINTS;
    offset = portion2_route_offset(route_id);

    for(uint16 i = 1; i < length; i++)
    {
        distance += get_distance(portion2_route_storage_get(offset + i - 1),
                                 portion2_route_storage_get(offset + i));
    }
    return distance;
}

static void portion2_serial_log_record_status(void)
{
    static uint32 last_ms = 0;
    uint32 now_ms;
    uint8 route_id;
    char line[256];
    int pos = 0;

    if(portion2_record_state != 1) return;
    route_id = portion2_record_route;
    if(route_id >= PORTION2_ROUTE_COUNT) return;

    now_ms = system_getval_ms();
    if((uint32)(now_ms - last_ms) < 500U) return;
    last_ms = now_ms;

    pos += sprintf(line,
                   "[P2-REC-STATUS] route=%u state=%u raw=%u/%u gps=%u/%u dist=",
                   (unsigned)(route_id + 1),
                   (unsigned)portion2_record_state,
                   (unsigned)portion2_route_length[route_id],
                   (unsigned)PORTION2_ROUTE_MAX_POINTS,
                   (unsigned)portion2_route_gps_count[route_id],
                   (unsigned)PORTION2_GPS_PER_ROUTE);
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_recorded_route_distance(route_id));
    pos += sprintf(&line[pos], " step=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), recode_threshold);
    pos += sprintf(&line[pos], " x=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), passage.current_state.x);
    pos += sprintf(&line[pos], " y=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), passage.current_state.y);
    pos += sprintf(&line[pos], " yaw=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), Yaw_1);
    pos += sprintf(&line[pos], "\r\n");
    uart_write_string(DEBUG_UART_INDEX, line);
}

void portion2_serial_dump_routes(void)
{
    char line[180];

    uart_write_string(DEBUG_UART_INDEX, "[P2-DUMP] routes summary\r\n");
    for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
    {
        int pos = sprintf(line,
                "[P2-DUMP] route=%u pts=%u gps=%u/%u saved=%s start_yaw=",
                (unsigned)(i + 1),
                (unsigned)portion2_route_length[i],
                (unsigned)portion2_route_gps_count[i],
                (unsigned)PORTION2_GPS_PER_ROUTE,
                portion2_route_saved_flag[i] ? "YES" : "NO");
        portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_route_start_yaw[i]);
        pos += sprintf(&line[pos], " final_yaw=");
        portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_route_final_yaw[i]);
        pos += sprintf(&line[pos], "\r\n");
        uart_write_string(DEBUG_UART_INDEX, line);
    }
    uart_write_string(DEBUG_UART_INDEX, "[P2-DUMP] send D1-D9 to dump points, T toggle run trace, S stop\r\n");
}

void portion2_serial_dump_route(uint8 route_id)
{
    uint16 len;
    uint16 offset;
    char line[120];

    if(route_id >= PORTION2_ROUTE_COUNT)
    {
        uart_write_string(DEBUG_UART_INDEX, "[P2-DUMP] invalid route\r\n");
        return;
    }

    len = portion2_route_length[route_id];
    offset = portion2_route_offset(route_id);
    if(len > PORTION2_ROUTE_MAX_POINTS) len = PORTION2_ROUTE_MAX_POINTS;

    sprintf(line,
            "[P2-DUMP] route=%u pts=%u gps=%u/%u begin\r\n",
            (unsigned)(route_id + 1),
            (unsigned)len,
            (unsigned)portion2_route_gps_count[route_id],
            (unsigned)PORTION2_GPS_PER_ROUTE);
    uart_write_string(DEBUG_UART_INDEX, line);

    for(uint16 i = 0; i < len; i++)
    {
        portion2_serial_write_state_point("[P2-DUMP]", route_id, i, portion2_route_storage_get(offset + i));
    }
    for(uint8 i = 0; i < portion2_route_gps_count[route_id] && i < PORTION2_GPS_PER_ROUTE; i++)
    {
        portion2_serial_write_gps_point("[P2-DUMP-GPS]", route_id, i,
                                        portion2_gps_storage_get(portion2_route_gps_offset(route_id) + i));
    }

    uart_write_string(DEBUG_UART_INDEX, "[P2-DUMP] route end\r\n");
}

void portion2_serial_toggle_trace(void)
{
    portion2_serial_trace_enabled = !portion2_serial_trace_enabled;
    uart_write_string(DEBUG_UART_INDEX, portion2_serial_trace_enabled ? "[P2-RUN] trace=ON\r\n" : "[P2-RUN] trace=OFF\r\n");
}

uint8 portion2_is_running(void)
{
    return (portion2_state_flag != 0) ? 1 : 0;
}

static void portion2_record_point(void)
{
    uint16 len = portion2_route_length[portion2_record_route];
    uint16 offset = portion2_route_offset(portion2_record_route);

    if(portion2_record_route >= PORTION2_ROUTE_COUNT) return;
    if(len >= PORTION2_ROUTE_MAX_POINTS) return;

    if(len == 0)
    {
        portion2_route_storage_set(offset, passage.current_state);
        portion2_route_length[portion2_record_route] = 1;
        portion2_route_saved_flag[portion2_record_route] = 0;
        portion2_serial_write_state_point("[P2-REC]", portion2_record_route, 0, passage.current_state);
        return;
    }

    if(get_distance(passage.current_state, portion2_route_storage_get(offset + len - 1)) >= recode_threshold)
    {
        portion2_route_storage_set(offset + len, passage.current_state);
        portion2_route_length[portion2_record_route]++;
        portion2_route_saved_flag[portion2_record_route] = 0;
        portion2_serial_write_state_point("[P2-REC]", portion2_record_route, len, passage.current_state);
    }
}

static void portion2_record_capture_final_pose(void)
{
    char line[120];
    int pos;

    if(portion2_record_route >= PORTION2_ROUTE_COUNT) return;
    portion2_route_final_yaw[portion2_record_route] = Yaw_1;
    pos = sprintf(line, "[P2-REC-END-POSE] route=%u start_yaw=", (unsigned)(portion2_record_route + 1));
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_route_start_yaw[portion2_record_route]);
    pos += sprintf(&line[pos], " final_yaw=");
    portion2_serial_append_fixed100(line, &pos, (int)sizeof(line), portion2_route_final_yaw[portion2_record_route]);
    pos += sprintf(&line[pos], "\r\n");
    uart_write_string(DEBUG_UART_INDEX, line);
}

static double portion2_gps_median(const double *values, uint8 count)
{
    double sorted[PORTION2_GPS_ORIGIN_SAMPLE_COUNT];

    if(values == 0 || count == 0 || count > PORTION2_GPS_ORIGIN_SAMPLE_COUNT) return 0.0;
    for(uint8 i = 0; i < count; i++) sorted[i] = values[i];
    for(uint8 i = 1; i < count; i++)
    {
        double value = sorted[i];
        int8 j = (int8)i - 1;
        while(j >= 0 && sorted[j] > value)
        {
            sorted[j + 1] = sorted[j];
            j--;
        }
        sorted[j + 1] = value;
    }
    return sorted[count / 2U];
}

static void portion2_gps_filter_latest(uint8 sample_count, double *latitude, double *longitude)
{
    double latitudes[PORTION2_GPS_ORIGIN_SAMPLE_COUNT];
    double longitudes[PORTION2_GPS_ORIGIN_SAMPLE_COUNT];

    if(sample_count > portion2_gps_record_filter.count) sample_count = portion2_gps_record_filter.count;
    for(uint8 i = 0; i < sample_count; i++)
    {
        uint8 index = (uint8)((portion2_gps_record_filter.next
                + PORTION2_GPS_ORIGIN_SAMPLE_COUNT - sample_count + i)
                % PORTION2_GPS_ORIGIN_SAMPLE_COUNT);
        latitudes[i] = portion2_gps_record_filter.latitude[index];
        longitudes[i] = portion2_gps_record_filter.longitude[index];
    }
    *latitude = portion2_gps_median(latitudes, sample_count);
    *longitude = portion2_gps_median(longitudes, sample_count);
}

static void portion2_gps_filter_clear_samples(void)
{
    portion2_gps_record_filter.count = 0;
    portion2_gps_record_filter.next = 0;
    portion2_gps_record_filter.last_sample_ms = 0;
}

static void portion2_gps_filter_update(void)
{
    uint32 rmc_sequence = portion2_gps_get_rmc_sequence();

    if(rmc_sequence == portion2_gps_record_filter.last_rmc_sequence) return;
    portion2_gps_record_filter.last_rmc_sequence = rmc_sequence;
    if(!gnss.state || gnss.satellite_used < PORTION2_GPS_RECORD_MIN_SATELLITES ||
       gnss.hdop <= 0.0f || gnss.hdop > PORTION2_GPS_MAX_HDOP ||
       gnss.latitude == 0.0 || gnss.longitude == 0.0)
    {
        portion2_gps_reject_reason = (!gnss.state) ? PORTION2_GPS_REJECT_NO_FIX :
                ((gnss.satellite_used < PORTION2_GPS_RECORD_MIN_SATELLITES) ? PORTION2_GPS_REJECT_LOW_SAT :
                ((gnss.hdop <= 0.0f || gnss.hdop > PORTION2_GPS_MAX_HDOP) ? PORTION2_GPS_REJECT_BAD_HDOP :
                PORTION2_GPS_REJECT_ZERO_COORD));
        if(portion2_record_state != 1 ||
           portion2_route_gps_count[portion2_record_route] == 0)
        {
            portion2_gps_filter_clear_samples();
        }
        return;
    }

    portion2_gps_record_filter.latitude[portion2_gps_record_filter.next] = gnss.latitude;
    portion2_gps_record_filter.longitude[portion2_gps_record_filter.next] = gnss.longitude;
    portion2_gps_record_filter.next = (uint8)((portion2_gps_record_filter.next + 1U)
            % PORTION2_GPS_ORIGIN_SAMPLE_COUNT);
    if(portion2_gps_record_filter.count < PORTION2_GPS_ORIGIN_SAMPLE_COUNT)
    {
        portion2_gps_record_filter.count++;
    }
    portion2_gps_record_filter.last_sample_ms = system_getval_ms();
}

static uint8 portion2_gps_filter_fresh(void)
{
    return (portion2_gps_record_filter.last_sample_ms != 0U &&
            (uint32)(system_getval_ms() - portion2_gps_record_filter.last_sample_ms)
                    <= PORTION2_GPS_FILTER_STALE_MS) ? 1U : 0U;
}

static uint8 portion2_gps_origin_ready(void)
{
    double median_latitude;
    double median_longitude;

    if(portion2_gps_record_filter.count < PORTION2_GPS_ORIGIN_SAMPLE_COUNT)
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_STABILIZE;
        return 0;
    }
    if(!portion2_gps_filter_fresh())
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_STALE;
        return 0;
    }
    portion2_gps_filter_latest(PORTION2_GPS_ORIGIN_SAMPLE_COUNT,
            &median_latitude, &median_longitude);
    for(uint8 i = 0; i < PORTION2_GPS_ORIGIN_SAMPLE_COUNT; i++)
    {
        if(get_two_points_distance(median_latitude, median_longitude,
                portion2_gps_record_filter.latitude[i],
                portion2_gps_record_filter.longitude[i]) > PORTION2_GPS_ORIGIN_STABILITY_M)
        {
            portion2_gps_reject_reason = PORTION2_GPS_REJECT_STABILIZE;
            return 0;
        }
    }
    portion2_gps_reject_reason = PORTION2_GPS_REJECT_NONE;
    return 1;
}

static uint8 portion2_gps_candidate_valid(uint8 route_id, double candidate_latitude,
        double candidate_longitude)
{
    uint8 gps_count;
    uint16 previous_index;
    GPS_state previous;
    float gps_distance;
    float inertial_distance;

    if(route_id >= PORTION2_ROUTE_COUNT)
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_INDEX;
        return 0;
    }
    if(candidate_latitude == 0.0 || candidate_longitude == 0.0)
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_ZERO_COORD;
        return 0;
    }

    gps_count = portion2_route_gps_count[route_id];
    if(gps_count == 0)
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_NONE;
        return 1;
    }
    previous_index = portion2_route_gps_offset(route_id) + gps_count - 1;
    if(previous_index >= PORTION2_TOTAL_GPS_COUNT)
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_INDEX;
        return 0;
    }
    previous = portion2_gps_storage_get(previous_index);
    gps_distance = (float)get_two_points_distance(previous.lat, previous.lon,
            candidate_latitude, candidate_longitude);
    if(gps_distance < PORTION2_GPS_RECORD_MIN_MOVE_M)
    {
        portion2_gps_reject_reason = PORTION2_GPS_REJECT_REPEAT;
        return 0;
    }
    if(portion2_gps_auto_has_point[route_id])
    {
        inertial_distance = get_distance(passage.current_state, portion2_gps_auto_last_state[route_id]);
        if(gps_distance > inertial_distance + PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M)
        {
            portion2_gps_reject_reason = PORTION2_GPS_REJECT_JUMP;
            return 0;
        }
    }

    portion2_gps_reject_reason = PORTION2_GPS_REJECT_NONE;
    return 1;
}

static const char *portion2_gps_reject_reason_text(void)
{
    switch(portion2_gps_reject_reason)
    {
        case PORTION2_GPS_REJECT_NO_FIX: return "NO_FIX";
        case PORTION2_GPS_REJECT_LOW_SAT: return "LOW_SAT";
        case PORTION2_GPS_REJECT_BAD_HDOP: return "BAD_HDOP";
        case PORTION2_GPS_REJECT_ZERO_COORD: return "ZERO_COORD";
        case PORTION2_GPS_REJECT_INTERVAL: return "INTERVAL";
        case PORTION2_GPS_REJECT_REPEAT: return "REPEAT";
        case PORTION2_GPS_REJECT_JUMP: return "JUMP";
        case PORTION2_GPS_REJECT_CAPACITY: return "CAPACITY";
        case PORTION2_GPS_REJECT_INDEX: return "INDEX";
        case PORTION2_GPS_REJECT_NO_RMC: return "NO_RMC";
        case PORTION2_GPS_REJECT_STABILIZE: return "STABILIZE";
        case PORTION2_GPS_REJECT_STALE: return "STALE";
        default: return "NONE";
    }
}

static uint8 portion2_gps_coordinate_valid(void)
{
    return (gnss.latitude != 0.0 && gnss.longitude != 0.0) ? 1U : 0U;
}

static void portion2_serial_log_gps_reject(void)
{
    uint32 now_ms = system_getval_ms();
    uint8 gps_count;
    char line[192];

    if(portion2_gps_reject_reason == PORTION2_GPS_REJECT_NONE) return;
    if((uint32)(now_ms - portion2_gps_reject_last_log_ms) < 1000U) return;
    portion2_gps_reject_last_log_ms = now_ms;
    gps_count = (portion2_record_route < PORTION2_ROUTE_COUNT)
            ? portion2_route_gps_count[portion2_record_route]
            : 0U;
    sprintf(line,
            "[P2-REC-GPS-SKIP] route=%u reason=%s fix=%u sats=%u hdop100=%u coord=%u seq=%lu gps=%u/%u\r\n",
            (unsigned)(portion2_record_route + 1),
            portion2_gps_reject_reason_text(),
            (unsigned)gnss.state,
            (unsigned)gnss.satellite_used,
            (unsigned)(gnss.hdop * 100.0f + 0.5f),
            (unsigned)portion2_gps_coordinate_valid(),
            (unsigned long)portion2_gps_get_fix_sequence(),
            (unsigned)gps_count,
            (unsigned)PORTION2_GPS_PER_ROUTE);
    uart_write_string(DEBUG_UART_INDEX, line);
}

static uint8 portion2_record_reject_gps(portion2_gps_reject_reason_t reason)
{
    portion2_gps_reject_reason = reason;
    portion2_serial_log_gps_reject();
    return 0;
}

static uint8 portion2_record_try_gps_point(uint8 force_endpoint)
{
    uint8 gps_count;
    uint8 filter_sample_count;
    uint16 gps_index;
    GPS_state gps_point;
    double filtered_latitude;
    double filtered_longitude;

    if(portion2_record_route >= PORTION2_ROUTE_COUNT) return portion2_record_reject_gps(PORTION2_GPS_REJECT_INDEX);
    gps_count = portion2_route_gps_count[portion2_record_route];
    if(gps_count == 0)
    {
        if(!portion2_gps_origin_ready()) return portion2_record_reject_gps(portion2_gps_reject_reason);
        filter_sample_count = PORTION2_GPS_ORIGIN_SAMPLE_COUNT;
    }
    else
    {
        if(portion2_gps_record_filter.count < PORTION2_GPS_FILTER_SAMPLE_COUNT)
        {
            return portion2_record_reject_gps(PORTION2_GPS_REJECT_STABILIZE);
        }
        if(!portion2_gps_filter_fresh()) return portion2_record_reject_gps(PORTION2_GPS_REJECT_STALE);
        if(portion2_gps_record_filter.last_rmc_sequence == portion2_gps_last_stored_rmc_sequence)
        {
            return portion2_record_reject_gps(PORTION2_GPS_REJECT_NO_RMC);
        }
        filter_sample_count = PORTION2_GPS_FILTER_SAMPLE_COUNT;
    }
    if(force_endpoint)
    {
        if(gps_count >= PORTION2_GPS_PER_ROUTE) return portion2_record_reject_gps(PORTION2_GPS_REJECT_CAPACITY);
    }
    else
    {
        if(gps_count >= PORTION2_GPS_PER_ROUTE - 1) return portion2_record_reject_gps(PORTION2_GPS_REJECT_CAPACITY);
        if(portion2_gps_auto_has_point[portion2_record_route] &&
           get_distance(passage.current_state, portion2_gps_auto_last_state[portion2_record_route])
                   < PORTION2_AUTO_GPS_RECORD_DIST) return portion2_record_reject_gps(PORTION2_GPS_REJECT_INTERVAL);
    }
    portion2_gps_filter_latest(filter_sample_count, &filtered_latitude, &filtered_longitude);
    if(!portion2_gps_candidate_valid(portion2_record_route, filtered_latitude, filtered_longitude))
    {
        portion2_serial_log_gps_reject();
        return 0;
    }

    gps_index = portion2_route_gps_offset(portion2_record_route) + gps_count;
    if(gps_index >= PORTION2_TOTAL_GPS_COUNT) return portion2_record_reject_gps(PORTION2_GPS_REJECT_INDEX);

    gps_point.lat = filtered_latitude;
    gps_point.lon = filtered_longitude;
    gps_point.theta = Yaw_1;
    gps_point.cheak_flag = (portion2_route_length[portion2_record_route] > 0)
            ? (int16)portion2_route_length[portion2_record_route] - 1 : 0;
    portion2_gps_storage_set(gps_index, gps_point);
    portion2_route_gps_count[portion2_record_route]++;
    passage.gps_recode_length = portion2_route_gps_offset(portion2_record_route) + portion2_route_gps_count[portion2_record_route];
    portion2_gps_auto_last_state[portion2_record_route] = passage.current_state;
    portion2_gps_auto_has_point[portion2_record_route] = 1;
    portion2_gps_last_stored_rmc_sequence = portion2_gps_record_filter.last_rmc_sequence;
    portion2_route_saved_flag[portion2_record_route] = 0;
    portion2_gps_reject_reason = PORTION2_GPS_REJECT_NONE;
    portion2_serial_write_gps_point("[P2-REC-GPS]", portion2_record_route, gps_count, gps_point);
    Buzzer_check(30);
    return 1;
}

static void portion2_auto_record_gps_point(void)
{
    if(!GPS_WORK_FLAG) return;
    if(portion2_record_route >= PORTION2_ROUTE_COUNT) return;
    portion2_record_try_gps_point(0);
}

void portion2_reset(void)
{
    portion2_state_flag = 0;
    portion2_run_reverse = 0;
    portion2_run_drive_reverse = 0;
    portion2_route12_overshoot_reset();
    portion2_final_zone_reset();
    daoche_flag = 0;
    portion2_gps_fusion_reset();
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
    if(GPS_WORK_FLAG && p == &portion_2 && portion2_route_uses_gps(portion2_selected_route))
    {
        portion2_gps_fusion_update(p);
    }
    pursuit_contral_mode(p ,&out_v_l ,&out_v_r ,&out_servo);
    if(GPS_WORK_FLAG && p != &portion_2)trace_gps(p);
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
    portion2_record_start_pending = 0;
    portion2_gps_filter_clear_samples();
    portion2_gps_last_stored_rmc_sequence = 0;
    guandao_state_init(&passage);
    for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
    {
        portion2_route_length[i] = 0;
        portion2_route_gps_count[i] = 0;
        portion2_route_start_yaw[i] = 0.0f;
        portion2_route_final_yaw[i] = 0.0f;
        portion2_gps_auto_has_point[i] = 0;
        portion2_route_saved_flag[i] = 0;
    }
    passage.gps_recode_length = 0;
}

void portion2_record_mark_loaded_routes_saved(void)
{
    for(uint8 i = 0; i < PORTION2_ROUTE_COUNT; i++)
    {
        portion2_route_saved_flag[i] = (portion2_route_length[i] > 0) ? 1 : 0;
    }
}

static void portion2_record_key_state_reset(void)
{
    portion2_record_k1_start_ms = 0;
    portion2_record_k2_start_ms = 0;
    portion2_record_k3_start_ms = 0;
    portion2_record_k4_start_ms = 0;
    portion2_record_k1_wait_release = 0;
    portion2_record_k2_wait_release = 0;
    portion2_record_k3_wait_release = 0;
    portion2_record_k4_wait_release = 0;
    key1_flag = 0;
    key2_flag = 0;
    key3_flag = 0;
    key4_flag = 0;
}

void portion2_mode_key_transition_lock(void)
{
    portion2_mode_k4_start_ms = 0;
    portion2_mode_k4_wait_release = 1;
    portion2_record_k4_start_ms = 0;
    portion2_record_k4_wait_release = 0;
    key4_flag = 0;
}

uint8 portion2_mode_k4_short_event(void)
{
    uint32 now_ms = system_getval_ms();
    uint8 key_down = (gpio_get_level(KEY4) == 0);
    uint8 release_event = 0;
    uint8 short_event = 0;

    if(key4_flag)
    {
        key4_flag = 0;
        release_event = 1;
    }

    if(portion2_mode_k4_wait_release)
    {
        if(!key_down)
        {
            portion2_mode_k4_wait_release = 0;
            portion2_mode_k4_start_ms = 0;
        }
        return 0;
    }

    if(key_down)
    {
        if(portion2_mode_k4_start_ms == 0) portion2_mode_k4_start_ms = now_ms;
        if((uint32)(now_ms - portion2_mode_k4_start_ms) > 1500U)
        {
            portion2_mode_k4_wait_release = 1;
        }
    }
    else
    {
        if(portion2_mode_k4_start_ms != 0 || release_event)
        {
            short_event = 1;
        }
        portion2_mode_k4_start_ms = 0;
    }

    return short_event;
}

void portion2_record_enter_mode(void)
{
    portion2_record_key_state_reset();
    portion2_record_start_pending = 0;
    daoche_flag = 0;
    if(portion2_record_route >= PORTION2_ROUTE_COUNT)
    {
        portion2_record_route = 0;
    }
    if(portion2_record_state == 1)
    {
        portion2_record_state = 2;
    }
}

static void portion2_record_begin_route(void)
{
    guandao_state_init(&passage);
    daoche_flag = portion2_route_uses_reverse_drive(portion2_record_route);
    passage.current_state.theta = daoche_flag ? Yaw_1 + 180.0f : Yaw_1;
    angle_plan(&passage.current_state.theta);
    Encoder_Get(&guandao_ecd);
    portion2_route_length[portion2_record_route] = 0;
    portion2_route_gps_count[portion2_record_route] = 0;
    portion2_route_start_yaw[portion2_record_route] = Yaw_1;
    portion2_route_final_yaw[portion2_record_route] = Yaw_1;
    portion2_gps_auto_has_point[portion2_record_route] = 0;
    portion2_route_saved_flag[portion2_record_route] = 0;
    portion2_gps_last_stored_rmc_sequence = 0;
    portion2_gps_reject_reason = PORTION2_GPS_REJECT_NONE;
    portion2_gps_reject_last_log_ms = 0;
    portion2_record_point();
    if(!portion2_record_try_gps_point(0))
    {
        portion2_record_start_pending = 1;
        return;
    }
    portion2_record_state = 1;
    portion2_record_start_pending = 0;
    portion2_serial_log_record_event("START");
    Buzzer_check(50);
}

static uint8 portion2_route_uses_reverse_drive(uint8 route_id)
{
    return (route_id == PORTION2_ROUTE_STRAIGHT) ? 1U : 0U;
}

void portion2_record_task(void)
{
    uint8 k1_short = 0, k2_short = 0, k3_short = 0, k4_short = 0;
    uint8 k1_long = 0, k2_long = 0, k3_long = 0, k4_long = 0;
    uint32 now_ms;

    if(portion2_record_route >= PORTION2_ROUTE_COUNT)
    {
        portion2_record_route = PORTION2_ROUTE_COUNT - 1;
        if(portion2_record_state != 1)
        {
            portion2_record_state = 2;
        }
    }
    portion2_gps_filter_update();

    now_ms = system_getval_ms();
    if(gpio_get_level(KEY1) == 0)
    {
        if(portion2_record_k1_start_ms == 0) portion2_record_k1_start_ms = now_ms;
        if(!portion2_record_k1_wait_release && (uint32)(now_ms - portion2_record_k1_start_ms) > 1500U)
        {
            k1_long = 1; portion2_record_k1_wait_release = 1;
        }
    }
    else
    {
        if(portion2_record_k1_start_ms != 0 && !portion2_record_k1_wait_release) k1_short = 1;
        portion2_record_k1_start_ms = 0; portion2_record_k1_wait_release = 0;
    }

    if(gpio_get_level(KEY2) == 0)
    {
        if(portion2_record_k2_start_ms == 0) portion2_record_k2_start_ms = now_ms;
        if(!portion2_record_k2_wait_release && (uint32)(now_ms - portion2_record_k2_start_ms) > 1500U)
        {
            k2_long = 1; portion2_record_k2_wait_release = 1;
        }
    }
    else
    {
        if(portion2_record_k2_start_ms != 0 && !portion2_record_k2_wait_release) k2_short = 1;
        portion2_record_k2_start_ms = 0; portion2_record_k2_wait_release = 0;
    }

    if(gpio_get_level(KEY3) == 0)
    {
        if(portion2_record_k3_start_ms == 0) portion2_record_k3_start_ms = now_ms;
        if(!portion2_record_k3_wait_release && (uint32)(now_ms - portion2_record_k3_start_ms) > 1500U)
        {
            k3_long = 1; portion2_record_k3_wait_release = 1;
        }
    }
    else
    {
        if(portion2_record_k3_start_ms != 0 && !portion2_record_k3_wait_release) k3_short = 1;
        portion2_record_k3_start_ms = 0; portion2_record_k3_wait_release = 0;
    }

    if(gpio_get_level(KEY4) == 0)
    {
        if(portion2_record_k4_start_ms == 0) portion2_record_k4_start_ms = now_ms;
        if(!portion2_record_k4_wait_release && (uint32)(now_ms - portion2_record_k4_start_ms) > 1500U)
        {
            k4_long = 1; portion2_record_k4_wait_release = 1;
        }
    }
    else
    {
        if(portion2_record_k4_start_ms != 0 && !portion2_record_k4_wait_release) k4_short = 1;
        portion2_record_k4_start_ms = 0; portion2_record_k4_wait_release = 0;
    }

    if(k1_long)
    {
        portion2_record_start_pending = 0;
        portion2_route_length[portion2_record_route] = 0;
        portion2_route_gps_count[portion2_record_route] = 0;
        portion2_route_start_yaw[portion2_record_route] = 0.0f;
        portion2_route_final_yaw[portion2_record_route] = 0.0f;
        portion2_gps_auto_has_point[portion2_record_route] = 0;
        portion2_route_saved_flag[portion2_record_route] = 0;
        portion2_serial_log_record_event("CLEAR");
        Buzzer_check(80);
    }
    if(k2_long)
    {
        Flash_Write_passage_points();
        portion2_route_saved_flag[portion2_record_route] = (portion2_route_length[portion2_record_route] > 0) ? 1 : 0;
        portion2_serial_log_record_event("SAVE");
        Buzzer_check(150);
    }
    if(k3_long)
    {
        portion2_record_point();
        portion2_auto_record_gps_point();
        Buzzer_check(30);
    }
    if(k4_long)
    {
        portion2_record_start_pending = 0;
        portion2_mode_key_transition_lock();
        portion2_record_key_state_reset();
        out_v_l = 0.0f;
        out_v_r = 0.0f;
        out_servo = 0.0f;
        daoche_flag = 0;
        main_mode = Guandao_Drive;
        route_setting_choice = 1;
        conrtol_mode = GUANDAO;
        Buzzer_check(80);
        ips200_clear();
        return;
    }

    if(k1_short && portion2_record_state != 1)
    {
        portion2_record_start_pending = 0;
        if(portion2_record_route > 0) portion2_record_route--;
        portion2_serial_log_record_event("SELECT");
        Buzzer_check(20);
    }
    if(k2_short && portion2_record_state != 1)
    {
        portion2_record_start_pending = 0;
        if(portion2_record_route + 1 < PORTION2_ROUTE_COUNT) portion2_record_route++;
        portion2_serial_log_record_event("SELECT");
        Buzzer_check(20);
    }
    if(k4_short)
    {
        portion2_record_start_pending = 0;
        portion2_mode_key_transition_lock();
        portion2_record_key_state_reset();
        daoche_flag = 0;
        main_mode = Guandao_Voice;
        route_setting_choice = 3;
        conrtol_mode = GUANDAO;
        Buzzer_check(50);
        ips200_clear();
        return;
    }

    switch(portion2_record_state)
    {
        case 0:
        case 2:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            if(k3_short)
            {
                portion2_record_start_pending = 1;
            }
            if(portion2_record_start_pending)
            {
                if(portion2_gps_origin_ready())
                {
                    portion2_record_begin_route();
                }
                else
                {
                    portion2_serial_log_gps_reject();
                }
            }
            break;
        case 1:
            update_state(&passage, &guandao_ecd);
            portion2_record_point();
            portion2_auto_record_gps_point();
            if(k3_short)
            {
                portion2_record_try_gps_point(1);
                portion2_record_capture_final_pose();
                portion2_record_state = 2;
                daoche_flag = 0;
                portion2_serial_log_record_event("STOP");
                Buzzer_check(50);
            }
            if(key2_flag)
            {
                key2_flag = 0;
                portion2_record_try_gps_point(1);
                portion2_record_capture_final_pose();
                if(portion2_route_gps_count[portion2_record_route] >= portion2_route_required_gps(portion2_record_route))
                {
                    if(portion2_record_route + 1 < PORTION2_ROUTE_COUNT)
                    {
                        portion2_record_route++;
                    }
                    portion2_record_state = 2;
                    daoche_flag = 0;
                    portion2_serial_log_record_event("NEXT");
                    Buzzer_check(100);
                }
                else
                {
                    Buzzer_check(20);
                }
            }
            break;
        default:
            portion2_record_reset();
            daoche_flag = 0;
            break;
    }

    portion2_serial_log_record_status();

    ips200_show_string(X(1), Y(0), "MODE: RECORD");
    ips200_show_string(X(1), Y(1), "ROUTE: ");
    ips200_show_int(X(9), Y(1), portion2_record_route + 1, 2);
    ips200_show_string(X(1), Y(2), "STATE: ");
    switch(portion2_record_state)
    {
        case 0: ips200_show_string(X(9), Y(2), "IDLE "); break;
        case 1: ips200_show_string(X(9), Y(2), "RECORD"); break;
        case 2: ips200_show_string(X(9), Y(2), "WAIT "); break;
        default: ips200_show_string(X(9), Y(2), "--   "); break;
    }
    {
        uint8 route_id = portion2_record_route < PORTION2_ROUTE_COUNT ? portion2_record_route : PORTION2_ROUTE_COUNT - 1;
        ips200_show_string(X(1), Y(3), "RAW");
        ips200_show_int(X(6), Y(3), portion2_route_length[route_id], 2);
        ips200_show_string(X(9), Y(3), "/");
        ips200_show_int(X(11), Y(3), PORTION2_ROUTE_MAX_POINTS, 2);
        ips200_show_string(X(1), Y(4), "GPS");
        ips200_show_int(X(6), Y(4), portion2_route_gps_count[route_id], 2);
        ips200_show_string(X(9), Y(4), "/");
        ips200_show_int(X(11), Y(4), PORTION2_GPS_PER_ROUTE, 2);
        ips200_show_string(X(1), Y(5), "DIST");
        ips200_show_float(X(7), Y(5), portion2_recorded_route_distance(route_id), 6, 2);
        ips200_show_string(X(14), Y(5), "m");
        ips200_show_string(X(1), Y(6), "STEP");
        ips200_show_float(X(7), Y(6), recode_threshold, 5, 2);
        ips200_show_string(X(13), Y(6), "m");
        ips200_show_string(X(1), Y(7), "SAVED: ");
        ips200_show_string(X(9), Y(7), portion2_route_saved_flag[route_id] ? "YES" : "NO ");
        ips200_show_string(X(1), Y(8), "FIX");
        ips200_show_int(X(6), Y(8), gnss.state, 1);
        ips200_show_string(X(9), Y(8), "SAT");
        ips200_show_int(X(13), Y(8), gnss.satellite_used, 2);
        ips200_show_string(X(1), Y(9), "COORD");
        ips200_show_string(X(8), Y(9), portion2_gps_coordinate_valid() ? "YES" : "NO ");
        ips200_show_string(X(1), Y(10), "GSEQ");
        ips200_show_int(X(7), Y(10), (int32)portion2_gps_get_fix_sequence(), 8);
        ips200_show_string(X(1), Y(11), "GREASON");
        ips200_show_string(X(9), Y(11), "          ");
        ips200_show_string(X(9), Y(11), portion2_gps_reject_reason_text());
    }

    ips200_show_string(X(1),  Y(12), "K1:-/CLR");
    ips200_show_string(X(1),  Y(13), "K2:+/SAVE");
    ips200_show_string(X(1),  Y(14), "K3:REC/PT");
    ips200_show_string(X(1),  Y(15), "K4:RUN");
}

void portion2_run_select_route(uint8 route_id)
{
    subject_2_gyro_route_stop("RECORDED_ROUTE");
    portion2_run_reject_reason = 0;
    portion2_run_reverse = 0;
    portion2_run_drive_reverse = 0;
    daoche_flag = 0;
    conrtol_mode = GUANDAO;
    if(route_id >= PORTION2_ROUTE_COUNT)
    {
        portion2_run_reject_reason = 1;
        Buzzer_check(80); Buzzer_check(80);
        return;
    }
    if(!portion2_route_ready_for_run(route_id))
    {
        portion2_run_reject_reason = 4;
        Buzzer_check(80); Buzzer_check(80);
        return;
    }
    portion2_selected_route = route_id;
    portion2_run_drive_reverse = portion2_route_uses_reverse_drive(route_id);
    daoche_flag = portion2_run_drive_reverse;
    conrtol_mode = portion2_run_drive_reverse ? DAOCHE : GUANDAO;
    portion2_state_flag = 1;
}

void portion2_run_select_reverse_route(uint8 route_id)
{
    subject_2_gyro_route_stop("RECORDED_ROUTE");
    portion2_run_reject_reason = 0;
    portion2_run_reverse = 0;
    portion2_run_drive_reverse = 0;
    daoche_flag = 0;
    conrtol_mode = GUANDAO;
    if(route_id > PORTION2_ROUTE_5)
    {
        portion2_run_reject_reason = 3;
        Buzzer_check(80); Buzzer_check(80);
        return;
    }
    if(!portion2_route_ready_for_run(route_id))
    {
        portion2_run_reject_reason = 4;
        Buzzer_check(80); Buzzer_check(80);
        return;
    }
    portion2_selected_route = route_id;
    portion2_run_reverse = 1;
    portion2_state_flag = 1;
}

void portion2_run_select_back_route(uint8 route_id)
{
    if(route_id != PORTION2_ROUTE_STRAIGHT && route_id != PORTION2_ROUTE_SNAKE)
    {
        portion2_run_reject_reason = 3;
        Buzzer_check(80); Buzzer_check(80);
        return;
    }
    portion2_run_select_route(route_id);
}

void portion2_run_stop(void)
{
    portion2_serial_log_track_summary();
    portion2_state_flag = 0;
    portion2_run_reverse = 0;
    portion2_run_drive_reverse = 0;
    portion2_final_yaw_align_start_ms = 0;
    portion2_run_final_yaw = 0.0f;
    portion2_route12_overshoot_reset();
    portion2_final_zone_reset();
    daoche_flag = 0;
    portion2_gps_fusion_reset();
    conrtol_mode = GUANDAO;
    out_v_l = 0;
    out_v_r = 0;
    out_servo = 0;
}

void portion2_run_task(void)
{
    uint16 offset;
    uint16 len;
    uint8 gps_prepare_ready;
    uint8 gps_startup_result;
    float run_start_theta;
    float yaw_delta;
    float recorded_terminal_yaw;

    switch(portion2_state_flag)
    {
        case 0:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            break;
        case 1:
            portion2_final_yaw_align_start_ms = 0;
            portion2_route12_overshoot_reset();
            portion2_final_zone_reset();
            portion2_track_reset();
            portion2_clear_route();
            Encoder_Get(&guandao_ecd);
            daoche_flag = portion2_run_drive_reverse;
            conrtol_mode = portion2_run_drive_reverse ? DAOCHE : GUANDAO;
            offset = portion2_route_offset(portion2_selected_route);
            len = portion2_route_length[portion2_selected_route];
            if(len > PORTION2_ROUTE_MAX_POINTS) len = PORTION2_ROUTE_MAX_POINTS;
            for(uint16 i = 0; i < len && i < MAX_LENGTH_INDEX; i++)
            {
                if(portion2_run_reverse)
                {
                    portion_2.recode_map[i] = portion2_route_storage_get(offset + len - 1 - i);
                }
                else
                {
                    portion_2.recode_map[i] = portion2_route_storage_get(offset + i);
                }
            }
            portion_2.length_index = len;
            portion_2.gps_recode_length = portion2_route_gps_count[portion2_selected_route];
            if(portion_2.gps_recode_length > PORTION2_GPS_PER_ROUTE)
            {
                portion_2.gps_recode_length = PORTION2_GPS_PER_ROUTE;
            }
            for(uint8 i = 0; i < portion_2.gps_recode_length; i++)
            {
                uint16 gps_offset = portion2_route_gps_offset(portion2_selected_route);
                if(portion2_run_reverse)
                {
                    GPS_state gps_point = portion2_gps_storage_get(gps_offset + portion_2.gps_recode_length - 1 - i);
                    if(gps_point.cheak_flag < 0 || gps_point.cheak_flag >= len)
                    {
                        gps_point.cheak_flag = 0;
                    }
                    else
                    {
                        gps_point.cheak_flag = len - 1 - gps_point.cheak_flag;
                    }
                    portion_2.recode_gpsmap[i] = gps_point;
                }
                else
                {
                    portion_2.recode_gpsmap[i] = portion2_gps_storage_get(gps_offset + i);
                }
            }
            run_start_theta = portion2_run_drive_reverse ? Yaw_1 + 180.0f : Yaw_1;
            portion2_translate_route_to_origin();
            yaw_delta = portion2_align_route_to_current_yaw(run_start_theta);
            portion2_capture_raw_transform(yaw_delta);
            gps_prepare_ready = 0;
            if(portion2_route_uses_gps(portion2_selected_route))
            {
                gps_prepare_ready = portion2_gps_fusion_prepare(&portion_2);
                if(!gps_prepare_ready)
                {
                    portion2_run_reject_reason = 5;
                    portion2_state_flag = 0;
                    portion2_run_reverse = 0;
                    portion2_run_drive_reverse = 0;
                    daoche_flag = 0;
                    conrtol_mode = GUANDAO;
                    out_v_l = 0;
                    out_v_r = 0;
                    out_servo = 0;
                    portion2_gps_fusion_reset();
                    Buzzer_check(80); Buzzer_check(80);
                    uart_write_string(DEBUG_UART_INDEX, "[P2-RUN-REJECT] reason=GPS_FIT\r\n");
                    break;
                }
            }
            else
            {
                portion2_gps_fusion_reset();
            }
            portion2_smooth_reference_route();
            recorded_terminal_yaw = portion2_run_reverse
                    ? portion2_route_start_yaw[portion2_selected_route]
                    : portion2_route_final_yaw[portion2_selected_route];
            portion2_run_final_yaw = guandao_normalize_angle(recorded_terminal_yaw + yaw_delta);
            guandao_build_smooth_plan(&portion_2);
            for(uint8 i = 0; i < portion_2.gps_recode_length; i++)
            {
                GPS_state gps_point = portion_2.recode_gpsmap[i];
                gps_point.cheak_flag = portion2_plan_index_from_raw_point(gps_point.cheak_flag, (int16)len, guandao_route_length(&portion_2));
                portion_2.recode_gpsmap[i] = gps_point;
            }
            portion2_run_last_report_point = -1;
            portion2_run_last_report_gps = 255;
            portion2_state_flag = gps_prepare_ready ? 4 : 2;
            portion2_serial_log_run_point_event(1);
            portion2_serial_log_run_gps_event(1);
            break;
        case 4:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            gps_startup_result = portion2_gps_fusion_startup_update(&portion_2);
            if(gps_startup_result == PORTION2_GPS_STARTUP_READY ||
               gps_startup_result == PORTION2_GPS_STARTUP_FALLBACK)
            {
                portion2_state_flag = 2;
            }
            break;
        case 2:
            guandao_trace_direct(&portion_2);
            portion2_serial_log_run_point_event(0);
            portion2_serial_log_run_gps_event(0);
            portion2_serial_log_run();
            if(portion_2.current_point_index >= guandao_route_length(&portion_2))
            {
                out_v_l = 0;
                out_v_r = 0;
                out_servo = 0;
                portion2_run_reverse = 0;
                portion2_run_drive_reverse = 0;
                portion2_final_yaw_align_start_ms = 0;
                portion2_run_final_yaw = 0.0f;
                portion2_route12_overshoot_reset();
                portion2_final_zone_reset();
                daoche_flag = 0;
                conrtol_mode = GUANDAO;
                portion2_state_flag = 3;
            }
            break;
        case 3:
            out_v_l = 0;
            out_v_r = 0;
            out_servo = 0;
            portion2_gps_fusion_reset();
            portion2_state_flag = 0;
            portion2_run_reverse = 0;
            portion2_run_drive_reverse = 0;
            portion2_final_yaw_align_start_ms = 0;
            portion2_run_final_yaw = 0.0f;
            portion2_route12_overshoot_reset();
            portion2_final_zone_reset();
            daoche_flag = 0;
            conrtol_mode = GUANDAO;
            break;
        default:
            portion2_reset();
            break;
    }

    {
        static uint32 p2_last_ms = 0;
        uint32 p2_now = system_getval_ms();
        if(p2_now - p2_last_ms >= 100)
        {
            int16 plan_total = guandao_route_length(&portion_2);
            int16 plan_index = guandao_clamp_length(portion_2.current_point_index);
            int16 raw_total = guandao_clamp_length(portion_2.length_index);
            int16 raw_index = portion2_raw_point_from_plan_index(plan_index);
            int16 raw_number = portion2_human_point_number(raw_index, raw_total);
            int16 plan_number = portion2_human_point_number(plan_index, plan_total);
            uint8 gps_done = portion2_run_gps_reached_count();

            p2_last_ms = p2_now;
            ips200_show_string(X(1), Y(8), "P2RUN");
            ips200_show_string(X(9), Y(8), "R");
            ips200_show_int(X(11), Y(8), portion2_selected_route + 1, 2);
            ips200_show_string(X(15), Y(8), "S");
            ips200_show_int(X(17), Y(8), portion2_state_flag, 2);
            ips200_show_string(X(1), Y(9), "RAW");
            ips200_show_int(X(6), Y(9), raw_number, 3);
            ips200_show_string(X(10), Y(9), "/");
            ips200_show_int(X(12), Y(9), raw_total, 3);
            ips200_show_string(X(1), Y(10), "PLAN");
            ips200_show_int(X(6), Y(10), plan_number, 4);
            ips200_show_string(X(11), Y(10), "/");
            ips200_show_int(X(13), Y(10), plan_total, 4);
            ips200_show_string(X(1), Y(11), "GPS");
            ips200_show_int(X(6), Y(11), gps_done, 2);
            ips200_show_string(X(9), Y(11), "/");
            ips200_show_int(X(11), Y(11), portion_2.gps_recode_length, 2);
            ips200_show_string(X(1), Y(12), "GF");
            ips200_show_string(X(4), Y(12), portion2_gps_fusion_is_ready() ? "ON " : "OFF");
            ips200_show_string(X(8), Y(12), "V");
            ips200_show_int(X(10), Y(12), portion2_gps_fusion_last_valid(), 1);
            ips200_show_string(X(13), Y(12), "SAT");
            ips200_show_int(X(17), Y(12), portion2_gps_fusion_get_satellites(), 2);
            ips200_show_string(X(1), Y(13), "RX");
            ips200_show_int(X(5), Y(13), portion2_run_last_rx, 3);
            ips200_show_string(X(10), Y(13), "Cnt");
            ips200_show_int(X(15), Y(13), portion2_run_rx_count, 4);
            ips200_show_string(X(1), Y(14), "REJ");
            ips200_show_int(X(6), Y(14), portion2_run_reject_reason, 2);
            ips200_show_string(X(11), Y(14), "REV");
            ips200_show_int(X(16), Y(14), portion2_run_drive_reverse ? 2 : portion2_run_reverse, 1);
            ips200_show_string(X(1), Y(15), "GERR");
            ips200_show_float(X(7), Y(15), portion2_gps_fusion_get_error(), 6, 2);
            ips200_show_string(X(14), Y(15), "m");
        }
    }
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
