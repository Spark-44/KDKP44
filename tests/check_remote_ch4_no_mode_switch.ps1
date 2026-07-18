Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repo = Split-Path -Parent $PSScriptRoot
$remote = Get-Content -Raw (Join-Path $repo 'code/remote_control.c')

if($remote -match 'remote_control_update_mode_from_ch4\s*\(') {
    throw 'CH4 mode switching implementation must be removed.'
}
if($remote -match 'channel\[3\][\s\S]*?main_mode\s*=') {
    throw 'CH4 must not write main_mode.'
}
if($remote -match 'remote_control_task[\s\S]*?remote_control_update_mode_from_ch4\s*\(') {
    throw 'Remote task must not invoke CH4 mode switching.'
}

Write-Host 'remote CH4 mode switching removal checks passed.'
