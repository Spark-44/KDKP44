$ErrorActionPreference = 'Stop'

$fixed = Get-Content -Raw (Join-Path $PSScriptRoot '..\code\subject_2_fixed_action.c')

foreach($pattern in @(
    'static\s+void\s+subject_2_encoder_yaw_update_steer\s*\(\s*uint32\s+now_ms\s*,\s*uint8\s+reverse\s*\)',
    'static\s+void\s+subject_2_turn_pre_straight_task\s*\(\s*uint32\s+now_ms\s*\)',
    'phase_distance_m\s*\+=\s*distance_step',
    'phase_distance_m\s*>=\s*SUBJECT_2_FIXED_TURN_PRE_DISTANCE_M',
    'subject_2_encoder_yaw_update_steer\s*\(\s*now_ms\s*,\s*0U\s*\)',
    'subject_2_fixed_is_turn_action\s*\(\s*subject_2_fixed_action_state\.mode\s*\)[\s\S]*?turn_phase\s*==\s*SUBJECT_2_FIXED_TURN_PHASE_STRAIGHT[\s\S]*?subject_2_turn_pre_straight_task\s*\(\s*now_ms\s*\)'
)) {
    if($fixed -notmatch $pattern) {
        throw "Missing turn pre-straight yaw behavior: $pattern"
    }
}

Write-Output 'Turn pre-straight yaw checks passed.'
