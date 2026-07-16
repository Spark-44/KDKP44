Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Assert-NotMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if($Text -match $Pattern) {
        throw $Message
    }
}

function Assert-Match {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

$repo = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw -Path (Join-Path $repo 'user/cpu0_main.c')
$fixed = Get-Content -Raw -Path (Join-Path $repo 'code/subject_2_fixed_action.c')
$voice = Get-Content -Raw -Path (Join-Path $repo 'code/voice_drive_action.h')
$guandao = Get-Content -Raw -Path (Join-Path $repo 'code/guandao.c')

Assert-NotMatch $main "data\s*>?=\s*'I'\s*&&\s*data\s*<=\s*'P'" `
    'serial range I-P must not remain because I/J/K/L legacy actions were removed.'
Assert-NotMatch $main "data\s*>?=\s*'i'\s*&&\s*data\s*<=\s*'p'" `
    'serial range i-p must not remain because i/j/k/l legacy actions were removed.'
Assert-NotMatch $main "data\s*==\s*'X'|data\s*==\s*'x'|data\s*==\s*'y'|data\s*==\s*'Y'" `
    'serial commands X/x/y/Y for gyro routes 13/14 must be removed.'
Assert-NotMatch $main 'subject_2_gyro_route_' `
    'main loop must not call removed gyro route 13/14 runtime.'
Assert-NotMatch $guandao 'subject_2_gyro_route_' `
    'recorded-route code must not stop removed gyro route 13/14 runtime.'
Assert-NotMatch $fixed 'subject_2_gyro_route_' `
    'fixed-action code must not stop removed gyro route 13/14 runtime.'

foreach($symbol in @(
    'VOICE_DRIVE_ACTION_FORWARD_10M',
    'VOICE_DRIVE_ACTION_BACKWARD_10M',
    'VOICE_DRIVE_ACTION_SNAKE_FORWARD_10M',
    'VOICE_DRIVE_ACTION_SNAKE_BACKWARD_10M'
)) {
    Assert-NotMatch $voice $symbol "legacy fixed-action enum remains: $symbol"
    Assert-NotMatch $fixed $symbol "legacy fixed-action implementation remains: $symbol"
}

foreach($file in @('code/subject_2_gyro_route.c', 'code/subject_2_gyro_route.h')) {
    if(Test-Path (Join-Path $repo $file)) {
        throw "removed gyro route 13/14 file still exists: $file"
    }
}

Assert-Match $main "data\s*>?=\s*'M'\s*&&\s*data\s*<=\s*'P'" `
    'serial commands M/N/O/P must remain for circle and turn fixed actions.'
Assert-Match $main 'OFFLINE_VOICE_CMD_CCW_CIRCLE[\s\S]*?VOICE_DRIVE_ACTION_CCW_CIRCLE' `
    'voice circle/turn commands must use explicit action mapping after legacy enum removal.'

Write-Host 'legacy serial action removal checks passed.'
