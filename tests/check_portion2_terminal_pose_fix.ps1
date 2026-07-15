$ErrorActionPreference = 'Stop'

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

function Assert-NotMatch {
    param(
        [string]$Text,
        [string]$Pattern,
        [string]$Message
    )
    if($Text -match $Pattern) {
        throw $Message
    }
}

$repo = Split-Path -Parent $PSScriptRoot
$guandao = Get-Content -Raw -Path (Join-Path $repo 'code\guandao.c')

Assert-Match $guandao 'static\s+void\s+portion2_record_update_final_yaw\s*\(\s*void\s*\)' `
    'recording must keep route final yaw synchronized with the latest recorded pose.'

Assert-Match $guandao 'portion2_route_final_yaw\[portion2_record_route\]\s*=\s*portion2_route_storage_get\(\s*offset\s*\+\s*len\s*-\s*1\s*\)\.theta' `
    'final yaw must come from the last recorded route point, not the first point.'

Assert-Match $guandao 'if\(k2_long\)[\s\S]*?portion2_record_update_final_yaw\s*\(\s*\)[\s\S]*?Flash_Write_passage_points\s*\(\s*\)' `
    'saving a route while still recording must capture the latest endpoint yaw before writing flash.'

Assert-Match $guandao 'yaw_steering\s*=\s*yaw_error\s*\*\s*PORTION2_GUIDED_FINAL_YAW_GAIN' `
    'guided terminal yaw steering must use the same sign as yaw error so positive yaw error commands a left correction and negative commands right correction according to the existing steering convention.'

Assert-NotMatch $guandao 'yaw_steering\s*=\s*-yaw_error\s*\*\s*PORTION2_GUIDED_FINAL_YAW_GAIN' `
    'guided terminal yaw steering must not invert yaw error.'

Assert-Match $guandao 'uint8\s+align_result\s*=\s*portion2_final_yaw_align\s*\(\s*final_point,\s*dist_to_final,\s*out_v_l,\s*out_v_r,\s*out_servo\s*\)' `
    'the final stop zone must run heading alignment before stopping when yaw is outside tolerance.'

Assert-Match $guandao 'if\(align_result\s*==\s*PORTION2_FINAL_YAW_ALIGN_RUNNING\)[\s\S]*?return;' `
    'while terminal heading alignment is running the controller must not fall through to the normal stop branch.'

Assert-Match $guandao 'final_dist="\);\s*portion2_serial_append_fixed100[\s\S]*?final_yaw="\);\s*portion2_serial_append_fixed100[\s\S]*?yaw_err="\);\s*portion2_serial_append_fixed100' `
    'run telemetry must include final_yaw and yaw_err so terminal heading corrections can be diagnosed before stop.'

Write-Host 'portion2 terminal pose checks passed.'
