$ErrorActionPreference = 'Stop'
$vehicleOutput = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\vehicle-wear-v1'
$otherBlender = @(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" | Where-Object { $_.ProcessId -ne 3224 })
if ($otherBlender.Count -gt 0) { throw 'Another Blender process is active; one authoring/render batch only' }
$previousJob = $env:CRIBTEST_BLENDER_OWNER_JOB
$previousResources = $env:BLENDER_USER_RESOURCES
$previousOMP = $env:OMP_NUM_THREADS
try {
    $env:CRIBTEST_BLENDER_OWNER_JOB = 'lagos-vehicle-wear-v1'
    $env:BLENDER_USER_RESOURCES = "$vehicleOutput\user-resources"
    $env:OMP_NUM_THREADS = '2'
    $vehicleArgs = @('--background','--factory-startup','--offline-mode','--threads','2','--python-exit-code','1','--python',"$vehicleOutput\author_vehicles.py",'--','--owned-background-job','lagos-vehicle-wear-v1')
    $vehicleProcess = Start-Process -FilePath 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' -ArgumentList $vehicleArgs -WindowStyle Hidden -PassThru -RedirectStandardOutput "$vehicleOutput\stdout.log" -RedirectStandardError "$vehicleOutput\stderr.log"
    $vehicleProcess.PriorityClass = 'BelowNormal'
    $vehicleProcess.ProcessorAffinity = [IntPtr]3
    [PSCustomObject]@{pid=$vehicleProcess.Id;output=$vehicleOutput;threads=2;samples=16;resolution='960x640';affinity=3;priority='BelowNormal'} | ConvertTo-Json
} finally {
    $env:CRIBTEST_BLENDER_OWNER_JOB = $previousJob
    $env:BLENDER_USER_RESOURCES = $previousResources
    $env:OMP_NUM_THREADS = $previousOMP
}
