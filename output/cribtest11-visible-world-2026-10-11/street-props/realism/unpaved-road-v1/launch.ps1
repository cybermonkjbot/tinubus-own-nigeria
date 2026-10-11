$ErrorActionPreference = 'Stop'
$roadOutput = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\unpaved-road-v1'
if (@(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" | Where-Object { $_.ProcessId -ne 3224 }).Count -gt 0) { throw 'Another Blender job active' }
if (Test-Path -LiteralPath "$roadOutput\LagosUnpavedRoadKit.blend") { throw 'Fresh output required' }
$roadPriorJob=$env:CRIBTEST_BLENDER_OWNER_JOB
$roadPriorResources=$env:BLENDER_USER_RESOURCES
$roadPriorOMP=$env:OMP_NUM_THREADS
try {
    $env:CRIBTEST_BLENDER_OWNER_JOB='lagos-unpaved-road-v1'
    $env:BLENDER_USER_RESOURCES="$roadOutput\user-resources"
    $env:OMP_NUM_THREADS='2'
    $roadArgs=@('--background','--factory-startup','--offline-mode','--threads','2','--python-exit-code','1','--python',"$roadOutput\author_road_kit.py")
    $roadProcess=Start-Process -FilePath 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' -ArgumentList $roadArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput "$roadOutput\stdout.log" -RedirectStandardError "$roadOutput\stderr.log"
    $roadProcess.PriorityClass='BelowNormal';$roadProcess.ProcessorAffinity=[IntPtr]3
    [PSCustomObject]@{pid=$roadProcess.Id;output=$roadOutput;threads=2;samples=16;resolution='960x540';affinity=3} | ConvertTo-Json
} finally {
    $env:CRIBTEST_BLENDER_OWNER_JOB=$roadPriorJob
    $env:BLENDER_USER_RESOURCES=$roadPriorResources
    $env:OMP_NUM_THREADS=$roadPriorOMP
}
