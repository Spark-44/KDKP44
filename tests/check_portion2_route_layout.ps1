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

Assert-Contains $header '#define\s+PORTION2_ROUTE_COUNT\s+12' 'route count must be 12'
Assert-Contains $header '#define\s+PORTION2_TOTAL_ROUTE_POINTS\s+588' 'inertial capacity must be 12 x 49'
Assert-Contains $header '#define\s+PORTION2_TOTAL_GPS_COUNT\s+240' 'GPS capacity must be 12 x 20'
Assert-Contains $header '#define\s+PORTION2_ROUTE_RETURN_1\s+5' 'route 6 must replace old reverse route 1'
Assert-Contains $header '#define\s+PORTION2_ROUTE_RETURN_5\s+9' 'route 10 must replace old reverse route 5'
Assert-Contains $header '#define\s+PORTION2_ROUTE_STRAIGHT\s+10' 'route 11 must be the straight route'
Assert-Contains $header '#define\s+PORTION2_ROUTE_SNAKE\s+11' 'route 12 must be the snake route'
Assert-Contains $source 'const uint8 portion2_route_required_gps_count\[PORTION2_ROUTE_COUNT\]\s*=\s*\{(?:\s*5,){11}\s*5\s*\}' 'all 12 routes must require GPS points'
Assert-Contains $flash 'PORTION2_ROUTE_CONTINUATION_MAGIC' 'expanded inertial records must use a continuation flash page'
Assert-Contains $main '#define\s+PORTION2_ALL_LIGHT_PIN\s+P33_4' 'all-light output must use P33_4'
Assert-Contains $main 'gpio_init\(PORTION2_ALL_LIGHT_PIN,\s*GPO,\s*GPIO_LOW,\s*GPO_PUSH_PULL\)' 'P33_4 must start low as a push-pull output'
Assert-Contains $main 'OFFLINE_VOICE_CMD_INTERIOR_LIGHT[\s\S]*?Portion2_Aux_Start\(7\)' 'voice command 0x0A must start the all-light action'
Assert-Contains $main 'if\(mode\s*==\s*7\)[\s\S]*?gpio_high\(PORTION2_ALL_LIGHT_PIN\)' 'all-light action must drive P33_4 high'
Assert-Contains $main 'Portion2_Aux_Stop\(void\)[\s\S]*?gpio_low\(PORTION2_ALL_LIGHT_PIN\)' 'stopping an auxiliary action must drive P33_4 low'
Assert-Contains $main 'system_getval_ms\(\)\s*-\s*portion2_aux_start_ms\)\s*>=\s*10000' 'all-light action must share the 10-second timeout'
Assert-Contains $main 'portion2_mode_k4_short_event\(\)[\s\S]*?Portion2_Aux_Stop\(\)[\s\S]*?main_mode\s*=\s*Guandao_Portion2_Recode' 'leaving run mode must turn P33_4 off'
Assert-Contains $main "data\s*>=\s*'A'\s*&&\s*data\s*<=\s*'G'[\s\S]*?Portion2_Aux_Start\(data\s*-\s*'A'\s*\+\s*1\)" 'serial G must start the all-light action'
Assert-Contains $main "data\s*>=\s*'a'\s*&&\s*data\s*<=\s*'g'[\s\S]*?Portion2_Aux_Start\(data\s*-\s*'a'\s*\+\s*1\)" 'serial g must start the all-light action'

$voiceMappings = @(
    'OFFLINE_VOICE_CMD_GATE1_RIGHT_BACK[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_1\)',
    'OFFLINE_VOICE_CMD_GATE3_LEFT_BACK[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_RETURN_5\)',
    'OFFLINE_VOICE_CMD_FORWARD_10M[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_STRAIGHT\)',
    'OFFLINE_VOICE_CMD_BACKWARD_10M[\s\S]*?portion2_run_select_back_route\(PORTION2_ROUTE_STRAIGHT\)',
    'OFFLINE_VOICE_CMD_SNAKE_FORWARD[\s\S]*?portion2_run_select_route\(PORTION2_ROUTE_SNAKE\)',
    'OFFLINE_VOICE_CMD_SNAKE_BACKWARD[\s\S]*?portion2_run_select_back_route\(PORTION2_ROUTE_SNAKE\)'
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
Assert-Contains $fixed '#define\s+SUBJECT_2_FIXED_STRAIGHT_SPEED_MPS\s+\(0\.35f\)' 'unrequested fixed straight and snake actions must remain at 0.35m/s'
Assert-Contains $fixed '\{VOICE_DRIVE_ACTION_FORWARD_10M,\s*SUBJECT_2_FIXED_STRAIGHT_SPEED_MPS' 'fixed straight action must use its separate speed'
Assert-Contains $fixed '#define\s+SUBJECT_2_ENCODER_YAW_SPEED_MPS\s+\(1\.0f\)' 'encoder/yaw straight speed must be 1.0m/s'

Write-Host 'portion-2 12-route layout checks passed'
