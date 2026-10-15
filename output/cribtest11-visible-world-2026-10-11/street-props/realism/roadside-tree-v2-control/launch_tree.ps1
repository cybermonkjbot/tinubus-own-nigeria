$ErrorActionPreference = 'Stop'
$treeControl = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\roadside-tree-v2-control'
$treeOutput = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\roadside-tree-v2'
$treeSource = 'C:\dev\cribtest-game\output\cribtest11-visible-world-2026-10-11\street-props\realism\roadside-tree-source-r2\roadside_tree.py'
$otherBlender = @(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" | Where-Object { $_.ProcessId -ne 3224 })
if ($otherBlender.Count -gt 0) { throw 'Another Blender process is active; serial authoring required' }
if (Test-Path -LiteralPath $treeOutput) { throw 'Fresh tree output must not already exist' }
if ((Get-FileHash -Algorithm SHA256 -LiteralPath $treeSource).Hash.ToLower() -ne '24ed6c208b2c4743ad94e6f26442d5174a3037ff751745270abf3d71c189c312') { throw 'Tree source hash mismatch' }
$priorJob = $env:CRIBTEST_BLENDER_OWNER_JOB
$priorResources = $env:BLENDER_USER_RESOURCES
$priorOMP = $env:OMP_NUM_THREADS
try {
    $env:CRIBTEST_BLENDER_OWNER_JOB = 'roadside-tree-v1'
    $env:BLENDER_USER_RESOURCES = "$treeControl\user-resources"
    $env:OMP_NUM_THREADS = '2'
    $treeArguments = @('--background','--factory-startup','--offline-mode','--threads','2','--python-exit-code','1','--python',"$treeControl\run_tree_owned.py")
    $treeProcess = Start-Process -FilePath 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe' -ArgumentList $treeArguments -WindowStyle Hidden -PassThru -RedirectStandardOutput "$treeControl\stdout.log" -RedirectStandardError "$treeControl\stderr.log"
    $treeProcess.PriorityClass = 'BelowNormal'
    $treeProcess.ProcessorAffinity = [IntPtr]3
    [PSCustomObject]@{pid=$treeProcess.Id;job='roadside-tree-v1';output=$treeOutput;threads=2;samples=24;affinity=3;priority='BelowNormal'} | ConvertTo-Json
} finally {
    $env:CRIBTEST_BLENDER_OWNER_JOB = $priorJob
    $env:BLENDER_USER_RESOURCES = $priorResources
    $env:OMP_NUM_THREADS = $priorOMP
}
