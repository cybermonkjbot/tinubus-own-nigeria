param(
    [string]$ToolsRoot = 'C:\dev\tools\baddie-blender',
    [string]$Workbench = 'C:\dev\baddie-assets\Blender\LagosWorkbench.blend'
)

$ErrorActionPreference = 'Stop'
$Blender = Join-Path $ToolsRoot 'blender-5.2.2-windows-x64\blender.exe'
if (!(Test-Path $Blender)) { throw "Blender not installed: $Blender" }
$Listener = Get-NetTCPConnection -State Listen -LocalPort 9876 -ErrorAction SilentlyContinue
if ($Listener) { throw 'Port 9876 already has an owner; inspect it rather than start another scene.' }
$Existing = @(Get-CimInstance Win32_Process -Filter "Name='blender.exe'" |
    Where-Object { $_.CommandLine -and $_.CommandLine.Contains('blender_bootstrap.py') })
if ($Existing.Count) { throw 'The asset workbench is already running.' }

$env:BADDIE_BLENDER_TOOLS = $ToolsRoot
$env:BADDIE_BLENDER_WORKBENCH = $Workbench
$env:BLENDER_USER_RESOURCES = Join-Path $ToolsRoot 'profile'
& $Blender --background --factory-startup --online-mode --python-exit-code 1 `
    --python (Join-Path $PSScriptRoot 'blender_bootstrap.py') `
    --command blender_mcp --host 127.0.0.1 --port 9876
if ($LASTEXITCODE -ne 0) { throw "Blender exited with code $LASTEXITCODE" }
