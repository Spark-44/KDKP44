$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$guandaoC = Get-Content -Raw -Path (Join-Path $root "code\guandao.c")
$guandaoH = Get-Content -Raw -Path (Join-Path $root "code\guandao.h")
$imuC = Get-Content -Raw -Path (Join-Path $root "code\IMU.c")
$imuH = Get-Content -Raw -Path (Join-Path $root "code\IMU.h")
$cpu0 = Get-Content -Raw -Path (Join-Path $root "user\cpu0_main.c")
$controlH = Get-Content -Raw -Path (Join-Path $root "code\control.h")
$displayC = Get-Content -Raw -Path (Join-Path $root "code\display.c")
$displayH = Get-Content -Raw -Path (Join-Path $root "code\display.h")
$flashC = Get-Content -Raw -Path (Join-Path $root "code\flash.c")
$flashH = Get-Content -Raw -Path (Join-Path $root "code\flash.h")
$fixedActionC = Get-Content -Raw -Path (Join-Path $root "code\subject_2_fixed_action.c")
$gpsC = Get-Content -Raw -Path (Join-Path $root "code\gps.c")
$gpsH = Get-Content -Raw -Path (Join-Path $root "code\gps.h")
$commonHeadfile = Get-Content -Raw -Path (Join-Path $root "libraries\zf_common\zf_common_headfile.h")
$debugCodeSubdirMkPath = Join-Path $root "Debug\code\subdir.mk"
$debugCodeSubdirMk = if(Test-Path $debugCodeSubdirMkPath) { Get-Content -Raw -Path $debugCodeSubdirMkPath } else { "" }
$runUi = ""
if($cpu0 -match "static\s+void\s+Portion2_Run_Mode_UI_Update\s*\(\s*void\s*\)\s*\{([\s\S]*?)\r?\n\}\r?\n\r?\nstatic\s+void\s+Portion2_Fixed_Action_Start")
{
    $runUi = $Matches[1]
}
$recordStartCases = ""
if($guandaoC -match "case\s+0\s*:\s*[\r\n\s]*case\s+2\s*:\s*([\s\S]*?)\r?\n\s*break\s*;\s*\r?\n\s*case\s+1\s*:")
{
    $recordStartCases = $Matches[1]
}
$recordKeyResetBody = ""
if($guandaoC -match "static\s+void\s+portion2_record_key_state_reset\s*\(\s*void\s*\)\s*\{([\s\S]*?)\r?\n\}")
{
    $recordKeyResetBody = $Matches[1]
}
$recordTaskBody = ""
if($guandaoC -match "void\s+portion2_record_task\s*\(\s*void\s*\)\s*\{([\s\S]*?)\r?\n\}\r?\n\r?\nvoid\s+portion2_run_select_route")
{
    $recordTaskBody = $Matches[1]
}
$sleepCaseBody = ""
if($cpu0 -match "case\s+OFFLINE_VOICE_CMD_SLEEP\s*:\s*([\s\S]*?)\r?\n\s*break\s*;")
{
    $sleepCaseBody = $Matches[1]
}

$checks = @(
    @{ Name = "route dump API declared"; Pass = $guandaoH -match "void\s+portion2_serial_dump_routes\s*\(\s*void\s*\)" },
    @{ Name = "runtime trace toggle API declared"; Pass = $guandaoH -match "void\s+portion2_serial_toggle_trace\s*\(\s*void\s*\)" },
    @{ Name = "stop API is reused for serial command"; Pass = $cpu0 -match "portion2_run_stop\s*\(\s*\)" },
    @{ Name = "D command handled"; Pass = $cpu0 -match "data\s*==\s*'D'" },
    @{ Name = "T command handled"; Pass = $cpu0 -match "data\s*==\s*'T'" },
    @{ Name = "S command handled"; Pass = $cpu0 -match "data\s*==\s*'S'" },
    @{ Name = "record point serial log emitted"; Pass = $guandaoC -match "\[P2-REC\]" },
    @{ Name = "record gps serial log emitted"; Pass = $guandaoC -match "\[P2-REC-GPS\]" },
    @{ Name = "record periodic progress emitted"; Pass = $guandaoC -match "\[P2-REC-STATUS\]" },
    @{ Name = "record progress includes raw gps distance and step"; Pass = $guandaoC -match "raw=%u/%u gps=%u/%u dist=" -and $guandaoC -match "step=" },
    @{ Name = "record screen shows raw gps distance and step"; Pass = $guandaoC -match '"RAW"' -and $guandaoC -match '"GPS"' -and $guandaoC -match '"DIST"' -and $guandaoC -match '"STEP"' },
    @{ Name = "runtime serial log emitted"; Pass = $guandaoC -match "\[P2-RUN\]" },
    @{ Name = "runtime live progress includes human point numbers"; Pass = $guandaoC -match "raw_no=%d/%d plan_no=%d/%d gps_no=%u/%d" },
    @{ Name = "runtime screen shows raw plan and gps progress"; Pass = $guandaoC -match '"RAW"' -and $guandaoC -match '"PLAN"' -and $guandaoC -match '"GPS"' },
    @{ Name = "runtime point progress event emitted"; Pass = $guandaoC -match "\[P2-RUN-PT\]" },
    @{ Name = "runtime point progress includes raw record point"; Pass = $guandaoC -match "raw_pt=%d/%d" },
    @{ Name = "runtime live progress includes steering"; Pass = $guandaoC -match "target_x=" -and $guandaoC -match "servo=" -and $guandaoC -match "final_dist=" },
    @{ Name = "runtime gps progress emitted"; Pass = $guandaoC -match "\[P2-RUN-GPS\]" },
    @{ Name = "runtime gps bind points are mapped to planned route"; Pass = $guandaoC -match "portion2_plan_index_from_raw_point" -and $guandaoC -match "gps_point\.cheak_flag\s*=\s*portion2_plan_index_from_raw_point" },
    @{ Name = "runtime end event emitted"; Pass = $guandaoC -match "\[P2-RUN-END\]" },
    @{ Name = "portion2 route yaw alignment helper exists"; Pass = $guandaoC -match "static\s+void\s+portion2_align_route_to_current_yaw\s*\(" -and $guandaoC -match "run_start_theta\s*=\s*portion2_run_drive_reverse\s*\?\s*Yaw_1\s*\+\s*180\.0f\s*:\s*Yaw_1" },
    @{ Name = "portion2 always aligns route before gps preparation"; Pass = $guandaoC -match "portion2_translate_route_to_origin\s*\(\s*\)[\s\S]*?portion2_align_route_to_current_yaw\s*\(\s*run_start_theta\s*\)[\s\S]*?portion2_smooth_reference_route\s*\(\s*\)[\s\S]*?portion2_gps_fusion_prepare\s*\(\s*&portion_2\s*\)" -and $guandaoC -notmatch "if\s*\(\s*!portion2_gps_fusion_prepare\s*\(\s*&portion_2\s*\)\s*\)" },
    @{ Name = "route alignment rebuilds yaw from route geometry after reset"; Pass = $guandaoC -match "route_start_yaw\s*=\s*guandao_segment_yaw\s*\(\s*portion_2\.recode_map\[0\]\s*,\s*portion_2\.recode_map\[1\]\s*\)" -and $guandaoC -match "portion_2\.recode_map\[length\s*-\s*1\]\.theta\s*=\s*guandao_segment_yaw" },
    @{ Name = "route dump serial log emitted"; Pass = $guandaoC -match "\[P2-DUMP\]" },
    @{ Name = "per-route saved flags exist"; Pass = $guandaoC -match "portion2_route_saved_flag\s*\[\s*PORTION2_ROUTE_COUNT\s*\]" },
    @{ Name = "route dump includes saved state"; Pass = $guandaoC -match "saved=%s" },
    @{ Name = "single saved flag removed"; Pass = $guandaoC -notmatch "portion2_saved_flag" },
    @{ Name = "route switching does not clear saved flag"; Pass = $guandaoC -notmatch "portion2_route_saved_flag\s*\[\s*portion2_record_route\s*\]\s*=\s*0\s*;\s*Buzzer_check\(20\)" },
    @{ Name = "boot read is not immediately reset"; Pass = $cpu0 -notmatch "Flash_Main_Read\s*\(\s*\)\s*;\s*[\r\n\s]*main_mode\s*=\s*Guandao_Portion2_Recode\s*;\s*[\r\n\s]*route_setting_choice\s*=\s*1\s*;\s*[\r\n\s]*conrtol_mode\s*=\s*YAOKONG\s*;\s*[\r\n\s]*portion2_record_reset\s*\(\s*\)" },
    @{ Name = "loaded routes are marked saved after boot read"; Pass = $cpu0 -match "Flash_Main_Read\s*\(\s*\)\s*;\s*[\r\n\s]*portion2_record_mark_loaded_routes_saved\s*\(\s*\)" },
    @{ Name = "menu entering record mode preserves routes"; Pass = $displayC -notmatch "Guandao_Portion2_Recode[^;\r\n]*;[^;\r\n]*route_setting_choice[^;\r\n]*;[^;\r\n]*conrtol_mode[^;\r\n]*;[^;\r\n]*portion2_record_reset\s*\(\s*\)" },
    @{ Name = "record K4 short clears release flag before run mode"; Pass = $guandaoC -match "if\s*\(\s*k4_short\s*\)\s*\{[\s\S]*?portion2_record_key_state_reset\s*\(\s*\)[\s\S]*?main_mode\s*=\s*Guandao_Voice" },
    @{ Name = "K4 long press is locked by shared mode key code"; Pass = $guandaoC -match "portion2_mode_k4_short_event\s*\(\s*void\s*\)[\s\S]*?gpio_get_level\s*\(\s*KEY4\s*\)\s*==\s*0" -and $guandaoC -match "now_ms\s*-\s*portion2_mode_k4_start_ms\s*\)\s*>\s*1500U[\s\S]*?portion2_mode_k4_wait_release\s*=\s*1" },
    @{ Name = "no commented-out key condition remains"; Pass = $cpu0 -notmatch "//[^\r\n]*if\s*\(\s*gpio_get_level\s*\(\s*KEY[14]\s*\)" },
    @{ Name = "run motor enable gate removed"; Pass = $cpu0 -notmatch "run_motor_enabled" },
    @{ Name = "run UI no longer shows motor enable"; Pass = $cpu0 -notmatch "MOTOR:\s*ON|MOTOR:\s*OFF|K1 MOTOR" },
    @{ Name = "voice commands ignored outside run mode"; Pass = $cpu0 -match "static\s+void\s+Portion2_Voice_Command_Handle[\s\S]*?if\s*\(\s*main_mode\s*!=\s*Guandao_Voice\s*\)[\s\S]*?return\s*;" },
    @{ Name = "record DONE state removed"; Pass = $recordTaskBody -ne "" -and $recordTaskBody -notmatch "case\s+3\s*:" -and $guandaoC -notmatch '"DONE\s*"' },
    @{ Name = "record route overflow clamps to last route and WAIT"; Pass = $recordTaskBody -match "if\s*\(\s*portion2_record_route\s*>=\s*PORTION2_ROUTE_COUNT\s*\)\s*\{[\s\S]*?portion2_record_route\s*=\s*PORTION2_ROUTE_COUNT\s*-\s*1\s*;[\s\S]*?portion2_record_state\s*=\s*2\s*;" },
    @{ Name = "record key state reset API declared"; Pass = $guandaoH -match "void\s+portion2_record_enter_mode\s*\(\s*void\s*\)" },
    @{ Name = "record key state reset API implemented"; Pass = $guandaoC -match "void\s+portion2_record_enter_mode\s*\(\s*void\s*\)[\s\S]*?portion2_record_key_state_reset\s*\(\s*\)" },
    @{ Name = "record key reset does not clear route data"; Pass = $recordKeyResetBody -ne "" -and $recordKeyResetBody -notmatch "portion2_route_length\s*\[" -and $recordKeyResetBody -notmatch "portion2_route_gps_count\s*\[" -and $recordKeyResetBody -notmatch "guandao_state_init" },
    @{ Name = "mode K4 shared event API declared"; Pass = $guandaoH -match "uint8\s+portion2_mode_k4_short_event\s*\(\s*void\s*\)" -and $guandaoH -match "void\s+portion2_mode_key_transition_lock\s*\(\s*void\s*\)" },
    @{ Name = "mode K4 shared event waits for release"; Pass = $guandaoC -match "static\s+uint8\s+portion2_mode_k4_wait_release" -and $guandaoC -match "uint8\s+portion2_mode_k4_short_event\s*\(\s*void\s*\)[\s\S]*?gpio_get_level\s*\(\s*KEY4\s*\)" -and $guandaoC -match "portion2_mode_k4_wait_release\s*=\s*1" },
    @{ Name = "record K4 enters run through shared event"; Pass = $guandaoC -match "k4_short\s*=\s*portion2_mode_k4_short_event\s*\(\s*\)" -and $guandaoC -match "if\s*\(\s*k4_short\s*\)\s*\{[\s\S]*?portion2_mode_key_transition_lock\s*\(\s*\)[\s\S]*?main_mode\s*=\s*Guandao_Voice" },
    @{ Name = "run K4 returns to record through shared event"; Pass = $cpu0 -match "if\s*\(\s*portion2_mode_k4_short_event\s*\(\s*\)\s*\)[\s\S]*?portion2_mode_key_transition_lock\s*\(\s*\)[\s\S]*?portion2_record_enter_mode\s*\(\s*\)" },
    @{ Name = "run to record restores key gpio after screen init"; Pass = $cpu0 -match "if\s*\(\s*portion2_mode_k4_short_event\s*\(\s*\)\s*\)[\s\S]*?portion2_record_enter_mode\s*\(\s*\)\s*;\s*[\r\n\s]*Key_Init\s*\(\s*\)" },
    @{ Name = "run mode no longer directly consumes K4 flag"; Pass = $cpu0 -notmatch "if\s*\(\s*key4_flag\s*\)[\s\S]*?main_mode\s*=\s*Guandao_Portion2_Recode" },
    @{ Name = "last route completion stays in WAIT"; Pass = $recordTaskBody -match "if\s*\(\s*portion2_record_route\s*\+\s*1\s*<\s*PORTION2_ROUTE_COUNT\s*\)[\s\S]*?portion2_record_route\s*\+\+[\s\S]*?portion2_record_state\s*=\s*2" -and $recordTaskBody -notmatch "portion2_record_state\s*=\s*\([^;\r\n]*PORTION2_ROUTE_COUNT[^;\r\n]*\)\s*\?\s*3\s*:\s*2" },
    @{ Name = "run main UI does not overwrite lower debug rows"; Pass = $runUi -ne "" -and $runUi -notmatch "ips200_show_string\s*\(\s*X\s*\(\s*1\s*\)\s*,\s*Y\s*\(\s*1[2-5]\s*\)" },
    @{ Name = "route run completeness helper exists"; Pass = $guandaoC -match "static\s+uint8\s+portion2_route_ready_for_run\s*\(\s*uint8\s+route_id\s*\)" },
    @{ Name = "each route stores up to fourteen gps points"; Pass = $guandaoH -match "PORTION2_GPS_PER_ROUTE\s+14" },
    @{ Name = "nine routes reserve 126 gps records"; Pass = $guandaoH -match "MAX_GPS_RECODE\s+126" -and $guandaoH -match "PORTION2_TOTAL_GPS_COUNT\s+126" },
    @{ Name = "route run still requires five gps points"; Pass = $guandaoC -match "portion2_route_required_gps_count\s*\[\s*PORTION2_ROUTE_COUNT\s*\]\s*=\s*\{\s*5\s*,\s*5\s*,\s*5\s*,\s*5\s*,\s*5\s*,\s*5\s*,\s*5\s*,\s*5\s*,\s*5\s*\}" },
    @{ Name = "gps route slots use capacity based offsets"; Pass = $guandaoC -match "return\s+\(uint16\)\(route_id\s*\*\s*PORTION2_GPS_PER_ROUTE\)\s*;" },
    @{ Name = "gps recording stops at route capacity"; Pass = $guandaoC -match "gps_count\s*>=\s*PORTION2_GPS_PER_ROUTE" },
    @{ Name = "expanded gps table uses second flash page"; Pass = $flashH -match "RECODE_PASSAGE_TWO" -and $flashC -match "flash_write_page_from_buffer\s*\(\s*FLASH_SECTION_INDEX\s*,\s*RECODE_PASSAGE_TWO\s*\)" -and $flashC -match "flash_read_page_to_buffer\s*\(\s*FLASH_SECTION_INDEX\s*,\s*RECODE_PASSAGE_TWO\s*\)" },
    @{ Name = "second gps page stores version marker and total count"; Pass = $flashC -match "PORTION2_GPS_LAYOUT_MAGIC\s+\(0x5032474EU\)" -and $flashC -match "PORTION2_GPS_PAGE_TOTAL_INDEX" -and $flashC -match "PORTION2_TOTAL_GPS_COUNT" },
    @{ Name = "legacy gps layout is invalidated"; Pass = $flashC -match "gps_layout_valid" -and $flashC -match "!gps_layout_valid[\s\S]*?portion2_route_gps_count\s*\[\s*i\s*\]\s*=\s*0" },
    @{ Name = "recording rejects repeated gps coordinates"; Pass = $guandaoC -match "PORTION2_GPS_RECORD_MIN_MOVE_M\s+0\.20f" -and $guandaoC -match "portion2_gps_candidate_valid\s*\(" },
    @{ Name = "automatic gps recording reserves final slot"; Pass = $guandaoC -match "gps_count\s*>=\s*PORTION2_GPS_PER_ROUTE\s*-\s*1" },
    @{ Name = "record stop attempts endpoint gps capture"; Pass = $recordTaskBody -match "portion2_record_try_gps_point\s*\(\s*1\s*\)" },
    @{ Name = "run validation requires gps endpoint coverage"; Pass = $guandaoC -match "PORTION2_GPS_END_MAX_RAW_GAP\s+4" -and $guandaoC -match "last_gps_raw_point\s*<\s*\(int16\)portion2_route_length\[route_id\]\s*-\s*1\s*-\s*PORTION2_GPS_END_MAX_RAW_GAP" },
    @{ Name = "portion2 gps fusion API declared"; Pass = $gpsH -match "uint8\s+portion2_gps_fusion_prepare" -and $gpsH -match "void\s+portion2_gps_fusion_update" -and $gpsH -match "void\s+portion2_gps_fusion_reset" },
    @{ Name = "gps fusion startup calibration API declared"; Pass = $gpsH -match "uint8\s+portion2_gps_fusion_startup_update\s*\(\s*guandao_state\s*\*state\s*\)" },
    @{ Name = "gps startup requires three stable fixes within six seconds"; Pass = $gpsC -match "PORTION2_GPS_STARTUP_REQUIRED_SAMPLES\s+\(3U\)" -and $gpsC -match "PORTION2_GPS_STARTUP_TIMEOUT_MS\s+\(6000U\)" -and $gpsC -match "PORTION2_GPS_STARTUP_STABILITY_M\s+\(1\.0f\)" },
    @{ Name = "gps startup calibration shifts transform to local origin"; Pass = $gpsC -match "transform_tx\s*\+=\s*state->current_state\.x\s*-\s*mean_x" -and $gpsC -match "transform_ty\s*\+=\s*state->current_state\.y\s*-\s*mean_y" },
    @{ Name = "portion2 gps calibration state holds vehicle stopped"; Pass = $guandaoC -match "case\s+4\s*:[\s\S]*?out_v_l\s*=\s*0[\s\S]*?out_v_r\s*=\s*0[\s\S]*?out_servo\s*=\s*0[\s\S]*?portion2_gps_fusion_startup_update" },
    @{ Name = "gps fusion validates fix and satellites"; Pass = $gpsC -match "gnss\.state" -and $gpsC -match "gnss\.satellite_used\s*<\s*PORTION2_GPS_FUSION_MIN_SATELLITES" },
    @{ Name = "gps fusion rejects repeated and large-error fixes"; Pass = $gpsC -match "PORTION2_GPS_FUSION_REPEAT_DISTANCE" -and $gpsC -match "PORTION2_GPS_FUSION_MAX_ERROR\s+\(3\.0f\)" },
    @{ Name = "gps fusion disables after three consecutive large errors"; Pass = $gpsC -match "PORTION2_GPS_FUSION_MAX_LARGE_ERRORS\s+\(3U\)" -and $gpsC -match "large_error_count\s*>=\s*PORTION2_GPS_FUSION_MAX_LARGE_ERRORS[\s\S]*?ready\s*=\s*0" },
    @{ Name = "gps fusion correction is filtered and capped"; Pass = $gpsC -match "PORTION2_GPS_FUSION_GAIN\s+\(0\.10f\)" -and $gpsC -match "PORTION2_GPS_FUSION_MAX_CORRECTION\s+\(0\.10f\)" -and $gpsC -match "state->current_state\.x\s*\+=\s*correction_x" -and $gpsC -match "state->current_state\.y\s*\+=\s*correction_y" },
    @{ Name = "gps fusion emits serial diagnostics"; Pass = $gpsC -match "\[P2-GPS-FUSION\]" },
    @{ Name = "recorded gps binds zero-based raw point"; Pass = $guandaoC -match "cheak_flag\s*=\s*\(portion2_route_length\[portion2_record_route\]\s*>\s*0\)\s*\?\s*\(int16\)portion2_route_length\[portion2_record_route\]\s*-\s*1\s*:\s*0" },
    @{ Name = "new fusion gps layout marker invalidates old binding"; Pass = $flashC -match "PORTION2_GPS_LAYOUT_MAGIC\s+\(0x5032474EU\)" },
    @{ Name = "gps transform is prepared after route alignment"; Pass = $guandaoC -match "portion2_align_route_to_current_yaw\s*\(\s*run_start_theta\s*\)[\s\S]*?gps_prepare_ready\s*=\s*portion2_gps_fusion_prepare" },
    @{ Name = "gps fusion updates before pursuit steering"; Pass = $guandaoC -match "update_state\s*\(\s*p[\s\S]*?portion2_gps_fusion_update\s*\(\s*p\s*\)[\s\S]*?pursuit_contral_mode" },
    @{ Name = "run screen shows gps fusion state"; Pass = $guandaoC -match '"GF"' -and $guandaoC -match "portion2_gps_fusion_get_error" },
    @{ Name = "route run requires enough inertial points"; Pass = $guandaoC -match "portion2_route_length\s*\[\s*route_id\s*\]\s*<\s*required" },
    @{ Name = "route run requires enough gps points"; Pass = $guandaoC -match "gps_count\s*=\s*portion2_route_gps_count\s*\[\s*route_id\s*\]" -and $guandaoC -match "gps_count\s*<\s*required" },
    @{ Name = "incomplete route rejected before run"; Pass = $guandaoC -match "portion2_run_reject_reason\s*=\s*4" -and $guandaoC -match "if\s*\(\s*!\s*portion2_route_ready_for_run\s*\(\s*route_id\s*\)\s*\)" },
    @{ Name = "default base speed is 10"; Pass = $guandaoC -match "float\s+base_speed\s*=\s*10\.0f\s*;" },
    @{ Name = "default forward control speed is 10"; Pass = $flashC -match "int16\s+control\s*\[\s*5\s*\]\s*=\s*\{\s*10\s*," },
    @{ Name = "flash forward speed forced to 10"; Pass = $flashC -match "control\s*\[\s*0\s*\]\s*=\s*10\s*;" },
    @{ Name = "record step is forced to 0.4 after flash read"; Pass = $flashC -match "Flash_Read_pid[\s\S]*?speed_pid\s*\[\s*3\s*\]\s*=\s*FLASH_RECODE_THRESHOLD_DEFAULT\s*;[\s\S]*?recode_threshold\s*=\s*speed_pid\s*\[\s*3\s*\]" },
    @{ Name = "record step 0.4 is persisted on flash write"; Pass = $flashC -match "Flash_Write_pid[\s\S]*?speed_pid\s*\[\s*3\s*\]\s*=\s*FLASH_RECODE_THRESHOLD_DEFAULT\s*;[\s\S]*?flash_union_buffer\s*\[\s*i\s*\]\.float_type\s*=\s*speed_pid\s*\[\s*i\s*\]" },
    @{ Name = "record start keeps selected route"; Pass = $recordStartCases -ne "" -and $recordStartCases -notmatch "portion2_record_route\s*=\s*0\s*;" },
    @{ Name = "voice sleep command has no handler"; Pass = $cpu0 -notmatch "case\s+OFFLINE_VOICE_CMD_SLEEP\s*:" },
    @{ Name = "voice sleep no longer stops vehicle"; Pass = $cpu0 -notmatch "case\s+OFFLINE_VOICE_CMD_SLEEP\s*:[\s\S]*?(voice_drive_action_stop|portion2_run_stop|rear_motor_stop)\s*\(" },
    @{ Name = "fixed turn timeout is 60 seconds"; Pass = $fixedActionC -match "SUBJECT_2_FIXED_TURN_TIMEOUT_MS\s+\(60000U\)" -and $fixedActionC -match "elapsed_ms\s*>=\s*SUBJECT_2_FIXED_TURN_TIMEOUT_MS" },
    @{ Name = "fixed turn logs yaw delta"; Pass = $fixedActionC -match "\[FIXED\]" -and $fixedActionC -match "voice_drive_action_get_yaw_delta" },
    @{ Name = "smooth plan keeps sharp corners linear"; Pass = $guandaoC -match "keep_corner_linear" -and $guandaoC -match "GUANDAO_SHARP_TURN_ANGLE" },
    @{ Name = "portion2 smooth plan does not force sharp corners"; Pass = $guandaoC -match "state\s*!=\s*&portion_2\s*&&\s*\(turn_in\s*>=\s*GUANDAO_SHARP_TURN_ANGLE" },
    @{ Name = "smooth pursuit uses local arrive threshold"; Pass = $guandaoC -match "float\s+arrive_threshold\s*=\s*persuit_threshold" -and $guandaoC -match "distance_to_target\s*<=\s*arrive_threshold" -and $guandaoC -notmatch "persuit_threshold\s*=\s*persuit_threshold\s*\*" },
    @{ Name = "portion2 planned-point arrive threshold is 0.08m"; Pass = $guandaoC -match "PORTION3_PURSUIT_THRESHOLD\s+0\.08f" },
    @{ Name = "smooth pursuit dynamically extends preview"; Pass = $guandaoC -match "int\s+steer_preview_steps\s*=\s*preview_spets" -and $guandaoC -match "steer_preview_steps\s*<\s*4" -and $guandaoC -match "steer_preview_steps\s*<\s*8" },
    @{ Name = "smooth pursuit uses curve preview"; Pass = $guandaoC -match "int\s+curve_preview_steps\s*=\s*5" -and $guandaoC -match "pursuit_midhandle\s*\(\s*state\s*,\s*&current_point\s*,\s*curve_preview_steps" },
    @{ Name = "smooth pursuit rate limits steering target"; Pass = $guandaoC -match "last_target_steering" -and $guandaoC -match "steer_delta_limit" -and $guandaoC -match "last_steer_limit_ms" },
    @{ Name = "smooth pursuit reduces speed from preview angle"; Pass = $guandaoC -match "fabsf\s*\(\s*preview_alpha2\s*\)\s*>\s*GUANDAO_CURVE_TRIGGER_ANGLE" -and $guandaoC -match "curve_scale" },
    @{ Name = "smooth pursuit outputs limited target steering"; Pass = $guandaoC -match "\*out_servo\s*=\s*-target_steering" },
    @{ Name = "smooth pursuit steering command limit is 25 degrees"; Pass = $guandaoC -match "GUANDAO_STEERING_CMD_LIMIT\s+25\.0f" -and $guandaoC -match "GUANDAO_HIGH_SPEED_CMD_LIMIT\s+25\.0f" -and $guandaoC -match "GUANDAO_VERY_HIGH_CMD_LIMIT\s+25\.0f" },
    @{ Name = "curve speed can fall below base speed"; Pass = $guandaoH -match "MIN_SPEED\s+4\.0f" },
    @{ Name = "portion2 reuses portion3 trace standard"; Pass = $guandaoC -match "static\s+uint8\s+guandao_uses_portion3_trace_standard\s*\(\s*guandao_state\s+\*state\s*\)" -and $guandaoC -match "state\s*==\s*&portion_2" },
    @{ Name = "portion3 standard arrival threshold"; Pass = $guandaoC -match "PORTION3_PURSUIT_THRESHOLD\s+0\.08f" -and $guandaoC -match "guandao_uses_portion3_trace_standard\s*\(\s*state\s*\)[\s\S]*?arrive_threshold\s*=\s*PORTION3_PURSUIT_THRESHOLD" },
    @{ Name = "portion3 standard final stop"; Pass = $guandaoC -match "PORTION3_FINAL_STOP_DIST\s+0\.1f" -and $guandaoC -match "guandao_uses_portion3_trace_standard\s*\(\s*state\s*\)[\s\S]*?dist_to_final\s*<=\s*PORTION3_FINAL_STOP_DIST" },
    @{ Name = "portion2 final raw point stop allows 0.6m"; Pass = $guandaoC -match "PORTION2_FINAL_RAW_POINT_STOP_DIST\s+0\.6f" -and $guandaoC -match "raw_point\s*>=\s*raw_length\s*-\s*1" -and $guandaoC -match "dist_to_final\s*<=\s*PORTION2_FINAL_RAW_POINT_STOP_DIST" },
    @{ Name = "portion2 final missed point forces stop"; Pass = $guandaoC -match "PORTION2_FINAL_MISSED_STOP_DIST\s+0\.8f" -and $guandaoC -match "PORTION2_FINAL_MISSED_STOP_REASON\s+12U" -and $guandaoC -match "state->current_point_index\s*>=\s*route_length\s*-\s*1" -and $guandaoC -match "dist_to_final\s*<=\s*PORTION2_FINAL_MISSED_STOP_DIST" },
    @{ Name = "portion2 final stop aligns recorded yaw"; Pass = $guandaoC -match "PORTION2_FINAL_YAW_TOLERANCE_DEG\s+8\.0f" -and $guandaoC -match "portion2_final_yaw_align\s*\(" -and $guandaoC -match "final_point\.theta\s*-\s*Yaw_1" },
    @{ Name = "portion2 final yaw align has safety stop"; Pass = $guandaoC -match "PORTION2_FINAL_YAW_ALIGN_TIMEOUT_MS" -and $guandaoC -match "PORTION2_FINAL_YAW_ALIGN_MAX_DIST" -and $guandaoC -match "dist_to_final\s*>\s*PORTION2_FINAL_YAW_ALIGN_MAX_DIST" },
    @{ Name = "portion2 final yaw is captured before run end"; Pass = $guandaoC -match "portion2_run_final_yaw" -and $guandaoC -match "portion2_run_final_yaw\s*=\s*guandao_route_point" },
    @{ Name = "portion2 final yaw align uses servo sign convention"; Pass = $guandaoC -match "out_servo\s*=\s*\(yaw_error\s*>\s*0\.0f\)\s*\?\s*-PORTION2_FINAL_YAW_ALIGN_STEER_DEG\s*:\s*PORTION2_FINAL_YAW_ALIGN_STEER_DEG" },
    @{ Name = "portion2 reference points are strongly smoothed before planning"; Pass = $guandaoC -match "portion2_smooth_reference_route\s*\(" -and $guandaoC -match "PORTION2_REFERENCE_SMOOTH_PASSES\s+4" -and $guandaoC -match "PORTION2_REFERENCE_SMOOTH_WEIGHT\s+0\.35f" },
    @{ Name = "portion2 pursuit uses softer steering"; Pass = $guandaoC -match "PORTION2_STEERING_CMD_LIMIT\s+12\.0f" -and $guandaoC -match "PORTION2_STEERING_GAIN\s+0\.90f" -and $guandaoC -match "PORTION2_STEER_RATE_LIMIT\s+0\.5f" },
    @{ Name = "portion2 sharp reference bends allow 20 degree steering"; Pass = $guandaoC -match "PORTION2_SHARP_STEERING_CMD_LIMIT\s+20\.0f" -and $guandaoC -match "PORTION2_SHARP_TURN_TRIGGER_DEG\s+4\.0f" -and $guandaoC -match "PORTION2_SHARP_TURN_RAW_LOOKAHEAD\s+6" },
    @{ Name = "portion2 steering limit is selected from raw route curvature"; Pass = $guandaoC -match "portion2_max_reference_turn\s*\(" -and $guandaoC -match "reference_turn\s*>=\s*PORTION2_SHARP_TURN_TRIGGER_DEG" -and $guandaoC -match "steering_limit\s*=\s*PORTION2_SHARP_STEERING_CMD_LIMIT" },
    @{ Name = "portion2 pursuit looks farther ahead"; Pass = $guandaoC -match "PORTION2_MIN_PREVIEW_STEPS\s+14" -and $guandaoC -match "PORTION2_CURVE_PREVIEW_STEPS\s+24" },
    @{ Name = "gps recording rejects jumps beyond inertial travel margin"; Pass = $guandaoC -match "PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M\s+2\.0f" -and $guandaoC -match "gps_distance\s*>\s*inertial_distance\s*\+\s*PORTION2_GPS_RECORD_MAX_JUMP_MARGIN_M" },
    @{ Name = "portion2 run end logs yaw error"; Pass = $guandaoC -match "final_yaw=" -and $guandaoC -match "yaw_err=" },
    @{ Name = "portion3 standard disables yaw correction"; Pass = $guandaoC -match "case\s+2\s*:\s*[\r\n\s]*break\s*;" },
    @{ Name = "portion3 standard endpoint slowdown after curve slowdown"; Pass = $guandaoC -match "curve_scale[\s\S]*?dist_to_final\s*<\s*final_dsts[\s\S]*?v_center\s*=\s*base_speed\s*\*\s*\(dist_to_final\s*/\s*final_dsts\)" },
    @{ Name = "main modes only include record and run"; Pass = $displayH -match "typedef\s+enum\s*\{\s*Guandao_Voice\s*,\s*Guandao_Portion2_Recode\s*\}\s*Mode_Choice\s*;" },
    @{ Name = "removed main modes are not referenced"; Pass = ($cpu0 + $displayC + $displayH + $guandaoC) -notmatch "Mode_IDLE|Guandao_Recode_Mode|Guandao_portion_1|Guandao_portion_3|Rack_Test_Mode|YaoKong_Mode" },
    @{ Name = "record mode uses IDLE control"; Pass = $cpu0 -match "main_mode\s*=\s*Guandao_Portion2_Recode\s*;\s*[\r\n\s]*route_setting_choice\s*=\s*1\s*;\s*[\r\n\s]*conrtol_mode\s*=\s*IDLE\s*;" -and $cpu0 -notmatch "conrtol_mode\s*=\s*YAOKONG" },
    @{ Name = "control modes keep only active controls"; Pass = $controlH -match "typedef\s+enum\s*\{\s*IDLE\s*,\s*GUANDAO\s*,\s*DAOCHE\s*\}\s*MOTER_control_mode\s*;" },
    @{ Name = "removed control modes are not referenced"; Pass = ($cpu0 + $displayC + $controlH) -notmatch "\bYAOKONG\b|\bRACK_TEST\b|\bGPS\b" },
    @{ Name = "old record preview branch removed"; Pass = $guandaoC -notmatch "main_mode\s*==\s*Guandao_Recode_Mode" },
    @{ Name = "IMU_2 source files removed"; Pass = -not (Test-Path (Join-Path $root "code\IMU_2.c")) -and -not (Test-Path (Join-Path $root "code\IMU_2.h")) },
    @{ Name = "IMU_2 removed from generated build list"; Pass = $debugCodeSubdirMk -ne "" -and $debugCodeSubdirMk -notmatch "IMU_2" },
    @{ Name = "common headfile includes merged IMU"; Pass = $commonHeadfile -match '#include\s+"IMU\.h"' -and $commonHeadfile -notmatch '#include\s+"IMU_2\.h"' },
    @{ Name = "merged IMU keeps AHRS public API"; Pass = $imuH -match "typedef\s+struct\s*\{[\s\S]*?\}\s*euler_param_t\s*;" -and $imuH -match "extern\s+euler_param_t\s+euler_angle" -and $imuH -match "void\s+Init_Gyro_Offset\s*\(\s*void\s*\)" -and $imuH -match "void\s+Get_Angles_ICM\s*\(\s*void\s*\)" },
    @{ Name = "merged IMU keeps AHRS implementation"; Pass = $imuC -match "void\s+Init_Gyro_Offset\s*\(\s*void\s*\)" -and $imuC -match "void\s+Get_Angles_ICM\s*\(\s*void\s*\)" -and $imuC -match "static\s+void\s+Update_AHRS_ICM" -and $imuC -match "euler_param_t\s+euler_angle" },
    @{ Name = "merged IMU preserves active Yaw_1 flow"; Pass = $imuC -match "float\s+Yaw_1\s*=\s*0" -and $imuC -match "void\s+IMU_GetValues\s*\(\s*void\s*\)" -and $imuC -match "imu963ra_gyro_z" }
)

$failed = @($checks | Where-Object { -not $_.Pass })
foreach($check in $checks)
{
    if($check.Pass)
    {
        Write-Host "[PASS] $($check.Name)"
    }
    else
    {
        Write-Host "[FAIL] $($check.Name)"
    }
}

if($failed.Count -gt 0)
{
    throw "portion2 serial debug verification failed: $($failed.Count) check(s)"
}
