param([string]$ToolsRoot = 'C:\dev\tools\baddie-blender')

$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
$env:PIP_DISABLE_PIP_VERSION_CHECK = '1'
$ArchiveName = 'blender-5.2.2-windows-x64.zip'
$ExpectedHash = '3849d17a682cba006075aaa3f3597ecb5c9c30ec31035b2e092c53e40679b535'
$Archive = Join-Path $ToolsRoot $ArchiveName
$Blender = Join-Path $ToolsRoot 'blender-5.2.2-windows-x64\blender.exe'
$Python = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\ThirdParty\Python3\Win64\python.exe'
$Venv = Join-Path $ToolsRoot 'venv'
$VenvPython = Join-Path $Venv 'Scripts\python.exe'
$StateFile = Join-Path $ToolsRoot 'install-state.json'
New-Item -ItemType Directory -Force $ToolsRoot | Out-Null

function Write-State([string]$Phase, [string]$Detail = '') {
    @{phase=$Phase; detail=$Detail; updatedAt=(Get-Date).ToUniversalTime().ToString('o')} |
        ConvertTo-Json | Set-Content -Encoding UTF8 $StateFile
}

try {
    if (!(Test-Path $Blender)) {
        Write-State 'download'
        if (!(Test-Path $Archive)) {
            & curl.exe --fail --location --silent --show-error --output ($Archive + '.partial') `
                ('https://download.blender.org/release/Blender5.2/' + $ArchiveName)
            if ($LASTEXITCODE -ne 0) { throw 'Blender download failed' }
            if ((Get-FileHash ($Archive + '.partial') -Algorithm SHA256).Hash.ToLower() -ne $ExpectedHash) {
                throw 'Blender archive checksum mismatch'
            }
            Move-Item ($Archive + '.partial') $Archive
        }
        if ((Get-FileHash $Archive -Algorithm SHA256).Hash.ToLower() -ne $ExpectedHash) {
            throw 'Blender archive checksum mismatch'
        }
        Write-State 'extract'
        & tar.exe -xf $Archive -C $ToolsRoot
        if ($LASTEXITCODE -ne 0) { throw 'Blender extraction failed' }
    }
    Write-State 'python-environment'
    if (!(Test-Path $VenvPython)) {
        & $Python -m venv $Venv
        if ($LASTEXITCODE -ne 0) { throw 'MCP virtual environment creation failed' }
    }
    & $VenvPython -m pip install --only-binary=:all: 'mcp[cli]==1.30.0' 'docutils==0.23' 'pyyaml==6.0.3'
    if ($LASTEXITCODE -ne 0) { throw 'MCP dependency installation failed' }
    & $Blender --version
    if ($LASTEXITCODE -ne 0) { throw 'Blender executable could not start' }
    Write-State 'installed'
} catch {
    Write-State 'failed' ($_.Exception.Message)
    throw
}
