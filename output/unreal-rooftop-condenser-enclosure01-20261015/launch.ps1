$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game\output\unreal-rooftop-condenser-enclosure01-20261015'
$exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
if (Test-Path -LiteralPath (Join-Path $root 'launch-identity.json')) { throw 'Single launch only' }
if ((git -C C:\dev\cribtest-game rev-parse HEAD) -ne '87b750f913f8c6211fa142091da25e0a973cecee') { throw 'HEAD changed' }
$source = Join-Path $root 'source-excerpt.txt'
if ((Get-Item -LiteralPath $source).Length -ne 6646 -or (Get-FileHash -Algorithm SHA256 -LiteralPath $source).Hash.ToLowerInvariant() -ne '55b74f16594f233ad3185a71c305be862c3d00a6888dbf3f65e9ee030bd27726') { throw 'Source bytes mismatch' }
$proc = Start-Process -FilePath $exe -ArgumentList @('--background','--factory-startup','--threads','6','--python-exit-code','1','--python',(Join-Path $root 'author_enclosure.py')) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $root 'production.log') -RedirectStandardError (Join-Path $root 'production-error.log')
function Receipt($name,$data) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($data | ConvertTo-Json -Depth 10) + "`n")
    $stream = [IO.File]::Open((Join-Path $root $name),[IO.FileMode]::CreateNew)
    try { $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
}
$launch = [ordered]@{ host=$env:COMPUTERNAME; root=$root; head=(git -C C:\dev\cribtest-game rev-parse HEAD); pid=$proc.Id; exe=$exe; utc=[DateTime]::UtcNow.ToString('o'); originalBlender=6116; originalUnreal=10404 }
Receipt 'launch-identity.json' $launch
$launch | ConvertTo-Json
$proc.WaitForExit()
$proc.Refresh()
$exit = [ordered]@{ pid=$proc.Id; os_exit_code=$proc.ExitCode; utc=[DateTime]::UtcNow.ToString('o'); retry_forbidden=$true }
Receipt 'process-exit.json' $exit
$exit | ConvertTo-Json
