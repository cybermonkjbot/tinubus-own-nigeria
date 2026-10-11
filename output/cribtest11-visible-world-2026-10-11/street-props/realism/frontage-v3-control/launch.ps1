$ErrorActionPreference = 'Stop'
$frontageControl = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\frontage-v3-control'
$frontageBase = 'C:\dev\baddie-assets\YabaRealismKit\frontage-v3-owned'
if (@(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" | Where-Object { $_.ProcessId -ne 3224 }).Count -gt 0) { throw 'Another Blender process active; no concurrent launch' }
if (Test-Path -LiteralPath "$frontageBase\frontage-v3") { throw 'Fresh frontage-v3 output required' }
if ((Get-Content -LiteralPath "$frontageBase\.owner-yaba-kit" -Raw).Trim() -ne 'yaba-frontage-v3') { throw 'Owner marker mismatch' }
$frontagePriorJob = $env:CRIBTEST_BLENDER_OWNER_JOB
$frontagePriorResources = $env:BLENDER_USER_RESOURCES
$frontagePriorOMP = $env:OMP_NUM_THREADS
try {
    $env:CRIBTEST_BLENDER_OWNER_JOB = 'yaba-frontage-v3'
    $env:BLENDER_USER_RESOURCES = "$frontageControl\user-resources"
    $env:OMP_NUM_THREADS = '2'
    $frontageArgs = @('--background','--factory-startup','--offline-mode','--threads','2','--python-exit-code','1','--python',"$frontageControl\execute_frontage_v3.py",'--','--repo-root','C:\dev\cribtest-game','--output-base',$frontageBase,'--run-name','frontage-v3','--owned-job','yaba-frontage-v3','--material-inputs',"$frontageControl\yaba-material-inputs-v3.json",'--render','--samples','24')
    $frontageProcess = Start-Process -FilePath 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' -ArgumentList $frontageArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput "$frontageControl\stdout.log" -RedirectStandardError "$frontageControl\stderr.log"
    $frontageProcess.PriorityClass = 'BelowNormal'
    $frontageProcess.ProcessorAffinity = [IntPtr]3
    [PSCustomObject]@{pid=$frontageProcess.Id;output="$frontageBase\frontage-v3";threads=2;samples=24;resolution='960x640';affinity=3} | ConvertTo-Json
} finally {
    $env:CRIBTEST_BLENDER_OWNER_JOB = $frontagePriorJob
    $env:BLENDER_USER_RESOURCES = $frontagePriorResources
    $env:OMP_NUM_THREADS = $frontagePriorOMP
}
