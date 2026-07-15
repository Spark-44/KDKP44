Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

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

$repo = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw -Path (Join-Path $repo 'user/cpu0_main.c')

Assert-Match $main '(?m)^\s*dot_matrix_screen_init\s*\(\s*\)\s*;' `
    'dot matrix screen must be initialized at startup.'
Assert-Match $main '(?m)^\s*Portion2_Dot_Matrix_Scan_Update\s*\(\s*\)\s*;' `
    'RUN mode must continuously scan the dot matrix screen.'
Assert-Match $main 'Portion2_Aux_Start\s*\(\s*data\s*-\s*''A''\s*\+\s*1\s*\)' `
    'A-F serial commands must still select the six dot matrix light patterns.'
Assert-NotMatch $main 'offline_voice_init\s*\(\s*Portion2_Voice_Command_Handle,\s*0\s*\)' `
    'offline voice must not initialize UART1 when dot matrix TLD7002 is enabled.'
Assert-NotMatch $main 'offline_voice_poll\s*\(\s*\)\s*;' `
    'offline voice polling must stay disabled when UART1 is reserved for dot matrix TLD7002.'

Write-Host 'portion2 dot matrix RUN checks passed.'
