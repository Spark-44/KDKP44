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

$repo = Split-Path -Parent $PSScriptRoot
$guandao = Get-Content -Raw -Path (Join-Path $repo 'code/guandao.c')

Assert-Match $guandao '#define\s+PORTION2_GUIDED_FINAL_STOP_DIST\s+0\.35f' `
    'guided portion2 routes must stop within a tighter 0.35m final radius.'
Assert-Match $guandao '#define\s+PORTION2_FINAL_CLOSEST_ARM_DIST\s+0\.80f' `
    'portion2 final stop must arm closest-approach tracking inside 0.80m.'
Assert-Match $guandao '#define\s+PORTION2_FINAL_CLOSEST_RISE_DIST\s+0\.05f' `
    'portion2 final stop must react when final distance rises by 0.05m after closest approach.'
Assert-Match $guandao '#define\s+PORTION2_FINAL_CLOSEST_CONFIRM_CYCLES\s+1U' `
    'portion2 final stop must not wait three cycles after closest approach is missed.'
Assert-Match $guandao '#define\s+PORTION2_FINAL_CRAWL_DIST\s+1\.00f' `
    'portion2 final speed limiting must be restricted to the final 1.00m.'
Assert-Match $guandao '#define\s+PORTION2_FINAL_CRAWL_SPEED\s+5\.0f' `
    'portion2 final speed limiting must use a low terminal crawl speed without changing normal speed.'

Assert-Match $guandao 'static\s+uint8\s+portion2_final_zone_should_stop\s*\(' `
    'portion2 final stopping must use a dedicated closest-approach helper.'
Assert-Match $guandao 'portion2_final_zone_should_stop\s*\(\s*raw_point,\s*raw_length,\s*dist_to_final,\s*final_stop_distance\s*\)' `
    'pursuit control must stop through the final-zone closest-approach helper.'
Assert-Match $guandao 'dist_to_final\s*<=\s*PORTION2_FINAL_CRAWL_DIST[\s\S]*?v_center\s*>\s*PORTION2_FINAL_CRAWL_SPEED[\s\S]*?v_center\s*=\s*PORTION2_FINAL_CRAWL_SPEED' `
    'portion2 terminal crawl speed must only apply inside the final distance window.'
Assert-Match $guandao 'portion2_final_zone_min_dist\s*=\s*0\.0f' `
    'portion2 final closest-distance state must be reset between runs.'

Write-Host 'portion2 final stop checks passed.'
