$ErrorActionPreference='Stop'
$roadPreview='C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\unpaved-road-v1\preview-r2'
if (@(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" | Where-Object {$_.ProcessId -ne 3224}).Count -gt 0) {throw 'Another Blender active'}
if (Test-Path -LiteralPath "$roadPreview\LagosUnpavedRoadKit-PreviewR2.blend") {throw 'Fresh preview required'}
$roadPreviewPriorJob=$env:CRIBTEST_BLENDER_OWNER_JOB
$roadPreviewPriorResources=$env:BLENDER_USER_RESOURCES
$roadPreviewPriorOMP=$env:OMP_NUM_THREADS
try {
  $env:CRIBTEST_BLENDER_OWNER_JOB='lagos-unpaved-preview-r2';$env:BLENDER_USER_RESOURCES="$roadPreview\user-resources";$env:OMP_NUM_THREADS='2'
  $roadPreviewProcess=Start-Process -FilePath 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' -ArgumentList @('--background','--factory-startup','--offline-mode','--threads','2','--python-exit-code','1','--python',"$roadPreview\repair_preview.py") -WindowStyle Hidden -PassThru -RedirectStandardOutput "$roadPreview\stdout.log" -RedirectStandardError "$roadPreview\stderr.log"
  $roadPreviewProcess.PriorityClass='BelowNormal';$roadPreviewProcess.ProcessorAffinity=[IntPtr]3
  [PSCustomObject]@{pid=$roadPreviewProcess.Id;output=$roadPreview;threads=2;samples=16;resolution='960x540'} | ConvertTo-Json
} finally {
  $env:CRIBTEST_BLENDER_OWNER_JOB=$roadPreviewPriorJob;$env:BLENDER_USER_RESOURCES=$roadPreviewPriorResources;$env:OMP_NUM_THREADS=$roadPreviewPriorOMP
}
