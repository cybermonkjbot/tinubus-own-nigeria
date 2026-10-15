$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game\output\unreal-road-kerb-segment01-20261015'
$exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
if (Test-Path -LiteralPath (Join-Path $root 'launch-identity.json')) { throw 'Single launch only' }
if ((git -C C:\dev\cribtest-game rev-parse HEAD) -ne '2fd4b8bb677fc3a68f644219bd5cef8f21869f3d') { throw 'HEAD changed' }
$source = Join-Path $root 'source-excerpt.txt'
if ((Get-Item -LiteralPath $source).Length -ne 5275 -or (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash.ToLowerInvariant() -ne '4d02b7064f5d9e150831c64da9712035ce4e37f5f0277444a9e9e3eaab0a14cd') { throw 'Source mismatch' }
function Receipt($name,$value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($value | ConvertTo-Json -Depth 10) + "`n")
    $stream = [IO.File]::Open((Join-Path $root $name),[IO.FileMode]::CreateNew)
    try { $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
}
$proc = Start-Process -FilePath $exe -ArgumentList @('--background','--factory-startup','--threads','6','--python-exit-code','1','--python',(Join-Path $root 'author_kerb.py')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'production.log') -RedirectStandardError (Join-Path $root 'production-error.log')
$launch = [ordered]@{host=$env:COMPUTERNAME;workspace='C:\dev\cribtest-game';head=(git -C C:\dev\cribtest-game rev-parse HEAD);pid=$proc.Id;exe=$exe;utc=[DateTime]::UtcNow.ToString('o');originalBlender=6116;originalUnreal=10404}
Receipt 'launch-identity.json' $launch
$launch | ConvertTo-Json
$proc.WaitForExit()
$proc.Refresh()
$code = $proc.ExitCode
$exit = [ordered]@{pid=$proc.Id;os_exit_code=$code;numeric_available=($null -ne $code);utc=[DateTime]::UtcNow.ToString('o');retry_forbidden=$true}
Receipt 'process-exit.json' $exit
$exit | ConvertTo-Json
