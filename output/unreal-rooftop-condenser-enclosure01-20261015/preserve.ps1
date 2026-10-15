param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$owned = Join-Path $root 'output\unreal-rooftop-condenser-enclosure01-20261015'
Set-Location -LiteralPath $root
function Receipt($name, $value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($value | ConvertTo-Json -Depth 15) + "`n")
    $stream = [IO.File]::Open((Join-Path $owned $name), [IO.FileMode]::CreateNew)
    try { $stream.Write($bytes, 0, $bytes.Length) } finally { $stream.Dispose() }
}
if ($Phase -eq 'before') {
    $prior = Get-Content -Raw -LiteralPath 'output\unreal-rooftop-water-tank01-20261015\preservation-before.json' | ConvertFrom-Json
    $properties = @($prior.PSObject.Properties)
    if ($properties.Count -ne 447) { throw 'Prior preservation count not447' }
    $tracked = @(git ls-files -- output/unreal-rooftop-water-tank01-20261015/)
    if ($LASTEXITCODE -ne 0 -or $tracked.Count -ne 42) { throw 'Exact tracked Tank count not42' }
    $expected = [ordered]@{}
    foreach ($property in $properties) { $expected[$property.Name.Replace('\','/')] = $property.Value }
    foreach ($path in $tracked) {
        if ($expected.Contains($path)) { throw "Duplicate historical path $path" }
        $expected[$path] = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $path)).Hash.ToLowerInvariant()
    }
} else {
    $prior = Get-Content -Raw -LiteralPath (Join-Path $owned 'preservation-before.json') | ConvertFrom-Json
    $expected = [ordered]@{}
    foreach ($property in $prior.PSObject.Properties) { $expected[$property.Name] = $property.Value }
}
if ($expected.Count -ne 489) { throw 'Historical count not489' }
$actual = [ordered]@{}
$mismatches = @()
foreach ($path in $expected.Keys) {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $path)).Hash.ToLowerInvariant()
    $actual[$path] = $hash
    if ($hash -ne $expected[$path]) { $mismatches += $path }
}
Receipt "preservation-$Phase.json" $actual
$result = [ordered]@{ phase=$Phase; count=$actual.Count; mismatches=$mismatches; utc=[DateTime]::UtcNow.ToString('o'); head=(git rev-parse HEAD) }
Receipt "preservation-$Phase-result.json" $result
if ($mismatches.Count -ne 0) { throw "Historical mismatch: $($mismatches -join ',')" }
$result | ConvertTo-Json -Depth 5
