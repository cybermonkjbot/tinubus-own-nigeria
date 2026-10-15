$ErrorActionPreference = 'Stop'
$frontageControl = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\frontage-v2-control'
$frontageOutput = 'C:\dev\baddie-assets\YabaRealismKit\frontage-v2'
$otherBlender = @(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" | Where-Object { $_.ProcessId -ne 3224 })
if ($otherBlender.Count -gt 0) { throw 'Another Blender process is active; serial authoring required' }
if (Test-Path -LiteralPath $frontageOutput) { throw 'Fresh frontage output must not already exist' }
if (-not (Test-Path -LiteralPath "$frontageControl\media-receipt-verified.json")) { throw 'Complete coordinator media receipt not confirmed' }
$priorJob = $env:CRIBTEST_BLENDER_OWNER_JOB
$priorResources = $env:BLENDER_USER_RESOURCES
$priorOMP = $env:OMP_NUM_THREADS
try {
    $env:CRIBTEST_BLENDER_OWNER_JOB = 'yaba-frontage-replacement-v1'
    $env:BLENDER_USER_RESOURCES = "$frontageControl\user-resources"
    $env:OMP_NUM_THREADS = '2'
    $frontageArguments = @('--background','--factory-startup','--offline-mode','--threads','2','--python-exit-code','1','--python',"$frontageControl\run_frontage_owned.py")
    $frontageProcess = Start-Process -FilePath 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' -ArgumentList $frontageArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput "$frontageControl\stdout.log" -RedirectStandardError "$frontageControl\stderr.log"
    $frontageProcess.PriorityClass = 'BelowNormal'
    $frontageProcess.ProcessorAffinity = [IntPtr]3
    [PSCustomObject]@{pid=$frontageProcess.Id;job='yaba-frontage-replacement-v1';output=$frontageOutput;threads=2;samples=24;affinity=3;priority='BelowNormal'} | ConvertTo-Json
} finally {
    $env:CRIBTEST_BLENDER_OWNER_JOB = $priorJob
    $env:BLENDER_USER_RESOURCES = $priorResources
    $env:OMP_NUM_THREADS = $priorOMP
}
