$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$out = Join-Path $root 'output\unreal-balcony-railing-panel01-20261015'
if ($env:COMPUTERNAME -ne 'CRIBTEST11') { throw 'Wrong host' }
if ((git -C $root rev-parse HEAD) -ne '193a09fe7f99a80d34f099b727f0a43ad747e5d8') { throw 'Wrong base HEAD' }
$status = @(git -C $root status --porcelain)
if ($status.Count -ne 1 -or $status[0] -ne '?? output/unreal-balcony-railing-panel01-20261015/') { throw 'Unexpected worktree state' }
$processes = @(Get-Process blender,UnrealEditor -ErrorAction SilentlyContinue)
if ($processes.Count -ne 2 -or @($processes | Where-Object { $_.Id -eq 6116 -and $_.SessionId -eq 0 }).Count -ne 1 -or @($processes | Where-Object { $_.Id -eq 10404 -and $_.SessionId -eq 2 }).Count -ne 1) { throw 'Original process identity mismatch' }
if (Test-Path -LiteralPath (Join-Path $out 'initial-identity.json')) { throw 'Single-launch guard: already launched' }
if (-not (Test-Path -LiteralPath (Join-Path $out 'prelaunch-inspection.md'))) { throw 'Manual inspection receipt required' }
if ((Get-FileHash -LiteralPath (Join-Path $out 'source-excerpt.txt') -Algorithm SHA256).Hash.ToLowerInvariant() -ne '3ded6704d15074cb83f77afc824cf14604fae5ff1d1b4d029aa5d16bd986b9ab') { throw 'Source SHA mismatch' }
& (Join-Path $out 'preserve.ps1') before
$exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
$args = @('--background','--factory-startup','--threads','6','--python-exit-code','23','--python',(Join-Path $out 'author_panel.py'))
$p = Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $out 'production.log') -RedirectStandardError (Join-Path $out 'production-error.log')
[ordered]@{ host=$env:COMPUTERNAME; root=$root; base='193a09fe7f99a80d34f099b727f0a43ad747e5d8'; pid=$p.Id; originalBlender=6116; originalUnreal=10404; utc=[DateTime]::UtcNow.ToString('o'); executable=$exe; arguments=$args } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'launch-receipt.json') -Encoding UTF8
$p.WaitForExit()
$code = $p.ExitCode
[ordered]@{ pid=$p.Id; utc=[DateTime]::UtcNow.ToString('o'); numericOSExit=$code; status=$(if ($null -eq $code) {'UNKNOWN'} else {'OBSERVED'}); completeReceipt=(Test-Path -LiteralPath (Join-Path $out 'complete.json')) } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out 'process-exit.json') -Encoding UTF8
if (Test-Path -LiteralPath (Join-Path $out 'failure.json')) { throw 'Production frozen on failure; no retry' }
if (-not (Test-Path -LiteralPath (Join-Path $out 'complete.json'))) { throw 'No completion receipt; stop' }
