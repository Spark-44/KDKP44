$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$remote = Get-Content -Raw (Join-Path $repo 'code/remote_control.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $remote '#define\s+REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s+\(2\.0f\)' 'record-mode remote speed limit must be 2.0m/s.'
Assert-Contains $remote 'REMOTE_CONTROL_SPEED_FULL_SCALE\s*\*\s*REMOTE_CONTROL_MAX_TARGET_SPEED_MPS' 'remote stick mapping must use the record-mode speed limit.'
Assert-Contains $remote 'rear_motor_set_speed_limit_mps\s*\(\s*REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\)' 'record-mode rear motor governor must use the 2.0m/s limit constant.'
Assert-Contains $remote 'actual_mps\s*>=\s*\(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\+\s*REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\)' 'forward overspeed check must track the 2.0m/s limit.'
Assert-Contains $remote 'actual_mps\s*<=\s*-\(REMOTE_CONTROL_MAX_TARGET_SPEED_MPS\s*\+\s*REMOTE_CONTROL_SPEED_OVERSPEED_MARGIN_MPS\)' 'reverse overspeed check must track the 2.0m/s limit.'

Write-Output 'Record-mode 2mps speed limit checks passed.'
