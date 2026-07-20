$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$header = Get-Content -Raw (Join-Path $repo 'code/guandao.h')
$source = Get-Content -Raw (Join-Path $repo 'code/guandao.c')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')
$flash = Get-Content -Raw (Join-Path $repo 'code/flash.c')
$fixed = Get-Content -Raw (Join-Path $repo 'code/subject_2_fixed_action.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotContains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -match $Pattern) {
        throw $Message
    }
}

Assert-Contains $header '#define\s+PORTION2_ROUTE_COUNT\s+12' 'route count must be 12'
Assert-Contains $header '#define\s+PORTION2_ROUTE_MAX_POINTS\s+75' 'each route must hold 75 inertial points'
Assert-Contains $header '#define\s+PORTION2_TOTAL_ROUTE_POINTS\s+900' 'inertial capacity must be 12 x 75'
Assert-Contains $header '#define\s+MAX_LENGTH_INDEX\s+588' 'runtime route arrays must retain the 588 point RAM limit'
Assert-Contains $header '#define\s+PORTION2_ROUTE_STORAGE_EXTENSION_POINTS\s+\(PORTION2_TOTAL_ROUTE_POINTS\s*-\s*MAX_LENGTH_INDEX\)' 'route storage must add only the 312 point tail'
Assert-Contains $header '#define\s+PORTION2_GPS_PER_ROUTE\s+30' 'each route must hold 30 GPS points'
Assert-Contains $header '#define\s+PORTION2_TOTAL_GPS_COUNT\s+360' 'GPS capacity must be 12 x 30'
Assert-Contains $header '#define\s+MAX_GPS_RECODE\s+240' 'runtime GPS arrays must retain the 240 record RAM limit'
Assert-Contains $header '#define\s+PORTION2_GPS_STORAGE_EXTENSION_POINTS\s+\(PORTION2_TOTAL_GPS_COUNT\s*-\s*MAX_GPS_RECODE\)' 'GPS storage must add only the 120 record tail'
Assert-Contains $header 'state_t\s+portion2_route_storage_get\s*\(\s*uint16\s+index\s*\)' 'segmented route storage getter must be declared'
Assert-Contains $header 'void\s+portion2_route_storage_set\s*\(\s*uint16\s+index\s*,\s*state_t\s+point\s*\)' 'segmented route storage setter must be declared'
Assert-Contains $header 'GPS_state\s+portion2_gps_storage_get\s*\(\s*uint16\s+index\s*\)' 'segmented GPS storage getter must be declared'
Assert-Contains $header 'void\s+portion2_gps_storage_set\s*\(\s*uint16\s+index\s*,\s*GPS_state\s+point\s*\)' 'segmented GPS storage setter must be declared'
Assert-Contains $header '#define\s+PORTION2_ROUTE_RETURN_1\s+5' 'route 6 must replace old reverse route 1'
Assert-Contains $header '#define\s+PORTION2_ROUTE_RETURN_5\s+9' 'route 10 must replace old reverse route 5'
Assert-Contains $header '#define\s+PORTION2_ROUTE_STRAIGHT\s+10' 'route 11 must be the straight route'
Assert-Contains $header '#define\s+PORTION2_ROUTE_SNAKE\s+11' 'route 12 must be the snake route'
Assert-Contains $source 'const uint8 portion2_route_required_gps_count\[PORTION2_ROUTE_COUNT\]\s*=\s*\{(?:\s*8,){11}\s*8\s*\}' 'all 12 routes must require eight trustworthy GPS points'
Assert-Contains $flash 'PORTION2_ROUTE_CONTINUATION_MAGIC' 'expanded inertial records must use a continuation flash page'
Assert-Contains $flash '#define\s+PORTION2_GPS_PRIMARY_RECORDS\s+\(\(PORTION2_FLASH_PAGE_WORDS\s*-\s*PORTION2_GPS_PAGE_HEADER_WORDS\)\s*/\s*4\)' 'primary GPS page capacity must be calculated from the flash page size'
Assert-Contains $flash '#define\s+PORTION2_GPS_CONTINUATION_RECORDS\s+\(PORTION2_TOTAL_GPS_COUNT\s*-\s*PORTION2_GPS_PRIMARY_RECORDS\)' 'remaining GPS records must use a continuation page'
Assert-Contains $flash 'PORTION2_GPS_CONTINUATION_MAGIC' 'GPS continuation page must have its own layout marker'
Assert-Contains $flash 'PORTION2_GPS_PRIMARY_PAGE_WORDS\s*>\s*PORTION2_FLASH_PAGE_WORDS' 'primary GPS page must have a compile-time capacity guard'
Assert-Contains $flash 'PORTION2_GPS_CONT_PAGE_WORDS\s*>\s*PORTION2_FLASH_PAGE_WORDS' 'continuation GPS page must have a compile-time capacity guard'
Assert-Contains $flash 'flash_write_page_from_buffer\(FLASH_SECTION_INDEX,\s*RECODE_PASSAGE_GPS_CONT\)' 'GPS continuation records must be written to a second page'
Assert-Contains $flash 'flash_read_page_to_buffer\(FLASH_SECTION_INDEX,\s*RECODE_PASSAGE_GPS_CONT\)' 'GPS continuation records must be read from a second page'
Assert-Contains $flash 'portion2_route_storage_get\s*\(' 'flash writes must read inertial points through segmented storage'
Assert-Contains $flash 'portion2_route_storage_set\s*\(' 'flash reads must write inertial points through segmented storage'
Assert-Contains $flash 'portion2_gps_storage_get\s*\(' 'flash writes must read GPS points through segmented storage'
Assert-Contains $flash 'portion2_gps_storage_set\s*\(' 'flash reads must write GPS points through segmented storage'
Assert-Contains $main '#define\s+PORTION2_ALL_LIGHT_PIN\s+P02_1' 'all-light output must use P02_1'
Assert-Contains $main 'gpio_init\(PORTION2_ALL_LIGHT_PIN,\s*GPO,\s*GPIO_LOW,\s*GPO_PUSH_PULL\)' 'P02_1 must start low as a push-pull output'
Assert-Contains $main 'OFFLINE_VOICE_CMD_INTERIOR_LIGHT[\s\S]*?Portion2_Aux_Start\(7\)' 'voice command 0x0A must start the all-light action'
Assert-Contains $main 'if\(mode\s*==\s*7\)[\s\S]*?gpio_high\(PORTION2_ALL_LIGHT_PIN\)' 'all-light action must drive P02_1 high'
Assert-Contains $main 'Portion2_Aux_Stop\(void\)[\s\S]*?gpio_low\(PORTION2_ALL_LIGHT_PIN\)' 'stopping an auxiliary action must drive P02_1 low'
Assert-Contains $main 'system_getval_ms\(\)\s*-\s*portion2_aux_start_ms\)\s*>=\s*10000' 'all-light action must share the 10-second timeout'
Assert-Contains $main 'portion2_mode_k4_short_event\(\)[\s\S]*?Portion2_Aux_Stop\(\)[\s\S]*?main_mode\s*=\s*Guandao_Portion2_Recode' 'leaving run mode must turn P02_1 off'
Assert-Contains $main "data\s*>=\s*'A'\s*&&\s*data\s*<=\s*'G'[\s\S]*?Portion2_Aux_Start\(data\s*-\s*'A'\s*\+\s*1\)" 'serial G must start the all-light action'
Assert-Contains $main "data\s*>=\s*'a'\s*&&\s*data\s*<=\s*'g'[\s\S]*?Portion2_Aux_Start\(data\s*-\s*'a'\s*\+\s*1\)" 'serial g must start the all-light action'
Assert-Contains $main "data\s*==\s*'U'\s*\|\|\s*data\s*==\s*'u'[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_5\)" 'serial U/u must run return route 5'
Assert-Contains $main "data\s*==\s*'V'\s*\|\|\s*data\s*==\s*'v'[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M" 'serial V/v must run the new gyro snake reverse action'
Assert-Contains $main "data\s*==\s*'W'\s*\|\|\s*data\s*==\s*'w'[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M" 'serial W/w must run the new gyro snake forward action'
Assert-Contains $main 'uint8\s+saved_route_count\s*=\s*0' 'run UI must count saved routes instead of printing a long route list'
Assert-Contains $main 'ips200_show_int\(X\(9\),\s*Y\(4\),\s*saved_route_count,\s*2\)' 'run UI saved route count must stay inside the screen width'
if ($main -match 'ips200_show_string\(X\(9\),\s*Y\(4\),\s*routes') {
    throw 'run UI must not print the full route list at X(9)'
}

$voiceMappings = @(
    'OFFLINE_VOICE_CMD_GATE1_RIGHT_BACK[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_1\)',
    'OFFLINE_VOICE_CMD_GATE3_LEFT_BACK[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_5\)',
    'OFFLINE_VOICE_CMD_FORWARD_10M[\s\S]*?VOICE_DRIVE_ACTION_ENCODER_YAW_FORWARD_10M',
    'OFFLINE_VOICE_CMD_BACKWARD_10M[\s\S]*?VOICE_DRIVE_ACTION_ENCODER_YAW_REVERSE_10M',
    'OFFLINE_VOICE_CMD_SNAKE_FORWARD[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M',
    'OFFLINE_VOICE_CMD_SNAKE_BACKWARD[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M',
    'OFFLINE_VOICE_CMD_ROUTE_SNAKE[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M',
    'OFFLINE_VOICE_CMD_BACK_SNAKE[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M'
)

foreach ($mapping in $voiceMappings) {
    Assert-Contains $main $mapping "missing voice mapping: $mapping"
}

Assert-Contains $source 'float\s+base_speed\s*=\s*15\.0f' 'route base speed must default to 1.5m/s'
Assert-Contains $flash 'int16\s+control\[5\]\s*=\s*\{15,\s*-10,' 'flash defaults must use 1.5m/s forward route speed'
if ([regex]::Matches($flash, 'control\[0\]\s*=\s*15;').Count -ne 2) {
    throw 'flash read and write paths must both force route speed to 1.5m/s'
}
Assert-Contains $fixed '#define\s+SUBJECT_2_FIXED_TURN_SPEED_MPS\s+\(1\.0f\)' 'fixed turn and circle speed must be 1.0m/s'
Assert-Contains $fixed '#define\s+SUBJECT_2_ENCODER_YAW_SPEED_MPS\s+\(1\.0f\)' 'encoder/yaw straight speed must be 1.0m/s'
Assert-Contains $fixed '#define\s+SUBJECT_2_ENCODER_YAW_MIN_SPEED_MPS\s+\(1\.0f\)' 'encoder/yaw action must not slow during its final meter'
Assert-Contains $header '#define\s+MIN_SPEED\s+6\.0f' 'minimum route tracking speed must be 0.6m/s'
Assert-Contains $source '#define\s+GUANDAO_LARGE_CURVE_SPEED\s+11\.0f' 'large-curve speed must be 1.1m/s'
Assert-Contains $source '#define\s+GUANDAO_SHARP_TURN_SPEED\s+8\.0f' 'sharp-turn speed must be 0.8m/s'
Assert-Contains $source '#define\s+PORTION2_FINAL_YAW_ALIGN_SPEED\s+4\.0f' 'final yaw alignment speed must be 0.4m/s'
Assert-Contains $source '#define\s+PORTION2_TERMINAL_APPROACH_SPEED\s+6\.0f' 'terminal route approach must respect the 0.6m/s minimum'
Assert-Contains $source 'upcoming_turn\s*>=\s*GUANDAO_SHARP_TURN_ANGLE[\s\S]*?v_center\s*>\s*GUANDAO_SHARP_TURN_SPEED' 'sharp turns must use the sharp-turn speed cap'
Assert-Contains $source 'fabsf\(angle_diff\)\s*>\s*25\.0f[\s\S]*?v_center\s*>\s*GUANDAO_LARGE_CURVE_SPEED' 'large turns must use the large-curve speed cap'
Assert-NotContains $source 'float\s+terminal_speed\s*=\s*base_speed\s*\*\s*\(dist_to_final\s*/\s*final_dsts\)' 'run mode must not use terminal-distance linear slowdown'
Assert-Contains $source 'target_steering\s*=\s*portion2_guided_terminal_steering\s*\(' 'terminal yaw guidance must happen during normal line following'
Assert-Contains $source '#define\s+PORTION2_SNAKE_STEERING_CMD_LIMIT\s+20\.0f' 'route 12 normal steering limit must be 20 degrees'
Assert-Contains $source '#define\s+PORTION2_SNAKE_SHARP_STEERING_CMD_LIMIT\s+30\.0f' 'route 12 sharp steering limit must be 30 degrees'
Assert-Contains $source '#define\s+PORTION2_SNAKE_FINAL_STOP_DIST\s+0\.40f' 'route 12 final stop radius must be 0.40m'
Assert-Contains $source '#define\s+PORTION2_SNAKE_OVERSHOOT_ARM_DIST\s+0\.80f' 'route 12 overshoot monitor must arm within 0.80m'
Assert-Contains $source '#define\s+PORTION2_SNAKE_OVERSHOOT_RISE_DIST\s+0\.15f' 'route 12 overshoot rise must be 0.15m'
Assert-Contains $source '#define\s+PORTION2_SNAKE_OVERSHOOT_CONFIRM_CYCLES\s+3U' 'route 12 overshoot must require three cycles'
Assert-Contains $source 'static\s+uint8\s+portion2_route12_overshoot_detect\s*\(' 'route 12 overshoot detector must exist'
Assert-Contains $source 'portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE\s*\?\s*PORTION2_SNAKE_STEERING_CMD_LIMIT' 'route 12 must select its normal steering limit'
Assert-Contains $source 'reference_turn\s*>=\s*PORTION2_SHARP_TURN_TRIGGER_DEG[\s\S]*?portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE\s*\?\s*PORTION2_SNAKE_SHARP_STEERING_CMD_LIMIT' 'route 12 sharp turns must select the 30 degree limit'
Assert-Contains $source 'portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE\s*\?\s*PORTION2_SNAKE_FINAL_STOP_DIST' 'route 12 must select the 0.40m stop radius'
Assert-Contains $source 'max_align_dist\s*=\s*\(portion2_selected_route\s*==\s*PORTION2_ROUTE_SNAKE\)[\s\S]*?\?\s*PORTION2_SNAKE_FINAL_STOP_DIST\s*:\s*PORTION2_FINAL_YAW_ALIGN_MAX_DIST' 'route 12 yaw alignment must retain the 0.40m radius'
Assert-Contains $source 'portion2_route12_overshoot_detect\s*\([^;]+\)[\s\S]*?guandao_debug_stop_reason\s*=\s*12' 'confirmed route 12 overshoot must stop with reason 12'
Assert-Contains $source 'void\s+portion2_run_stop\s*\([^)]*\)[\s\S]*?portion2_route12_overshoot_reset\s*\(' 'manual run stop must reset route 12 overshoot state'

Write-Host 'portion-2 12-route layout checks passed'

