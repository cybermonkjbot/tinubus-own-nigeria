param([string]$ToolsRoot = 'C:\dev\tools\baddie-blender')

$ErrorActionPreference = 'Stop'
$env:BADDIE_BLENDER_TOOLS = $ToolsRoot
$env:BLENDER_MCP_HOST = '127.0.0.1'
$env:BLENDER_MCP_PORT = '9876'
$env:BLENDER_PATH = Join-Path $ToolsRoot 'blender-5.2.2-windows-x64\blender.exe'
& (Join-Path $ToolsRoot 'venv\Scripts\python.exe') (Join-Path $PSScriptRoot 'blender_mcp_entry.py')
exit $LASTEXITCODE
