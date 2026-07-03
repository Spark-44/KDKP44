

#include "zf_common_headfile.h"
#include <stdio.h>

GPS_work gps_work;
Lost_Point lost_judge;

#define PORTION2_GPS_FUSION_MIN_SATELLITES    (6U)
#define PORTION2_GPS_FUSION_MAX_HDOP          (3.0f)
#define PORTION2_GPS_FUSION_MIN_ANCHORS       (4U)
#define PORTION2_GPS_FUSION_MIN_INLIERS       (8U)
#define PORTION2_GPS_FUSION_MAX_ANCHOR_RESIDUAL (1.5f)
#define PORTION2_GPS_FUSION_REPEAT_DISTANCE   (0.05f)
#define PORTION2_GPS_FUSION_MAX_ERROR         (3.0f)
#define PORTION2_GPS_FUSION_GAIN              (0.10f)
#define PORTION2_GPS_FUSION_MAX_CORRECTION    (0.10f)
#define PORTION2_GPS_FUSION_MIN_SCALE         (0.40f)
#define PORTION2_GPS_FUSION_MAX_SCALE         (2.50f)
#define PORTION2_GPS_FUSION_MAX_RMS_ERROR     (1.20f)
#define PORTION2_GPS_STARTUP_REQUIRED_SAMPLES (3U)
#define PORTION2_GPS_STARTUP_TIMEOUT_MS       (10000U)
#define PORTION2_GPS_STARTUP_STABILITY_M      (1.2f)
#define PORTION2_GPS_STARTUP_MAX_SHIFT_M      (5.0f)
#define PORTION2_GPS_FUSION_MAX_LARGE_ERRORS  (3U)
#define PORTION2_GPS_RECOVERY_REQUIRED_FIXES  (3U)
#define PORTION2_GPS_RECOVERY_MAX_ERROR       (2.0f)
#define PORTION2_GPS_METERS_PER_DEGREE        (111320.0f)

typedef struct
{
    uint8 ready;
    uint8 last_valid;
    uint8 satellites;
    uint8 has_last_coordinate;
    uint8 last_reason;
    uint8 startup_active;
    uint8 startup_sample_count;
    uint8 startup_has_last_coordinate;
    uint8 large_error_count;
    uint8 recovering;
    uint8 recovery_good_count;
    uint8 anchor_count;
    uint8 inlier_count;
    uint32 last_sequence;
    uint32 last_log_ms;
    uint32 startup_start_ms;
    double origin_lat;
    double origin_lon;
    double last_lat;
    double last_lon;
    double startup_last_lat;
    double startup_last_lon;
    float longitude_scale;
    float transform_a;
    float transform_b;
    float transform_tx;
    float transform_ty;
    float gps_x;
    float gps_y;
    float error;
    float correction;
    float startup_sum_x;
    float startup_sum_y;
    float startup_shift;
} portion2_gps_fusion_state_t;

static volatile uint32 portion2_gps_fix_sequence = 0;
static volatile uint32 portion2_gps_rmc_sequence = 0;
static portion2_gps_fusion_state_t portion2_gps_fusion;

uint32 portion2_gps_get_fix_sequence(void)
{
    return portion2_gps_fix_sequence;
}

uint32 portion2_gps_get_rmc_sequence(void)
{
    return portion2_gps_rmc_sequence;
}

void portion2_gps_note_parsed_update(uint8 parse_result)
{
    portion2_gps_fix_sequence++;
    if(parse_result & GNSS_PARSE_RMC_OK)
    {
        portion2_gps_rmc_sequence++;
    }
}

void gps_serial_diagnostic_task(void)
{
    static uint32 last_report_ms = 0;
    uint32 now_ms = system_getval_ms();
    gnss_diagnostic_struct diagnostics;
    char line[384];

    if((uint32)(now_ms - last_report_ms) < 1000U) return;
    last_report_ms = now_ms;
    gnss_get_diagnostics(&diagnostics);

    sprintf(line,
            "[GNSS-DIAG] bytes=%lu lines=%lu rmc=%lu/%lu gga=%lu/%lu ths=%lu/%lu other=%lu parseErr=%lu frameErr=%lu last=%u result=0x%02X fix=%u sats=%u hdop100=%u lat7=%ld lon7=%ld\r\n",
            (unsigned long)diagnostics.rx_byte_count,
            (unsigned long)diagnostics.line_count,
            (unsigned long)diagnostics.rmc_ok,
            (unsigned long)diagnostics.rmc_received,
            (unsigned long)diagnostics.gga_ok,
            (unsigned long)diagnostics.gga_received,
            (unsigned long)diagnostics.ths_ok,
            (unsigned long)diagnostics.ths_received,
            (unsigned long)diagnostics.other_received,
            (unsigned long)diagnostics.parse_error_count,
            (unsigned long)diagnostics.frame_error_count,
            (unsigned)diagnostics.last_sentence,
            (unsigned)diagnostics.last_parse_result,
            (unsigned)gnss.state,
            (unsigned)gnss.satellite_used,
            (unsigned)(gnss.hdop * 100.0f + 0.5f),
            (long)(gnss.latitude * 10000000.0),
            (long)(gnss.longitude * 10000000.0));
    uart_write_string(DEBUG_UART_INDEX, line);
}

static long portion2_gps_fixed100(float value)
{
    if(value >= 0.0f) return (long)(value * 100.0f + 0.5f);
    return (long)(value * 100.0f - 0.5f);
}

static void portion2_gps_append_fixed100(char *line, int *pos, float value)
{
    long scaled = portion2_gps_fixed100(value);
    if(scaled < 0)
    {
        line[(*pos)++] = '-';
        scaled = -scaled;
    }
    *pos += sprintf(&line[*pos], "%ld.%02ld", scaled / 100, scaled % 100);
}

static void portion2_gps_to_local_meters(double lat, double lon, float *east, float *north)
{
    *east = (float)((lon - portion2_gps_fusion.origin_lon) * (double)portion2_gps_fusion.longitude_scale);
    *north = (float)((lat - portion2_gps_fusion.origin_lat) * (double)PORTION2_GPS_METERS_PER_DEGREE);
}

static void portion2_gps_fusion_log(guandao_state *state)
{
    uint32 now_ms = system_getval_ms();
    char line[240];
    int pos = 0;

    if((uint32)(now_ms - portion2_gps_fusion.last_log_ms) < 200U) return;
    portion2_gps_fusion.last_log_ms = now_ms;
    pos += sprintf(line,
                   "[P2-GPS-FUSION] route=%u ready=%u valid=%u reason=%u sats=%u gx=",
                   (unsigned)(portion2_selected_route + 1),
                   (unsigned)portion2_gps_fusion.ready,
                   (unsigned)portion2_gps_fusion.last_valid,
                   (unsigned)portion2_gps_fusion.last_reason,
                   (unsigned)portion2_gps_fusion.satellites);
    portion2_gps_append_fixed100(line, &pos, portion2_gps_fusion.gps_x);
    pos += sprintf(&line[pos], " gy=");
    portion2_gps_append_fixed100(line, &pos, portion2_gps_fusion.gps_y);
    pos += sprintf(&line[pos], " ix=");
    portion2_gps_append_fixed100(line, &pos, state->current_state.x);
    pos += sprintf(&line[pos], " iy=");
    portion2_gps_append_fixed100(line, &pos, state->current_state.y);
    pos += sprintf(&line[pos], " err=");
    portion2_gps_append_fixed100(line, &pos, portion2_gps_fusion.error);
    pos += sprintf(&line[pos], " corr=");
    portion2_gps_append_fixed100(line, &pos, portion2_gps_fusion.correction);
    pos += sprintf(&line[pos], " cal=%u/%u large=%u hdop100=%u recover=%u/%u",
                   (unsigned)portion2_gps_fusion.startup_sample_count,
                   (unsigned)PORTION2_GPS_STARTUP_REQUIRED_SAMPLES,
                   (unsigned)portion2_gps_fusion.large_error_count,
                   (unsigned)(gnss.hdop * 100.0f + 0.5f),
                   (unsigned)portion2_gps_fusion.recovery_good_count,
                   (unsigned)PORTION2_GPS_RECOVERY_REQUIRED_FIXES);
    pos += sprintf(&line[pos], " anchors=%u inliers=%u removed=%u", (unsigned)portion2_gps_fusion.anchor_count, (unsigned)portion2_gps_fusion.inlier_count, (unsigned)(portion2_gps_fusion.anchor_count - portion2_gps_fusion.inlier_count));
    pos += sprintf(&line[pos], "\r\n");
    uart_write_string(DEBUG_UART_INDEX, line);
}

static uint8 portion2_gps_fusion_prepare_fail(guandao_state *state, uint8 reason)
{
    portion2_gps_fusion.last_reason = reason;
    if(state != 0) portion2_gps_fusion_log(state);
    return 0;
}

void portion2_gps_fusion_reset(void)
{
    portion2_gps_fusion.ready = 0;
    portion2_gps_fusion.last_valid = 0;
    portion2_gps_fusion.satellites = 0;
    portion2_gps_fusion.has_last_coordinate = 0;
    portion2_gps_fusion.last_reason = 1;
    portion2_gps_fusion.startup_active = 0;
    portion2_gps_fusion.startup_sample_count = 0;
    portion2_gps_fusion.startup_has_last_coordinate = 0;
    portion2_gps_fusion.large_error_count = 0;
    portion2_gps_fusion.recovering = 0;
    portion2_gps_fusion.recovery_good_count = 0;
    portion2_gps_fusion.last_sequence = portion2_gps_rmc_sequence;
    portion2_gps_fusion.last_log_ms = 0;
    portion2_gps_fusion.startup_start_ms = 0;
    portion2_gps_fusion.origin_lat = 0.0;
    portion2_gps_fusion.origin_lon = 0.0;
    portion2_gps_fusion.last_lat = 0.0;
    portion2_gps_fusion.last_lon = 0.0;
    portion2_gps_fusion.startup_last_lat = 0.0;
    portion2_gps_fusion.startup_last_lon = 0.0;
    portion2_gps_fusion.longitude_scale = 0.0f;
    portion2_gps_fusion.transform_a = 0.0f;
    portion2_gps_fusion.transform_b = 0.0f;
    portion2_gps_fusion.transform_tx = 0.0f;
    portion2_gps_fusion.transform_ty = 0.0f;
    portion2_gps_fusion.gps_x = 0.0f;
    portion2_gps_fusion.gps_y = 0.0f;
    portion2_gps_fusion.error = 0.0f;
    portion2_gps_fusion.correction = 0.0f;
    portion2_gps_fusion.startup_sum_x = 0.0f;
    portion2_gps_fusion.startup_sum_y = 0.0f;
    portion2_gps_fusion.startup_shift = 0.0f;
    portion2_gps_fusion.anchor_count = 0;
    portion2_gps_fusion.inlier_count = 0;
}

static uint8 portion2_gps_fit_transform(const float *gps_east, const float *gps_north,
        const float *route_x, const float *route_y, const uint8 *included,
        uint8 anchor_count, float *transform_a, float *transform_b,
        float *transform_tx, float *transform_ty, float *rms_error)
{
    float mean_east = 0.0f;
    float mean_north = 0.0f;
    float mean_x = 0.0f;
    float mean_y = 0.0f;
    float denominator = 0.0f;
    float numerator_a = 0.0f;
    float numerator_b = 0.0f;
    float residual_sum = 0.0f;
    float scale;
    uint8 included_count = 0;

    for(uint8 i = 0; i < anchor_count; i++)
    {
        if(!included[i]) continue;
        mean_east += gps_east[i];
        mean_north += gps_north[i];
        mean_x += route_x[i];
        mean_y += route_y[i];
        included_count++;
    }
    if(included_count < 2U) return 12;
    mean_east /= (float)included_count;
    mean_north /= (float)included_count;
    mean_x /= (float)included_count;
    mean_y /= (float)included_count;

    for(uint8 i = 0; i < anchor_count; i++)
    {
        float ge;
        float gn;
        float rx;
        float ry;
        if(!included[i]) continue;
        ge = gps_east[i] - mean_east;
        gn = gps_north[i] - mean_north;
        rx = route_x[i] - mean_x;
        ry = route_y[i] - mean_y;
        denominator += ge * ge + gn * gn;
        numerator_a += ge * rx + gn * ry;
        numerator_b += ge * ry - gn * rx;
    }
    if(denominator < 4.0f) return 12;

    *transform_a = numerator_a / denominator;
    *transform_b = numerator_b / denominator;
    scale = hypotf(*transform_a, *transform_b);
    if(scale < PORTION2_GPS_FUSION_MIN_SCALE || scale > PORTION2_GPS_FUSION_MAX_SCALE)
    {
        return 13;
    }
    *transform_tx = mean_x - *transform_a * mean_east + *transform_b * mean_north;
    *transform_ty = mean_y - *transform_b * mean_east - *transform_a * mean_north;

    for(uint8 i = 0; i < anchor_count; i++)
    {
        float fitted_x;
        float fitted_y;
        float dx;
        float dy;
        if(!included[i]) continue;
        fitted_x = *transform_a * gps_east[i] - *transform_b * gps_north[i] + *transform_tx;
        fitted_y = *transform_b * gps_east[i] + *transform_a * gps_north[i] + *transform_ty;
        dx = fitted_x - route_x[i];
        dy = fitted_y - route_y[i];
        residual_sum += dx * dx + dy * dy;
    }
    *rms_error = sqrtf(residual_sum / (float)included_count);
    return 0;
}

uint8 portion2_gps_fusion_prepare(guandao_state *state)
{
    float gps_east[PORTION2_GPS_PER_ROUTE];
    float gps_north[PORTION2_GPS_PER_ROUTE];
    float route_x[PORTION2_GPS_PER_ROUTE];
    float route_y[PORTION2_GPS_PER_ROUTE];
    uint8 inlier_mask[PORTION2_GPS_PER_ROUTE];
    float transform_a;
    float transform_b;
    float transform_tx;
    float transform_ty;
    float rms_error;
    uint8 anchor_count = 0;
    uint8 inlier_count = 0;
    uint8 fit_result;
    int16 raw_length;

    portion2_gps_fusion_reset();
    if(state == 0) return 0;
    raw_length = state->length_index;
    if(raw_length <= 1 || state->gps_recode_length < PORTION2_GPS_FUSION_MIN_ANCHORS)
    {
        return portion2_gps_fusion_prepare_fail(state, 10);
    }

    for(int16 i = 0; i < state->gps_recode_length && i < PORTION2_GPS_PER_ROUTE; i++)
    {
        GPS_state gps_point = state->recode_gpsmap[i];
        int16 raw_index = gps_point.cheak_flag;
        float east;
        float north;
        uint8 duplicate = 0;

        if(gps_point.lat == 0.0 || gps_point.lon == 0.0) continue;
        if(raw_index < 0 || raw_index >= raw_length) continue;
        if(anchor_count == 0)
        {
            portion2_gps_fusion.origin_lat = gps_point.lat;
            portion2_gps_fusion.origin_lon = gps_point.lon;
            portion2_gps_fusion.longitude_scale = PORTION2_GPS_METERS_PER_DEGREE
                    * cosf((float)gps_point.lat / 180.0f * M_PI);
        }
        portion2_gps_to_local_meters(gps_point.lat, gps_point.lon, &east, &north);
        for(uint8 j = 0; j < anchor_count; j++)
        {
            if(hypotf(east - gps_east[j], north - gps_north[j]) < 0.20f)
            {
                duplicate = 1;
                break;
            }
        }
        if(duplicate) continue;

        gps_east[anchor_count] = east;
        gps_north[anchor_count] = north;
        route_x[anchor_count] = state->recode_map[raw_index].x;
        route_y[anchor_count] = state->recode_map[raw_index].y;
        inlier_mask[anchor_count] = 1U;
        anchor_count++;
    }

    if(anchor_count < PORTION2_GPS_FUSION_MIN_ANCHORS)
    {
        return portion2_gps_fusion_prepare_fail(state, 11);
    }
    portion2_gps_fusion.anchor_count = anchor_count;
    fit_result = portion2_gps_fit_transform(gps_east, gps_north, route_x, route_y,
            inlier_mask, anchor_count, &transform_a, &transform_b,
            &transform_tx, &transform_ty, &rms_error);
    if(fit_result != 0U) return portion2_gps_fusion_prepare_fail(state, fit_result);
    for(uint8 i = 0; i < anchor_count; i++)
    {
        float fitted_x = transform_a * gps_east[i] - transform_b * gps_north[i] + transform_tx;
        float fitted_y = transform_b * gps_east[i] + transform_a * gps_north[i] + transform_ty;
        float dx = fitted_x - route_x[i];
        float dy = fitted_y - route_y[i];
        float residual = hypotf(dx, dy);
        inlier_mask[i] = (residual <= PORTION2_GPS_FUSION_MAX_ANCHOR_RESIDUAL) ? 1U : 0U;
        if(inlier_mask[i]) inlier_count++;
    }
    portion2_gps_fusion.inlier_count = inlier_count;
    if(inlier_count < PORTION2_GPS_FUSION_MIN_INLIERS)
    {
        portion2_gps_fusion.error = rms_error;
        return portion2_gps_fusion_prepare_fail(state, 16);
    }
    fit_result = portion2_gps_fit_transform(gps_east, gps_north, route_x, route_y,
            inlier_mask, anchor_count, &transform_a, &transform_b,
            &transform_tx, &transform_ty, &rms_error);
    if(fit_result != 0U) return portion2_gps_fusion_prepare_fail(state, fit_result);
    portion2_gps_fusion.transform_a = transform_a;
    portion2_gps_fusion.transform_b = transform_b;
    portion2_gps_fusion.transform_tx = transform_tx;
    portion2_gps_fusion.transform_ty = transform_ty;
    portion2_gps_fusion.error = rms_error;
    if(rms_error > PORTION2_GPS_FUSION_MAX_RMS_ERROR)
    {
        return portion2_gps_fusion_prepare_fail(state, 14);
    }

    portion2_gps_fusion.ready = 1;
    portion2_gps_fusion.startup_active = 1;
    portion2_gps_fusion.startup_start_ms = system_getval_ms();
    portion2_gps_fusion.last_sequence = portion2_gps_rmc_sequence - 1U;
    portion2_gps_fusion.last_reason = 0;
    portion2_gps_fusion.satellites = gnss.satellite_used;
    portion2_gps_fusion_log(state);
    return 1;
}

uint8 portion2_gps_fusion_startup_update(guandao_state *state)
{
    uint32 now_ms = system_getval_ms();
    uint32 sequence = portion2_gps_rmc_sequence;
    float east;
    float north;
    float sample_x;
    float sample_y;
    float mean_x;
    float mean_y;

    if(state == 0 || !portion2_gps_fusion.ready) return PORTION2_GPS_STARTUP_FALLBACK;
    if(!portion2_gps_fusion.startup_active) return PORTION2_GPS_STARTUP_READY;
    if((uint32)(now_ms - portion2_gps_fusion.startup_start_ms) >= PORTION2_GPS_STARTUP_TIMEOUT_MS)
    {
        portion2_gps_fusion.ready = 0;
        portion2_gps_fusion.last_valid = 0;
        portion2_gps_fusion.startup_active = 0;
        portion2_gps_fusion.last_reason = 15;
        portion2_gps_fusion_log(state);
        return PORTION2_GPS_STARTUP_FALLBACK;
    }
    if(sequence == portion2_gps_fusion.last_sequence) return PORTION2_GPS_STARTUP_WAIT;
    portion2_gps_fusion.last_sequence = sequence;
    portion2_gps_fusion.satellites = gnss.satellite_used;

    if(!gnss.state || gnss.satellite_used < PORTION2_GPS_FUSION_MIN_SATELLITES ||
       gnss.hdop <= 0.0f || gnss.hdop > PORTION2_GPS_FUSION_MAX_HDOP ||
       gnss.latitude == 0.0 || gnss.longitude == 0.0)
    {
        portion2_gps_fusion.last_reason = (!gnss.state) ? 2 :
                ((gnss.satellite_used < PORTION2_GPS_FUSION_MIN_SATELLITES) ? 3 :
                ((gnss.hdop <= 0.0f || gnss.hdop > PORTION2_GPS_FUSION_MAX_HDOP) ? 8 : 4));
        portion2_gps_fusion_log(state);
        return PORTION2_GPS_STARTUP_WAIT;
    }
    if(portion2_gps_fusion.startup_has_last_coordinate)
    {
        float last_east = (float)((gnss.longitude - portion2_gps_fusion.startup_last_lon)
                * (double)portion2_gps_fusion.longitude_scale);
        float last_north = (float)((gnss.latitude - portion2_gps_fusion.startup_last_lat)
                * (double)PORTION2_GPS_METERS_PER_DEGREE);
        if(hypotf(last_east, last_north) < PORTION2_GPS_FUSION_REPEAT_DISTANCE)
        {
            portion2_gps_fusion.last_reason = 5;
            portion2_gps_fusion_log(state);
            return PORTION2_GPS_STARTUP_WAIT;
        }
    }
    portion2_gps_fusion.startup_last_lat = gnss.latitude;
    portion2_gps_fusion.startup_last_lon = gnss.longitude;
    portion2_gps_fusion.startup_has_last_coordinate = 1;

    portion2_gps_to_local_meters(gnss.latitude, gnss.longitude, &east, &north);
    sample_x = portion2_gps_fusion.transform_a * east
            - portion2_gps_fusion.transform_b * north
            + portion2_gps_fusion.transform_tx;
    sample_y = portion2_gps_fusion.transform_b * east
            + portion2_gps_fusion.transform_a * north
            + portion2_gps_fusion.transform_ty;

    if(portion2_gps_fusion.startup_sample_count > 0)
    {
        mean_x = portion2_gps_fusion.startup_sum_x / (float)portion2_gps_fusion.startup_sample_count;
        mean_y = portion2_gps_fusion.startup_sum_y / (float)portion2_gps_fusion.startup_sample_count;
        if(hypotf(sample_x - mean_x, sample_y - mean_y) > PORTION2_GPS_STARTUP_STABILITY_M)
        {
            portion2_gps_fusion.startup_sample_count = 0;
            portion2_gps_fusion.startup_sum_x = 0.0f;
            portion2_gps_fusion.startup_sum_y = 0.0f;
        }
    }
    portion2_gps_fusion.startup_sum_x += sample_x;
    portion2_gps_fusion.startup_sum_y += sample_y;
    portion2_gps_fusion.startup_sample_count++;
    portion2_gps_fusion.gps_x = sample_x;
    portion2_gps_fusion.gps_y = sample_y;
    portion2_gps_fusion.last_reason = 20;
    portion2_gps_fusion_log(state);

    if(portion2_gps_fusion.startup_sample_count < PORTION2_GPS_STARTUP_REQUIRED_SAMPLES)
    {
        return PORTION2_GPS_STARTUP_WAIT;
    }

    mean_x = portion2_gps_fusion.startup_sum_x / (float)portion2_gps_fusion.startup_sample_count;
    mean_y = portion2_gps_fusion.startup_sum_y / (float)portion2_gps_fusion.startup_sample_count;
    portion2_gps_fusion.startup_shift = hypotf(state->current_state.x - mean_x, state->current_state.y - mean_y);
    if(portion2_gps_fusion.startup_shift > PORTION2_GPS_STARTUP_MAX_SHIFT_M)
    {
        portion2_gps_fusion.ready = 0;
        portion2_gps_fusion.recovering = 1;
        portion2_gps_fusion.recovery_good_count = 0;
        portion2_gps_fusion.startup_active = 0;
        portion2_gps_fusion.last_reason = 21;
        portion2_gps_fusion_log(state);
        return PORTION2_GPS_STARTUP_REJECT;
    }
    portion2_gps_fusion.transform_tx += state->current_state.x - mean_x;
    portion2_gps_fusion.transform_ty += state->current_state.y - mean_y;
    portion2_gps_fusion.gps_x = state->current_state.x;
    portion2_gps_fusion.gps_y = state->current_state.y;
    portion2_gps_fusion.error = 0.0f;
    portion2_gps_fusion.correction = 0.0f;
    portion2_gps_fusion.startup_active = 0;
    portion2_gps_fusion.has_last_coordinate = 1;
    portion2_gps_fusion.last_lat = gnss.latitude;
    portion2_gps_fusion.last_lon = gnss.longitude;
    portion2_gps_fusion.last_valid = 1;
    portion2_gps_fusion.last_reason = 0;
    portion2_gps_fusion_log(state);
    return PORTION2_GPS_STARTUP_READY;
}

void portion2_gps_fusion_update(guandao_state *state)
{
    uint32 sequence = portion2_gps_rmc_sequence;
    float east;
    float north;
    float error_x;
    float error_y;
    float correction_x;
    float correction_y;
    float correction_scale;

    if(state == 0 || portion2_gps_fusion.startup_active) return;
    if(!portion2_gps_fusion.ready && !portion2_gps_fusion.recovering) return;
    if(sequence == portion2_gps_fusion.last_sequence) return;
    portion2_gps_fusion.last_sequence = sequence;
    portion2_gps_fusion.last_valid = 0;
    portion2_gps_fusion.correction = 0.0f;
    portion2_gps_fusion.satellites = gnss.satellite_used;

    if(!gnss.state)
    {
        portion2_gps_fusion.recovery_good_count = 0;
        portion2_gps_fusion.last_reason = 2;
        portion2_gps_fusion_log(state);
        return;
    }
    if(gnss.satellite_used < PORTION2_GPS_FUSION_MIN_SATELLITES)
    {
        portion2_gps_fusion.recovery_good_count = 0;
        portion2_gps_fusion.last_reason = 3;
        portion2_gps_fusion_log(state);
        return;
    }
    if(gnss.hdop <= 0.0f || gnss.hdop > PORTION2_GPS_FUSION_MAX_HDOP)
    {
        portion2_gps_fusion.recovery_good_count = 0;
        portion2_gps_fusion.last_reason = 8;
        portion2_gps_fusion_log(state);
        return;
    }
    if(gnss.latitude == 0.0 || gnss.longitude == 0.0)
    {
        portion2_gps_fusion.recovery_good_count = 0;
        portion2_gps_fusion.last_reason = 4;
        portion2_gps_fusion_log(state);
        return;
    }

    if(portion2_gps_fusion.has_last_coordinate)
    {
        float last_east = (float)((gnss.longitude - portion2_gps_fusion.last_lon)
                * (double)portion2_gps_fusion.longitude_scale);
        float last_north = (float)((gnss.latitude - portion2_gps_fusion.last_lat)
                * (double)PORTION2_GPS_METERS_PER_DEGREE);
        if(hypotf(last_east, last_north) < PORTION2_GPS_FUSION_REPEAT_DISTANCE)
        {
            portion2_gps_fusion.last_reason = 5;
            portion2_gps_fusion_log(state);
            return;
        }
    }
    portion2_gps_fusion.last_lat = gnss.latitude;
    portion2_gps_fusion.last_lon = gnss.longitude;
    portion2_gps_fusion.has_last_coordinate = 1;

    portion2_gps_to_local_meters(gnss.latitude, gnss.longitude, &east, &north);
    portion2_gps_fusion.gps_x = portion2_gps_fusion.transform_a * east
            - portion2_gps_fusion.transform_b * north
            + portion2_gps_fusion.transform_tx;
    portion2_gps_fusion.gps_y = portion2_gps_fusion.transform_b * east
            + portion2_gps_fusion.transform_a * north
            + portion2_gps_fusion.transform_ty;
    error_x = portion2_gps_fusion.gps_x - state->current_state.x;
    error_y = portion2_gps_fusion.gps_y - state->current_state.y;
    portion2_gps_fusion.error = hypotf(error_x, error_y);
    if(portion2_gps_fusion.recovering)
    {
        if(portion2_gps_fusion.error <= PORTION2_GPS_RECOVERY_MAX_ERROR)
        {
            portion2_gps_fusion.recovery_good_count++;
            portion2_gps_fusion.last_reason = 22;
            if(portion2_gps_fusion.recovery_good_count >= PORTION2_GPS_RECOVERY_REQUIRED_FIXES)
            {
                portion2_gps_fusion.ready = 1;
                portion2_gps_fusion.recovering = 0;
                portion2_gps_fusion.recovery_good_count = 0;
                portion2_gps_fusion.large_error_count = 0;
                portion2_gps_fusion.last_reason = 0;
            }
        }
        else
        {
            portion2_gps_fusion.recovery_good_count = 0;
            portion2_gps_fusion.last_reason = 23;
        }
        portion2_gps_fusion_log(state);
        return;
    }
    if(portion2_gps_fusion.error > PORTION2_GPS_FUSION_MAX_ERROR)
    {
        portion2_gps_fusion.large_error_count++;
        portion2_gps_fusion.last_reason = 6;
        if(portion2_gps_fusion.large_error_count >= PORTION2_GPS_FUSION_MAX_LARGE_ERRORS)
        {
            portion2_gps_fusion.ready = 0;
            portion2_gps_fusion.recovering = 1;
            portion2_gps_fusion.recovery_good_count = 0;
            portion2_gps_fusion.last_valid = 0;
            portion2_gps_fusion.last_reason = 7;
        }
        portion2_gps_fusion_log(state);
        return;
    }
    portion2_gps_fusion.large_error_count = 0;

    correction_x = error_x * PORTION2_GPS_FUSION_GAIN;
    correction_y = error_y * PORTION2_GPS_FUSION_GAIN;
    portion2_gps_fusion.correction = hypotf(correction_x, correction_y);
    if(portion2_gps_fusion.correction > PORTION2_GPS_FUSION_MAX_CORRECTION)
    {
        correction_scale = PORTION2_GPS_FUSION_MAX_CORRECTION / portion2_gps_fusion.correction;
        correction_x *= correction_scale;
        correction_y *= correction_scale;
        portion2_gps_fusion.correction = PORTION2_GPS_FUSION_MAX_CORRECTION;
    }
    state->current_state.x += correction_x;
    state->current_state.y += correction_y;
    portion2_gps_fusion.last_valid = 1;
    portion2_gps_fusion.last_reason = 0;
    portion2_gps_fusion_log(state);
}

uint8 portion2_gps_fusion_is_ready(void)
{
    return portion2_gps_fusion.ready;
}

uint8 portion2_gps_fusion_last_valid(void)
{
    return portion2_gps_fusion.last_valid;
}

uint8 portion2_gps_fusion_get_satellites(void)
{
    return portion2_gps_fusion.satellites;
}

float portion2_gps_fusion_get_error(void)
{
    return portion2_gps_fusion.error;
}

float portion2_gps_fusion_get_correction(void)
{
    return portion2_gps_fusion.correction;
}

void angle_plan(float * angle)
{
    if(* angle >= 180){* angle -= 360 ;}
    else if(* angle < -180){* angle += 360 ;}

}

void recode_gps(guandao_state * state)
{
    static uint8 rcd_gps_flag = 0 ;
    switch(rcd_gps_flag)
    {
        case 0 :
            state->recode_gpsmap[state->gps_recode_length].lat = gnss.latitude;
            state->recode_gpsmap[state->gps_recode_length].lon = gnss.longitude;
            state->recode_gpsmap[state->gps_recode_length].theta = Yaw_1;
            state->recode_gpsmap[state->gps_recode_length].cheak_flag = state->length_index;
            state->gps_recode_length ++;
            rcd_gps_flag =1;
            break;
        case 1 :
            state->recode_gpsmap[state->gps_recode_length].lat = gnss.latitude;
            state->recode_gpsmap[state->gps_recode_length].lon = gnss.longitude;
            state->recode_gpsmap[state->gps_recode_length].theta =fabs(state->recode_gpsmap[state->gps_recode_length - 1].theta - Yaw_1) ;
            state->recode_gpsmap[state->gps_recode_length].cheak_flag = state->length_index;
            state->gps_recode_length ++;
            rcd_gps_flag =0;
            break;
    }

}

uint8 swtich_gps(void)
{
    static float loop_theta = 0;
    static uint8 loop_theta_flag = 1;
    if(gps_work.distance_gps <= GPS_SWITCH_DISTANCE && gps_work.gps_current_point%2 ==0)
    {
        loop_theta = Yaw_1;
        loop_theta_flag = 0;
        gps_work.gps_current_point++;
        gps_work.azimuth_gps = get_two_points_azimuth(gnss.latitude,   gnss.longitude,  gps_work.points[gps_work.gps_current_point  ].lat , gps_work.points[gps_work.gps_current_point  ].lon);
        angle_plan(&gps_work.azimuth_gps);
        gps_work.angle_gps = gps_work.azimuth_gps - Yaw_1;
        angle_plan(&gps_work.angle_gps);
        gps_work.distance_gps =   get_two_points_distance(gnss.latitude,   gnss.longitude,  gps_work.points[gps_work.gps_current_point  ].lat , gps_work.points[gps_work.gps_current_point  ].lon);

    }
    if(gps_work.distance_gps <= GPS_SWITCH_DISTANCE && gps_work.gps_current_point%2 !=0 &&loop_theta_flag == 1)
    {
        gps_work.gps_current_point++;
        gps_work.azimuth_gps = get_two_points_azimuth(gnss.latitude,   gnss.longitude,  gps_work.points[gps_work.gps_current_point  ].lat , gps_work.points[gps_work.gps_current_point  ].lon);
        angle_plan(&gps_work.azimuth_gps);
        gps_work.angle_gps = gps_work.azimuth_gps - Yaw_1;
        angle_plan(&gps_work.angle_gps);
        gps_work.distance_gps =   get_two_points_distance(gnss.latitude,   gnss.longitude,  gps_work.points[gps_work.gps_current_point  ].lat , gps_work.points[gps_work.gps_current_point  ].lon);
        return 1;
    }
    if(fabs(Yaw_1 - loop_theta) == gps_work.points[gps_work.gps_current_point].theta/2.0f && loop_theta_flag == 0)
    {
        loop_theta_flag = 1;
    }
    return 0 ;

}

void trace_gps(guandao_state * e)
{
    if( gps_work.gps_update_flag == 1)
    {
        gps_work.gps_update_flag = 0;
        if(swtich_gps())
        {
            if(e->recode_gpsmap[gps_work.gps_current_point - 1].cheak_flag == e->current_point_index)  //e->recode_gpsmap[gps_work.gps_current_point - 1].cheak_flag == e->current_point_index + 1
            {
                lost_judge = FIND;
            }
            else
            {
                lost_judge = LOST;
            }
            if(fabs(gps_work.angle_gps) >=90)
            {
                if(gps_work.gps_current_point%2 !=0)
                {
                    lost_judge = LOST;
                }
                gps_work.gps_current_point++;
            }
        }
    }

}

void update_gpsinformation(void)
{
    if(gps_work.gps_current_point >= gps_work.work_gps_length)return;
    gps_work.azimuth_gps = get_two_points_azimuth(gnss.latitude,   gnss.longitude,  gps_work.points[gps_work.gps_current_point  ].lat , gps_work.points[gps_work.gps_current_point  ].lon);
    angle_plan(&gps_work.azimuth_gps);
    gps_work.angle_gps = gps_work.azimuth_gps - Yaw_1;
    angle_plan(&gps_work.angle_gps);
    gps_work.distance_gps =   get_two_points_distance(gnss.latitude,   gnss.longitude,  gps_work.points[gps_work.gps_current_point  ].lat , gps_work.points[gps_work.gps_current_point  ].lon);
    gps_work.gps_update_flag = 1;
}

int32 double_to_int32(double y)
{
    int32 x = 0;
    x = (int32)(y*10000000);
    return x;
}

double int32_to_double(int32 y)
{
    double x = 0;
    x = (double)y * 1.0f / 10000000;
    return x;
}

void GPS_WorkMap_Copy(guandao_state * e)
{
    int choice_flag = 0;
    guandao_state * p = e;
    while(choice_flag < route_setting_choice)
    {
        p = p->next;
        if(p == NULL)return;
        choice_flag++;
    }
    if(p->gps_recode_length%2 != 0)Buzzer_check(500);

    gps_work.work_gps_length = p->gps_recode_length;
    for( int i = 0 ; i <p->gps_recode_length ; i++)gps_work.points[i].theta= 0.0f;
    for( int i = 0 ; i <p->gps_recode_length/2 ; i+=2)gps_work.points[i].theta= p->recode_gpsmap[i].theta;

    for( int i = 0 ; i <p->gps_recode_length ; i++)
    {
        gps_work.points[i].lat = p->recode_gpsmap[i].lat;
        gps_work.points[i].lon = p->recode_gpsmap[i].lon;
        gps_work.points[i].gps_cheak_flag = p->recode_gpsmap[i].cheak_flag;
    }

}

void GPS_Points_Show(guandao_state * e)
{
    double Max_R_Line = -10000.0f , Max_D_Line = -10000.0f , Min_L_Line = 10000.0f , Min_U_Line = 10000.0f;
    double Center_H = 0 ,  Center_W = 0,  INDEX_H = 0,  INDEX_W = 0 ;
    uint16 GD_Show [2][e->gps_recode_length] ;
    if(e->gps_recode_length ==0)return;
    for( int i = 0 ; i< e->gps_recode_length ;i ++)
    {
        if(e ->recode_gpsmap[i].lat < Min_L_Line)Min_L_Line =e ->recode_gpsmap[i].lat  ;
        if(e ->recode_gpsmap[i].lat   > Max_R_Line)Max_R_Line =e ->recode_gpsmap[i].lat  ;
        if(e->recode_gpsmap[i].lon < Min_U_Line)Min_U_Line =e->recode_gpsmap[i].lon;
        if(e->recode_gpsmap[i].lon > Max_D_Line)Max_D_Line =e->recode_gpsmap[i].lon;
    }
    Center_W = (Max_R_Line + Min_L_Line)/2.0f;
    Center_H = (Max_D_Line + Min_U_Line)/2.0f;
    INDEX_W = (Max_R_Line - Min_L_Line);
    INDEX_H = (Max_D_Line - Min_U_Line);

    for(  int i = 0 ; i< e->gps_recode_length ; i ++ )
    {
        GD_Show[0][i] = 110.0f + (e ->recode_gpsmap[i].lat  - Center_W)*(200.0f/INDEX_W);
        GD_Show[1][i] = 150.0f - (e->recode_gpsmap[i].lon - Center_H)*(280.0f/INDEX_H);

    }

    for(int i = 0 ; i< e->gps_recode_length - 1  ; i ++ )
    {
        ips200_draw_point((uint16)GD_Show[0][i],(uint16)GD_Show[1][i],RGB565_WHITE);
        ips200_draw_line((uint16)GD_Show[0][i] ,(uint16)GD_Show[1][i] ,(uint16)GD_Show[0][i+1] ,(uint16)GD_Show[1][i+1] , RGB565_WHITE);
        system_delay_ms(20);
    }

}

void GPS_Work_SHOW(void)
{
    double Max_R_Line = -10000.0f , Max_D_Line = -10000.0f , Min_L_Line = 10000.0f , Min_U_Line = 10000.0f;
    double Center_H = 0 ,  Center_W = 0,  INDEX_H = 0,  INDEX_W = 0 ;
    uint16 GD_Show [2][gps_work.work_gps_length] ;
    if(gps_work.work_gps_length ==0)return;
    for( int i = 0 ; i< gps_work.work_gps_length ;i ++)
    {
        if( gps_work.points[i].lat< Min_L_Line)Min_L_Line =gps_work.points[i].lat  ;
        if(gps_work.points[i].lat   > Max_R_Line)Max_R_Line =gps_work.points[i].lat  ;
        if(gps_work.points[i].lon < Min_U_Line)Min_U_Line =gps_work.points[i].lon;
        if(gps_work.points[i].lon > Max_D_Line)Max_D_Line =gps_work.points[i].lon;
    }
    Center_W = (Max_R_Line + Min_L_Line)/2.0f;
    Center_H = (Max_D_Line + Min_U_Line)/2.0f;
    INDEX_W = (Max_R_Line - Min_L_Line);
    INDEX_H = (Max_D_Line - Min_U_Line);

    for(  int i = 0 ; i< gps_work.work_gps_length ; i ++ )
    {
        GD_Show[0][i] = 110.0f + (gps_work.points[i].lat  - Center_W)*(200.0f/INDEX_W);
        GD_Show[1][i] = 150.0f - (gps_work.points[i].lon - Center_H)*(280.0f/INDEX_H);

    }

    for(int i = 0 ; i<gps_work.work_gps_length - 1  ; i ++ )
    {
        ips200_draw_point((uint16)GD_Show[0][i],(uint16)GD_Show[1][i],RGB565_WHITE);
        ips200_draw_line((uint16)GD_Show[0][i] ,(uint16)GD_Show[1][i] ,(uint16)GD_Show[0][i+1] ,(uint16)GD_Show[1][i+1] , RGB565_WHITE);
        system_delay_ms(50);
    }

}
