param(
    [Parameter(Mandatory = $true)][string]$EngineRoot,
    [string]$Project,
    [switch]$Headless
)

$ErrorActionPreference = 'Stop'
if (!$Project) { $Project = Join-Path $PSScriptRoot '..\BaddieCompanion.uproject' }
$Project = (Resolve-Path $Project).Path
$ProjectDir = Split-Path $Project
$Editor = Join-Path $EngineRoot 'Binaries\Win64\UnrealEditor.exe'
if (!(Test-Path $Editor)) { throw "Editor not found: $Editor" }
if (!(Test-Path (Join-Path $ProjectDir 'Binaries\Win64\UnrealEditor-BaddieCompanion.dll'))) {
    throw 'Compile BaddieCompanionEditor before launching. This script never starts a build.'
}
$Existing = @(Get-CimInstance Win32_Process -Filter "Name='UnrealEditor.exe'" |
    Where-Object { $_.CommandLine -and $_.CommandLine.Contains($Project) })
if ($Existing.Count -gt 0) { throw 'This project already has an editor process. Inspect it instead of starting another.' }

$BridgeDir = Join-Path $ProjectDir 'Saved\Bridge'
New-Item -ItemType Directory -Force $BridgeDir | Out-Null
$Stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
$Log = Join-Path $BridgeDir "editor-$Stamp.log"
$Arguments = @(
    ('"' + $Project + '"'),
    '-NoSplash', '-Unattended', '-EnablePython',
    '-ModelContextProtocolStartServer', '-ModelContextProtocolPort=8000',
    '-ini:Engine:[HTTPServer.Listeners]:DefaultBindAddress=127.0.0.1',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRemoteExecution=True',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:RemoteExecutionMulticastBindAddress=127.0.0.1',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:RemoteExecutionMulticastTtl=0',
    '-ini:Engine:[/Script/PythonScriptPlugin.PythonScriptPluginSettings]:bRunPipInstallOnStartup=False',
    '-ExecCmds="t.MaxFPS 15"',
    ('-AbsLog="' + $Log + '"')
)
if ($Headless) {
    $Arguments += '-nullrhi'
} else {
    $Arguments += @('-d3d11', '-AllowSoftwareRendering', '-windowed', '-ResX=1280', '-ResY=720')
}
$Process = Start-Process -FilePath $Editor -ArgumentList $Arguments -WorkingDirectory $ProjectDir -PassThru
$Receipt = [ordered]@{
    StartedAt = (Get-Date).ToUniversalTime().ToString('o')
    ProcessId = $Process.Id
    Project = $Project
    Headless = [bool]$Headless
    Log = $Log
    Arguments = $Arguments
}
$Receipt | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $BridgeDir "launch-$Stamp.json") -Encoding UTF8
$Receipt | ConvertTo-Json -Depth 4
Wait-Process -Id $Process.Id
