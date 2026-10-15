$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$out = Join-Path $root 'output\unreal-balcony-railing-panel01-20261015'
& (Join-Path $out 'preserve.ps1') after
$hashes = [ordered]@{}
foreach ($file in Get-ChildItem -LiteralPath $out -File | Sort-Object Name) {
    if ($file.Name -ne 'owned-sha256.json') { $hashes.Add($file.Name, (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()) }
}
$hashes | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $out 'owned-sha256.json') -Encoding UTF8
Write-Output "Sealed $($hashes.Count) owned files, excluding self-referential hash manifest"
