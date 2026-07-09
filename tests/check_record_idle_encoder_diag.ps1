$ErrorActionPreference = 'Stop'

function Assert-Contains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotContains {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )

    if($Text -match $Pattern) {
        throw $Message
    }
}

$root = Split-Path -Parent $PSScriptRoot
$main = Get-Content (Join-Path $root 'user\cpu0_main.c') -Raw
$loop = $main.Substring($main.IndexOf('while (TRUE)'))

Assert-Contains $main '#define\s+RECORD_IDLE_ENCODER_DIAG_PERIOD_MS\s+\(10U\)' 'Record idle encoder diagnostics must run every 10ms.'
Assert-Contains $main 'static\s+void\s+Record_Idle_Encoder_Diag_Update\s*\(\s*void\s*\)' 'Main loop must define record idle encoder diagnostic task.'
Assert-Contains $main 'if\s*\(\s*main_mode\s*!=\s*Guandao_Portion2_Recode\s*\)[\s\S]*?return\s*;' 'Encoder diagnostics must only run in record mode.'
Assert-Contains $main 'if\s*\(\s*remote_control_is_active\s*\(\s*\)\s*\)[\s\S]*?return\s*;' 'Encoder diagnostics must not run while remote control is active.'
Assert-Contains $main 'if\s*\(\s*conrtol_mode\s*!=\s*IDLE\s*\)[\s\S]*?return\s*;' 'Encoder diagnostics must only run while car control is idle.'
Assert-Contains $main 'rear_motor_encoder_update_10ms\s*\(\s*\)' 'Encoder diagnostics must sample the rear encoder.'
Assert-Contains $main '\[ENC-DIAG\][\s\S]*?enc10=%d[\s\S]*?enc100=%ld[\s\S]*?actual=%\.2f[\s\S]*?pwm=%d' 'Encoder diagnostics must print pulses, actual speed and PWM.'
Assert-NotContains $loop 'Record_Idle_Encoder_Diag_Update\s*\(\s*\)\s*;' 'Encoder diagnostics must not auto-refresh serial output from the main loop.'

Write-Host 'Record idle encoder diagnostic checks passed.'
