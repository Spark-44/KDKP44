$ErrorActionPreference = 'Stop'

$sourcePath = Join-Path $PSScriptRoot '..\code\guandao.c'
$source = Get-Content -Raw $sourcePath

$requiredDefinitions = @{
    PORTION2_SNAKE_STEERING_GAIN = '0.80f'
    PORTION2_SNAKE_STRAIGHT_SPEED = '10.0f'
    PORTION2_SNAKE_CURVE_SPEED = '8.0f'
    PORTION2_SNAKE_SHARP_SPEED = '6.0f'
    PORTION2_SNAKE_STRAIGHT_PREVIEW_STEPS = '18'
    PORTION2_SNAKE_STRAIGHT_CURVE_PREVIEW_STEPS = '28'
    PORTION2_SNAKE_CURVE_PREVIEW_STEPS = '12'
    PORTION2_SNAKE_CURVE_LOOKAHEAD_STEPS = '20'
    PORTION2_SNAKE_SHARP_PREVIEW_STEPS = '8'
    PORTION2_SNAKE_SHARP_LOOKAHEAD_STEPS = '14'
    PORTION2_SNAKE_STEER_RATE = '0.35f'
    PORTION2_SNAKE_SHARP_STEER_RATE = '0.60f'
    PORTION2_SNAKE_FINAL_ZONE_M = '3.0f'
}

foreach ($entry in $requiredDefinitions.GetEnumerator()) {
    $pattern = '#define\s+' + [regex]::Escape($entry.Key) + '\s+' + [regex]::Escape($entry.Value)
    if ($source -notmatch $pattern) {
        throw "Missing route 12 policy: $($entry.Key)=$($entry.Value)"
    }
}

foreach ($marker in @(
    'portion2_snake_turn_level',
    'PORTION2_SNAKE_TURN_CURVE',
    'PORTION2_SNAKE_TURN_SHARP',
    'portion2_snake_speed_limit'
)) {
    if ($source -notmatch [regex]::Escape($marker)) {
        throw "Missing route 12 control marker: $marker"
    }
}

Write-Output 'Route 12 policy checks passed.'
