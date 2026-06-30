$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$displayH = Get-Content -Raw (Join-Path $repo 'code/display.h')
$main = Get-Content -Raw (Join-Path $repo 'user/cpu0_main.c')
$guandaoC = Get-Content -Raw (Join-Path $repo 'code/guandao.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $displayH 'Guandao_Drive' 'main mode enum must include drive mode'
Assert-Contains $main '#define\s+PORTION2_DRIVE_SPEED_STEP_MPS\s+\(1\.0f\)' 'drive mode speed step must be 1.0m/s'
Assert-Contains $main 'static\s+float\s+portion2_drive_target_mps\s*=\s*0\.0f' 'drive mode must keep a target speed starting at 0'
Assert-Contains $main 'static\s+void\s+Portion2_Drive_Mode_Enter\s*\(\s*void\s*\)' 'drive mode must have an enter helper'
Assert-Contains $main 'portion2_drive_target_mps\s*=\s*0\.0f;[\s\S]*?main_mode\s*=\s*Guandao_Drive' 'entering drive mode must reset speed and switch mode'
Assert-Contains $main 'static\s+void\s+Portion2_Drive_Mode_Key_Handle\s*\(\s*void\s*\)' 'drive mode must handle keys'
Assert-Contains $main 'key2_flag[\s\S]*?portion2_drive_target_mps\s*\+=\s*PORTION2_DRIVE_SPEED_STEP_MPS' 'K2 must increase drive speed by 1m/s'
Assert-Contains $main 'key1_flag[\s\S]*?portion2_drive_target_mps\s*-=\s*PORTION2_DRIVE_SPEED_STEP_MPS' 'K1 must decrease drive speed by 1m/s'
Assert-Contains $main 'if\(portion2_drive_target_mps\s*<\s*0\.0f\)[\s\S]*?portion2_drive_target_mps\s*=\s*0\.0f' 'drive speed must not go below zero'
Assert-Contains $main 'portion2_drive_k4_long_event\(\)[\s\S]*?main_mode\s*=\s*Guandao_Portion2_Recode' 'drive long K4 must return to record mode'
Assert-Contains $main 'case\s+Guandao_Drive:' 'main loop must dispatch drive mode'
Assert-Contains $main 'rear_motor_set_target_mps\(portion2_drive_target_mps\)' 'drive task must command rear motor target speed directly'
Assert-Contains $main 'ips200_show_string\(X\(1\),\s*Y\(0\),\s*"MODE: DRIVE"\)' 'drive mode must display its mode'
Assert-Contains $guandaoC 'main_mode\s*=\s*Guandao_Drive' 'record long K4 must enter drive mode'
Assert-Contains $guandaoC 'portion2_mode_key_transition_lock\(\);[\s\S]*?main_mode\s*=\s*Guandao_Drive' 'record to drive transition must lock K4 until release'
if ($main -match 'static\s+void\s+Portion2_Run_Mode_Key_Handle\s*\(\s*void\s*\)\s*\{([\s\S]*?)\r?\n\}\r?\n\r?\nstatic\s+void\s+Portion2_Run_Mode_UI_Update') {
    if ($Matches[1] -match 'Guandao_Drive') {
        throw 'run key handler must not switch directly to drive mode'
    }
} else {
    throw 'run key handler block not found'
}

Write-Host 'drive mode checks passed'
