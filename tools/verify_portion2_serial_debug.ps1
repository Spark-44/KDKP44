$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$guandaoC = Get-Content -Raw -Path (Join-Path $root "code\guandao.c")
$guandaoH = Get-Content -Raw -Path (Join-Path $root "code\guandao.h")
$cpu0 = Get-Content -Raw -Path (Join-Path $root "user\cpu0_main.c")
$controlH = Get-Content -Raw -Path (Join-Path $root "code\control.h")
$displayC = Get-Content -Raw -Path (Join-Path $root "code\display.c")
$displayH = Get-Content -Raw -Path (Join-Path $root "code\display.h")
$flashC = Get-Content -Raw -Path (Join-Path $root "code\flash.c")
$fixedActionC = Get-Content -Raw -Path (Join-Path $root "code\subject_2_fixed_action.c")
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
    @{ Name = "runtime serial log emitted"; Pass = $guandaoC -match "\[P2-RUN\]" },
    @{ Name = "route dump serial log emitted"; Pass = $guandaoC -match "\[P2-DUMP\]" },
    @{ Name = "per-route saved flags exist"; Pass = $guandaoC -match "portion2_route_saved_flag\s*\[\s*PORTION2_ROUTE_COUNT\s*\]" },
    @{ Name = "route dump includes saved state"; Pass = $guandaoC -match "saved=%s" },
    @{ Name = "single saved flag removed"; Pass = $guandaoC -notmatch "portion2_saved_flag" },
    @{ Name = "route switching does not clear saved flag"; Pass = $guandaoC -notmatch "portion2_route_saved_flag\s*\[\s*portion2_record_route\s*\]\s*=\s*0\s*;\s*Buzzer_check\(20\)" },
    @{ Name = "boot read is not immediately reset"; Pass = $cpu0 -notmatch "Flash_Main_Read\s*\(\s*\)\s*;\s*[\r\n\s]*main_mode\s*=\s*Guandao_Portion2_Recode\s*;\s*[\r\n\s]*route_setting_choice\s*=\s*1\s*;\s*[\r\n\s]*conrtol_mode\s*=\s*YAOKONG\s*;\s*[\r\n\s]*portion2_record_reset\s*\(\s*\)" },
    @{ Name = "loaded routes are marked saved after boot read"; Pass = $cpu0 -match "Flash_Main_Read\s*\(\s*\)\s*;\s*[\r\n\s]*portion2_record_mark_loaded_routes_saved\s*\(\s*\)" },
    @{ Name = "menu entering record mode preserves routes"; Pass = $displayC -notmatch "Guandao_Portion2_Recode[^;\r\n]*;[^;\r\n]*route_setting_choice[^;\r\n]*;[^;\r\n]*conrtol_mode[^;\r\n]*;[^;\r\n]*portion2_record_reset\s*\(\s*\)" },
    @{ Name = "record K4 short clears release flag before run mode"; Pass = $guandaoC -match "if\s*\(\s*k4_short\s*\)\s*\{[\s\S]*?portion2_record_key_state_reset\s*\(\s*\)[\s\S]*?main_mode\s*=\s*Guandao_Voice" },
    @{ Name = "run K4 long press condition is active code"; Pass = $cpu0 -match "if\s*\(\s*gpio_get_level\s*\(\s*KEY4\s*\)\s*==\s*0\s*\)" },
    @{ Name = "no commented-out key condition remains"; Pass = $cpu0 -notmatch "//[^\r\n]*if\s*\(\s*gpio_get_level\s*\(\s*KEY[14]\s*\)" },
    @{ Name = "run motor enable gate removed"; Pass = $cpu0 -notmatch "run_motor_enabled" },
    @{ Name = "run UI no longer shows motor enable"; Pass = $cpu0 -notmatch "MOTOR:\s*ON|MOTOR:\s*OFF|K1 MOTOR" },
    @{ Name = "voice commands ignored outside run mode"; Pass = $cpu0 -match "static\s+void\s+Portion2_Voice_Command_Handle[\s\S]*?if\s*\(\s*main_mode\s*!=\s*Guandao_Voice\s*\)[\s\S]*?return\s*;" },
    @{ Name = "record DONE state removed"; Pass = $recordTaskBody -ne "" -and $recordTaskBody -notmatch "case\s+3\s*:" -and $guandaoC -notmatch '"DONE\s*"' },
    @{ Name = "record route overflow clamps to last route and WAIT"; Pass = $recordTaskBody -match "if\s*\(\s*portion2_record_route\s*>=\s*PORTION2_ROUTE_COUNT\s*\)\s*\{[\s\S]*?portion2_record_route\s*=\s*PORTION2_ROUTE_COUNT\s*-\s*1\s*;[\s\S]*?portion2_record_state\s*=\s*2\s*;" },
    @{ Name = "record key state reset API declared"; Pass = $guandaoH -match "void\s+portion2_record_enter_mode\s*\(\s*void\s*\)" },
    @{ Name = "record key state reset API implemented"; Pass = $guandaoC -match "void\s+portion2_record_enter_mode\s*\(\s*void\s*\)[\s\S]*?portion2_record_key_state_reset\s*\(\s*\)" },
    @{ Name = "record key reset does not clear route data"; Pass = $recordKeyResetBody -ne "" -and $recordKeyResetBody -notmatch "portion2_route_length\s*\[" -and $recordKeyResetBody -notmatch "portion2_route_gps_count\s*\[" -and $recordKeyResetBody -notmatch "guandao_state_init" },
    @{ Name = "record K4 enters run through shared transition"; Pass = $guandaoC -match "if\s*\(\s*k4_short\s*\)\s*\{[\s\S]*?portion2_record_key_state_reset\s*\(\s*\)[\s\S]*?main_mode\s*=\s*Guandao_Voice" },
    @{ Name = "record K4 accepts global key event fallback"; Pass = $guandaoC -match "if\s*\(\s*key4_flag\s*\)\s*\{[\s\S]*?key4_flag\s*=\s*0\s*;[\s\S]*?k4_short\s*=\s*1" },
    @{ Name = "run K4 returns to record through shared transition"; Pass = $cpu0 -match "if\s*\(\s*key4_flag\s*\)[\s\S]*?portion2_record_enter_mode\s*\(\s*\)" },
    @{ Name = "last route completion stays in WAIT"; Pass = $recordTaskBody -match "if\s*\(\s*portion2_record_route\s*\+\s*1\s*<\s*PORTION2_ROUTE_COUNT\s*\)[\s\S]*?portion2_record_route\s*\+\+[\s\S]*?portion2_record_state\s*=\s*2" -and $recordTaskBody -notmatch "portion2_record_state\s*=\s*\([^;\r\n]*PORTION2_ROUTE_COUNT[^;\r\n]*\)\s*\?\s*3\s*:\s*2" },
    @{ Name = "run main UI does not overwrite lower debug rows"; Pass = $runUi -ne "" -and $runUi -notmatch "ips200_show_string\s*\(\s*X\s*\(\s*1\s*\)\s*,\s*Y\s*\(\s*1[2-5]\s*\)" },
    @{ Name = "route run completeness helper exists"; Pass = $guandaoC -match "static\s+uint8\s+portion2_route_ready_for_run\s*\(\s*uint8\s+route_id\s*\)" },
    @{ Name = "route run requires enough inertial points"; Pass = $guandaoC -match "portion2_route_length\s*\[\s*route_id\s*\]\s*<\s*required" },
    @{ Name = "route run requires enough gps points"; Pass = $guandaoC -match "portion2_route_gps_count\s*\[\s*route_id\s*\]\s*<\s*required" },
    @{ Name = "incomplete route rejected before run"; Pass = $guandaoC -match "portion2_run_reject_reason\s*=\s*4" -and $guandaoC -match "if\s*\(\s*!\s*portion2_route_ready_for_run\s*\(\s*route_id\s*\)\s*\)" },
    @{ Name = "default base speed is 10"; Pass = $guandaoC -match "float\s+base_speed\s*=\s*10\.0f\s*;" },
    @{ Name = "default forward control speed is 10"; Pass = $flashC -match "int16\s+control\s*\[\s*5\s*\]\s*=\s*\{\s*10\s*," },
    @{ Name = "flash forward speed forced to 10"; Pass = $flashC -match "control\s*\[\s*0\s*\]\s*=\s*10\s*;" },
    @{ Name = "record start keeps selected route"; Pass = $recordStartCases -ne "" -and $recordStartCases -notmatch "portion2_record_route\s*=\s*0\s*;" },
    @{ Name = "voice sleep command has no handler"; Pass = $cpu0 -notmatch "case\s+OFFLINE_VOICE_CMD_SLEEP\s*:" },
    @{ Name = "voice sleep no longer stops vehicle"; Pass = $cpu0 -notmatch "case\s+OFFLINE_VOICE_CMD_SLEEP\s*:[\s\S]*?(voice_drive_action_stop|portion2_run_stop|rear_motor_stop)\s*\(" },
    @{ Name = "fixed turn timeout is 60 seconds"; Pass = $fixedActionC -match "SUBJECT_2_FIXED_TURN_TIMEOUT_MS\s+\(60000U\)" -and $fixedActionC -match "elapsed_ms\s*>=\s*SUBJECT_2_FIXED_TURN_TIMEOUT_MS" },
    @{ Name = "fixed turn logs yaw delta"; Pass = $fixedActionC -match "\[FIXED\]" -and $fixedActionC -match "voice_drive_action_get_yaw_delta" },
    @{ Name = "smooth plan keeps sharp corners linear"; Pass = $guandaoC -match "keep_corner_linear" -and $guandaoC -match "GUANDAO_SHARP_TURN_ANGLE" },
    @{ Name = "smooth pursuit uses local arrive threshold"; Pass = $guandaoC -match "float\s+arrive_threshold\s*=\s*persuit_threshold" -and $guandaoC -match "distance_to_target\s*<=\s*arrive_threshold" -and $guandaoC -notmatch "persuit_threshold\s*=\s*persuit_threshold\s*\*" },
    @{ Name = "smooth pursuit dynamically extends preview"; Pass = $guandaoC -match "int\s+steer_preview_steps\s*=\s*preview_spets" -and $guandaoC -match "steer_preview_steps\s*<\s*4" -and $guandaoC -match "steer_preview_steps\s*<\s*8" },
    @{ Name = "smooth pursuit uses curve preview"; Pass = $guandaoC -match "int\s+curve_preview_steps\s*=\s*5" -and $guandaoC -match "pursuit_midhandle\s*\(\s*state\s*,\s*&current_point\s*,\s*curve_preview_steps" },
    @{ Name = "smooth pursuit rate limits steering target"; Pass = $guandaoC -match "last_target_steering" -and $guandaoC -match "steer_delta_limit" -and $guandaoC -match "last_steer_limit_ms" },
    @{ Name = "smooth pursuit reduces speed from preview angle"; Pass = $guandaoC -match "fabsf\s*\(\s*preview_alpha2\s*\)\s*>\s*GUANDAO_CURVE_TRIGGER_ANGLE" -and $guandaoC -match "curve_scale" },
    @{ Name = "smooth pursuit outputs limited target steering"; Pass = $guandaoC -match "\*out_servo\s*=\s*-target_steering" },
    @{ Name = "curve speed can fall below base speed"; Pass = $guandaoH -match "MIN_SPEED\s+4\.0f" },
    @{ Name = "portion2 reuses portion3 trace standard"; Pass = $guandaoC -match "static\s+uint8\s+guandao_uses_portion3_trace_standard\s*\(\s*guandao_state\s+\*state\s*\)" -and $guandaoC -match "state\s*==\s*&portion_2" },
    @{ Name = "portion3 standard arrival threshold"; Pass = $guandaoC -match "PORTION3_PURSUIT_THRESHOLD\s+0\.25f" -and $guandaoC -match "guandao_uses_portion3_trace_standard\s*\(\s*state\s*\)[\s\S]*?arrive_threshold\s*=\s*PORTION3_PURSUIT_THRESHOLD" },
    @{ Name = "portion3 standard final stop"; Pass = $guandaoC -match "PORTION3_FINAL_STOP_DIST\s+0\.6f" -and $guandaoC -match "guandao_uses_portion3_trace_standard\s*\(\s*state\s*\)[\s\S]*?dist_to_final\s*<=\s*PORTION3_FINAL_STOP_DIST" },
    @{ Name = "portion3 standard disables yaw correction"; Pass = $guandaoC -match "case\s+2\s*:\s*[\r\n\s]*break\s*;" },
    @{ Name = "portion3 standard endpoint slowdown after curve slowdown"; Pass = $guandaoC -match "curve_scale[\s\S]*?dist_to_final\s*<\s*final_dsts[\s\S]*?v_center\s*=\s*base_speed\s*\*\s*\(dist_to_final\s*/\s*final_dsts\)" },
    @{ Name = "main modes only include record and run"; Pass = $displayH -match "typedef\s+enum\s*\{\s*Guandao_Voice\s*,\s*Guandao_Portion2_Recode\s*\}\s*Mode_Choice\s*;" },
    @{ Name = "removed main modes are not referenced"; Pass = ($cpu0 + $displayC + $displayH + $guandaoC) -notmatch "Mode_IDLE|Guandao_Recode_Mode|Guandao_portion_1|Guandao_portion_3|Rack_Test_Mode|YaoKong_Mode" },
    @{ Name = "record mode uses IDLE control"; Pass = $cpu0 -match "main_mode\s*=\s*Guandao_Portion2_Recode\s*;\s*[\r\n\s]*route_setting_choice\s*=\s*1\s*;\s*[\r\n\s]*conrtol_mode\s*=\s*IDLE\s*;" -and $cpu0 -notmatch "conrtol_mode\s*=\s*YAOKONG" },
    @{ Name = "control modes keep only active controls"; Pass = $controlH -match "typedef\s+enum\s*\{\s*IDLE\s*,\s*GUANDAO\s*,\s*DAOCHE\s*\}\s*MOTER_control_mode\s*;" },
    @{ Name = "removed control modes are not referenced"; Pass = ($cpu0 + $displayC + $controlH) -notmatch "\bYAOKONG\b|\bRACK_TEST\b|\bGPS\b" },
    @{ Name = "old record preview branch removed"; Pass = $guandaoC -notmatch "main_mode\s*==\s*Guandao_Recode_Mode" }
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
