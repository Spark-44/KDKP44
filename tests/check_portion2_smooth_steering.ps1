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

Assert-Match $guandao '#define\s+PORTION2_SMOOTH_STEER_RATE_LIMIT\s+\(0\.28f\)' `
    'portion2 smooth steering must use a gentler steering rate limit.'

Assert-Match $guandao 'static\s+void\s+portion2_apply_smooth_steering_params\s*\([^)]*float\s*\*\s*steering_gain[^)]*float\s*\*\s*steering_limit[^)]*float\s*\*\s*steering_rate_limit[^)]*\)' `
    'portion2 steering params must be centralized in a helper.'

Assert-Match $guandao 'if\(state\s*==\s*&portion_2\)[\s\S]*?portion2_apply_smooth_steering_params\(&steering_gain,\s*&steering_limit,\s*&steering_rate_limit,\s*route11_reverse\);[\s\S]*?if\(base_speed\s*>\s*GUANDAO_HIGH_SPEED_THRESHOLD\)' `
    'portion2 params must be applied before general speed handling.'

Assert-Match $guandao 'if\(state\s*==\s*&portion_2\)[\s\S]*?portion2_apply_smooth_steering_params\(&steering_gain,\s*&steering_limit,\s*&steering_rate_limit,\s*route11_reverse\);[\s\S]*?pursuit_midhandle' `
    'portion2 params must be restored after general speed handling so high-speed logic cannot make route2 aggressive.'

Assert-Match $guandao 'steering_rate_limit\s*=\s*route11_reverse\s*\?\s*PORTION2_STEER_RATE_LIMIT\s*:\s*PORTION2_SMOOTH_STEER_RATE_LIMIT' `
    'normal portion2 routes must use the smoother steering rate while route11 reverse keeps its existing rate.'

Write-Host 'portion2 smooth steering checks passed.'
