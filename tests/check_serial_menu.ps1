param(
    [string]$Root = (Join-Path $PSScriptRoot '..')
)

$mainPath = Join-Path $Root 'user\cpu0_main.c'
$headerPath = Join-Path $Root 'code\serial_menu.h'
$sourcePath = Join-Path $Root 'code\serial_menu.c'

if(!(Test-Path $headerPath)) { throw 'Missing code\serial_menu.h' }
if(!(Test-Path $sourcePath)) { throw 'Missing code\serial_menu.c' }

$main = Get-Content -Raw $mainPath
$header = Get-Content -Raw $headerPath
$source = Get-Content -Raw $sourcePath

function Assert-Contains($Text, $Pattern, $Message) {
    if($Text -notmatch $Pattern) {
        throw $Message
    }
}

Assert-Contains $header 'uint8\s+serial_menu_handle_byte\s*\(\s*uint8\s+data\s*\)' 'serial menu must expose a byte handler that reports consumed input.'
Assert-Contains $source '\[SERIAL-MENU\]' 'serial menu output must be clearly tagged.'
Assert-Contains $source 'M/w/s/e/b' 'serial menu help must document the keyboard mapping.'
Assert-Contains $source 'main_mode\s*=\s*Guandao_Voice[\s\S]*?route_setting_choice\s*=\s*3[\s\S]*?conrtol_mode\s*=\s*GUANDAO' 'serial menu must be able to switch to run mode without using the screen menu.'
Assert-Contains $source 'main_mode\s*=\s*Guandao_Portion2_Recode[\s\S]*?route_setting_choice\s*=\s*1[\s\S]*?conrtol_mode\s*=\s*IDLE' 'serial menu must be able to switch to record mode without using the screen menu.'
Assert-Contains $source 'main_mode\s*=\s*Guandao_Drive[\s\S]*?route_setting_choice\s*=\s*1[\s\S]*?conrtol_mode\s*=\s*GUANDAO' 'serial menu must be able to switch to drive mode without using the screen menu.'
Assert-Contains $main '#include\s+"serial_menu\.h"' 'cpu0_main must include serial_menu.h.'
Assert-Contains $main 'if\s*\(\s*serial_menu_handle_byte\s*\(\s*data\s*\)\s*\)[\s\S]*?continue\s*;' 'serial menu must get first chance to consume menu bytes.'
Assert-Contains $main 'Portion2_Serial_Command_Update\s*\(\s*\);[\s\S]*?switch\s*\(\s*main_mode\s*\)' 'serial commands must be polled before the mode switch so the menu works when the screen does not.'

Write-Output 'Serial menu checks passed.'
