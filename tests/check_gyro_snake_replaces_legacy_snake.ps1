Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-Match {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

function Assert-NotMatch {
    param([string]$Text, [string]$Pattern, [string]$Message)
    if($Text -match $Pattern) {
        throw $Message
    }
}

$repo = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw -Path (Join-Path $repo 'user/cpu0_main.c')
$fixed = Get-Content -Raw -Path (Join-Path $repo 'code/subject_2_fixed_action.c')

Assert-Match $fixed '#define\s+SUBJECT_2_GYRO_SNAKE_DISTANCE_M\s+\(15\.0f\)' `
    'new gyro snake must be the retained 15m snake implementation.'

Assert-Match $main "data\s*==\s*'W'[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M" `
    'serial W must start the new forward gyro snake.'
Assert-Match $main "data\s*==\s*'w'[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M" `
    'serial w must start the new forward gyro snake.'
Assert-Match $main "data\s*==\s*'V'[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M" `
    'serial V must start the new reverse gyro snake.'
Assert-Match $main "data\s*==\s*'v'[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M" `
    'serial v must start the new reverse gyro snake.'
Assert-NotMatch $main "data\s*==\s*'Z'|data\s*==\s*'z'" `
    'dedicated Z/z gyro snake aliases must be removed after replacing the old snake commands.'

foreach($pattern in @(
    'OFFLINE_VOICE_CMD_ROUTE_SNAKE[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M',
    'OFFLINE_VOICE_CMD_SNAKE_FORWARD[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_FORWARD_15M',
    'OFFLINE_VOICE_CMD_BACK_SNAKE[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M',
    'OFFLINE_VOICE_CMD_SNAKE_BACKWARD[\s\S]*?VOICE_DRIVE_ACTION_GYRO_SNAKE_REVERSE_15M'
)) {
    Assert-Match $main $pattern "voice snake command must use new gyro snake: $pattern"
}

foreach($pattern in @(
    'OFFLINE_VOICE_CMD_ROUTE_SNAKE[\s\S]*?portion2_run_select_route\s*\(\s*PORTION2_ROUTE_SNAKE\s*\)',
    'OFFLINE_VOICE_CMD_SNAKE_FORWARD[\s\S]*?portion2_run_select_route\s*\(\s*PORTION2_ROUTE_SNAKE\s*\)',
    'OFFLINE_VOICE_CMD_BACK_SNAKE[\s\S]*?portion2_run_select_route\s*\(\s*PORTION2_ROUTE_STRAIGHT\s*\)',
    'OFFLINE_VOICE_CMD_SNAKE_BACKWARD[\s\S]*?portion2_run_select_route\s*\(\s*PORTION2_ROUTE_STRAIGHT\s*\)'
)) {
    Assert-NotMatch $main $pattern "old recorded-route snake mapping remains: $pattern"
}

Write-Host 'new gyro snake replaces legacy snake checks passed.'
