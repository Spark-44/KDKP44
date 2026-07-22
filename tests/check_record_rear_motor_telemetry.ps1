$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw (Join-Path $root 'user/cpu0_main.c')

function Assert-Match([string]$Text, [string]$Pattern, [string]$Message) {
    if($Text -notmatch $Pattern) { throw $Message }
}

Assert-Match $main '#define\s+RECORD_REAR_TELEMETRY_PERIOD_MS\s+\(200U\)' `
    'Record rear telemetry must use a 200ms period.'
Assert-Match $main 'static\s+void\s+Record_Rear_Motor_Telemetry_Update\s*\(\s*void\s*\)' `
    'Record rear telemetry update function is missing.'
Assert-Match $main 'Record_Rear_Motor_Telemetry_Update[\s\S]*?main_mode\s*!=\s*Guandao_Portion2_Recode[\s\S]*?return\s*;' `
    'Record rear telemetry must be guarded by Record mode.'
Assert-Match $main 'Record_Rear_Motor_Telemetry_Update[\s\S]*?!remote_control_is_active\s*\(\s*\)[\s\S]*?return\s*;' `
    'Record rear telemetry must require active remote control.'
Assert-Match $main '\[REC-REAR\]\s+speed=%\.3fmps\s+pwm=%d' `
    'Record rear telemetry output format is missing.'
Assert-Match $main '\[REC-REAR\][\s\S]*?rear_motor_get_speed_mps\s*\(\s*\)[\s\S]*?rear_motor_get_pwm\s*\(\s*\)' `
    'Record rear telemetry must read actual speed and PWM.'
Assert-Match $main 'case\s+Guandao_Portion2_Recode\s*:[\s\S]*?portion2_record_task\s*\(\s*\)\s*;[\s\S]*?Record_Rear_Motor_Telemetry_Update\s*\(\s*\)\s*;' `
    'Record mode must schedule rear telemetry after the record task.'
Assert-Match $main 'case\s+Guandao_Portion2_Recode\s*:[\s\S]*?Record_Idle_Encoder_Diag_Update\s*\(\s*\)\s*;' `
    'Existing idle encoder diagnostics must remain scheduled.'

Write-Host 'Record rear motor telemetry checks passed.'
