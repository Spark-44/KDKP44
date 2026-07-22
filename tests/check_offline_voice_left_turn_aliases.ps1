$ErrorActionPreference = 'Stop'

$root = Join-Path $PSScriptRoot '..'
$header = Get-Content -Raw (Join-Path $root 'code\offline_voice.h')
$main = Get-Content -Raw (Join-Path $root 'user\cpu0_main.c')

foreach($pattern in @(
    '#define\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1\s+\(0x2D\)',
    '#define\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2\s+\(0x2E\)',
    '#define\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3\s+\(0x2F\)',
    '#define\s+OFFLINE_VOICE_CMD_MAX\s+\(0x2F\)'
)) {
    if($header -notmatch $pattern) {
        throw "Missing left-turn voice alias definition: $pattern"
    }
}

$sharedBranch = 'case\s+OFFLINE_VOICE_CMD_TURN_LEFT_DRIVE\s*:\s*case\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_1\s*:\s*case\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_2\s*:\s*case\s+OFFLINE_VOICE_CMD_TURN_LEFT_ALIAS_3\s*:\s*Portion2_Fixed_Action_Start\s*\(\s*VOICE_DRIVE_ACTION_TURN_LEFT\s*\)\s*;'
if($main -notmatch $sharedBranch) {
    throw 'All four left-turn drive command IDs must share the existing left-turn action branch.'
}

Write-Output 'Run-mode left-turn voice aliases are mapped to the existing left-turn action.'
