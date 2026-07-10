$ErrorActionPreference = 'Stop'

$root = Split-Path -Parent $PSScriptRoot
$main = Get-Content -Raw (Join-Path $root 'user\cpu0_main.c')

function Assert-Contains([string]$Text, [string]$Pattern, [string]$Message) {
    if ($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $main 'static\s+void\s+Portion2_Ascii_Enter_Run_Mode\s*\(\s*void\s*\)' 'serial route commands must have a helper that enters run mode without depending on the remote.'
Assert-Contains $main 'Portion2_Ascii_Enter_Run_Mode\s*\(\s*void\s*\)[\s\S]*?main_mode\s*=\s*Guandao_Voice[\s\S]*?route_setting_choice\s*=\s*3[\s\S]*?conrtol_mode\s*=\s*GUANDAO' 'serial route helper must switch to run mode and GUANDAO control.'
Assert-Contains $main "data\s*>=\s*'1'\s*&&\s*data\s*<=\s*'9'[\s\S]*?Portion2_Ascii_Enter_Run_Mode\s*\(\s*\)[\s\S]*?portion2_run_select_route\s*\(\s*data\s*-\s*'1'\s*\)" 'serial numeric route commands must enter run mode before selecting routes.'
Assert-Contains $main "data\s*>=\s*'U'\s*&&\s*data\s*<=\s*'V'[\s\S]*?Portion2_Ascii_Enter_Run_Mode\s*\(\s*\)[\s\S]*?portion2_run_select_route" 'serial U/V route commands must enter run mode before selecting routes.'
Assert-Contains $main "data\s*==\s*'W'[\s\S]*?Portion2_Ascii_Enter_Run_Mode\s*\(\s*\)[\s\S]*?portion2_run_select_route" 'serial W route command must enter run mode before selecting routes.'
Assert-Contains $main "data\s*>=\s*'u'\s*&&\s*data\s*<=\s*'w'[\s\S]*?Portion2_Ascii_Enter_Run_Mode\s*\(\s*\)[\s\S]*?portion2_run_select_route" 'serial u/v/w route commands must enter run mode before selecting routes.'

Write-Output 'Serial route run-mode entry checks passed.'
