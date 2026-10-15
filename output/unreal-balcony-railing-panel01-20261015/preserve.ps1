param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$out = Join-Path $root 'output\unreal-balcony-railing-panel01-20261015'
$parent = Join-Path $root 'output\unreal-raised-flower-bed01-20261015\preservation-before.json'
if ((Get-FileHash -LiteralPath $parent -Algorithm SHA256).Hash.ToLowerInvariant() -ne '83dc76f921011a8f82fd1fe1087b986c8efda6fb2955eecd3914e84162303b8a') { throw '650 manifest SHA mismatch' }
$old = Get-Content -LiteralPath $parent -Raw | ConvertFrom-Json
if (@($old.PSObject.Properties).Count -ne 650) { throw 'Expected exactly 650 parent entries' }
$tracked = @(git -C $root ls-files -- output/unreal-raised-flower-bed01-20261015/)
if ($LASTEXITCODE -ne 0 -or $tracked.Count -ne 45) { throw 'Expected exactly 45 tracked Flower files' }
$expected = [ordered]@{}
foreach ($p in $old.PSObject.Properties) { $expected.Add($p.Name, $p.Value) }
foreach ($p in $tracked) {
    if ($expected.Contains($p)) { throw "Duplicate historical path: $p" }
    if (-not (Test-Path -LiteralPath (Join-Path $root $p) -PathType Leaf)) { throw "Missing: $p" }
    $expected.Add($p, (Get-FileHash -LiteralPath (Join-Path $root $p) -Algorithm SHA256).Hash.ToLowerInvariant())
}
if ($expected.Count -ne 695) { throw 'Expected exactly 695 distinct historical files' }
if ($Phase -eq 'after') {
    $baseline = Get-Content -LiteralPath (Join-Path $out 'preservation-before.json') -Raw | ConvertFrom-Json
    if (@($baseline.PSObject.Properties).Count -ne 695) { throw 'Invalid before manifest count' }
    foreach ($p in $baseline.PSObject.Properties) {
        if (-not $expected.Contains($p.Name)) { throw "Historical path set changed: $($p.Name)" }
        $expected[$p.Name] = $p.Value
    }
}
$actual = [ordered]@{}
foreach ($p in $expected.Keys) {
    $path = Join-Path $root $p
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing: $p" }
    $sha = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($sha -ne $expected[$p]) { throw "Historical SHA changed: $p expected=$($expected[$p]) actual=$sha" }
    $actual.Add($p, $sha)
}
$actual | ConvertTo-Json -Depth 3 | Set-Content -LiteralPath (Join-Path $out "preservation-$Phase.json") -Encoding UTF8
[ordered]@{ phase=$Phase; utc=[DateTime]::UtcNow.ToString('o'); count=695; mismatches=0; parentCount=650; flowerTrackedCount=45; parentSHA='83dc76f921011a8f82fd1fe1087b986c8efda6fb2955eecd3914e84162303b8a' } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $out "preservation-$Phase-receipt.json") -Encoding UTF8
Write-Output "Historical preservation ${Phase}: 695 files, zero differences"
