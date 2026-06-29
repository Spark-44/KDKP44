$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$display = Get-Content -Raw (Join-Path $repo 'code/display.h')
$header = Get-Content -Raw (Join-Path $repo 'code/guandao.h')
$source = Get-Content -Raw (Join-Path $repo 'code/guandao.c')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $display 'Guandao_Voice\s*,\s*Guandao_Portion2_Recode\s*,\s*Guandao_Drive' 'main modes must include DRIVE'
Assert-Contains $header 'PORTION2_MODE_KEY_NONE[\s\S]*?PORTION2_MODE_KEY_SHORT[\s\S]*?PORTION2_MODE_KEY_LONG' 'K4 must expose none short and long events'
Assert-Contains $header 'portion2_mode_key_event_t\s+portion2_mode_k4_event\(void\)' 'K4 event API must be declared'
Assert-Contains $source 'portion2_mode_key_event_t\s+portion2_mode_k4_event\(void\)[\s\S]*?>=\s*1500U[\s\S]*?return\s+PORTION2_MODE_KEY_LONG' 'K4 must emit long after 1.5 seconds'
Assert-Contains $source 'k4_event\s*==\s*PORTION2_MODE_KEY_SHORT[\s\S]*?main_mode\s*=\s*Guandao_Voice' 'record short K4 must enter RUN'
Assert-Contains $source 'k4_event\s*==\s*PORTION2_MODE_KEY_LONG[\s\S]*?main_mode\s*=\s*Guandao_Drive' 'record long K4 must enter DRIVE'

Assert-Contains $main '#define\s+PORTION2_DRIVE_SPEED_STEP_MPS\s+\(1\.0f\)' 'DRIVE speed step must be 1m/s'
Assert-Contains $main '#define\s+PORTION2_DRIVE_MAX_SPEED_MPS\s+\(5\.0f\)' 'DRIVE maximum speed must be 5m/s'
Assert-Contains $main 'portion2_drive_target_mps\s*=\s*0\.0f' 'DRIVE must initialize stopped'
Assert-Contains $main 'key1_flag[\s\S]*?portion2_drive_target_mps\s*-=' 'DRIVE K1 must reduce speed'
Assert-Contains $main 'key2_flag[\s\S]*?portion2_drive_target_mps\s*\+=' 'DRIVE K2 must increase speed'
Assert-Contains $main 'portion2_drive_target_mps\s*<\s*0\.0f[\s\S]*?portion2_drive_target_mps\s*=\s*0\.0f' 'DRIVE speed must clamp at zero'
Assert-Contains $main 'portion2_drive_target_mps\s*>\s*PORTION2_DRIVE_MAX_SPEED_MPS[\s\S]*?portion2_drive_target_mps\s*=\s*PORTION2_DRIVE_MAX_SPEED_MPS' 'DRIVE speed must clamp at five'
Assert-Contains $main 'out_v_l\s*=\s*portion2_drive_target_mps\s*/\s*GUANDAO_SPEED_TO_MPS[\s\S]*?out_v_r\s*=\s*out_v_l[\s\S]*?out_servo\s*=\s*0\.0f' 'DRIVE must command equal wheels and centered steering'
Assert-Contains $main 'PORTION2_MODE_KEY_LONG[\s\S]*?rear_motor_stop\(\)[\s\S]*?main_mode\s*=\s*Guandao_Portion2_Recode' 'DRIVE long K4 must stop and return to RECORD'
Assert-Contains $main 'case\s+Guandao_Drive\s*:[\s\S]*?Portion2_Drive_Mode_Task\(\)' 'main loop must run DRIVE task'
Assert-Contains $main 'Portion2_Run_Mode_Key_Handle\(void\)[\s\S]*?PORTION2_MODE_KEY_SHORT[\s\S]*?main_mode\s*=\s*Guandao_Portion2_Recode' 'RUN short K4 must return to RECORD'

if ($main -match '#include\s+"zf_common_headfile\.h"\s*\+') {
    throw 'cpu0 main include must not contain trailing plus signs'
}

Write-Host 'drive mode checks passed'
