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

Assert-Match $guandao 'static\s+void\s+portion2_run_reset_start_basis\s*\(\s*void\s*\)' `
    'portion2 run mode must use a dedicated start-basis reset helper.'
Assert-Match $guandao 'portion2_run_reset_start_basis[\s\S]*?Yaw_1\s*=\s*0\.0f' `
    'run start reset must zero Yaw_1 so recording-time yaw drift is not carried into run alignment.'
Assert-Match $guandao 'portion2_run_reset_start_basis[\s\S]*?portion_2\.current_point_index\s*=\s*0' `
    'run start reset must restart route tracking from point 0.'
Assert-Match $guandao 'portion2_run_reset_start_basis[\s\S]*?portion_2\.current_state\.x\s*=\s*0\.0f[\s\S]*?portion_2\.current_state\.y\s*=\s*0\.0f[\s\S]*?portion_2\.current_state\.theta\s*=\s*0\.0f' `
    'run start reset must clear the inertial route pose before alignment.'
Assert-Match $guandao 'portion2_run_reset_start_basis[\s\S]*?Encoder_count_init\s*\(\s*&guandao_ecd\s*\)[\s\S]*?encoder_clear_count\s*\(\s*ENCODER_QUADDEC\s*\)' `
    'run start reset must clear encoder deltas so old movement cannot leak into the first update.'
Assert-Match $guandao 'portion2_run_reset_start_basis[\s\S]*?portion2_gps_fusion_reset\s*\(\s*\)' `
    'run start reset must clear GPS fusion state before preparing the route.'
Assert-Match $guandao 'case\s+1:[\s\S]*?portion2_run_reset_start_basis\s*\(\s*\)[\s\S]*?run_start_theta\s*=' `
    'portion2 run state must reset the start basis before computing run_start_theta.'

Write-Host 'portion2 run start basis reset checks passed.'
