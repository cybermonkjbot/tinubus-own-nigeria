param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$owned = Join-Path $root 'output\unreal-road-kerb-segment01-20261015'
Set-Location -LiteralPath $root
function Receipt($name,$value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($value | ConvertTo-Json -Depth 12) + "`n")
    $stream = [IO.File]::Open((Join-Path $owned $name),[IO.FileMode]::CreateNew)
    try { $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
}
$expected = [ordered]@{}
if ($Phase -eq 'before') {
    $prior = Get-Content -Raw -LiteralPath 'output\unreal-rooftop-condenser-enclosure01-20261015\preservation-before.json' | ConvertFrom-Json
    if (@($prior.PSObject.Properties).Count -ne 489) { throw 'Prior not489' }
    foreach ($entry in $prior.PSObject.Properties) { $expected[$entry.Name.Replace('\','/')] = $entry.Value }
    $tracked = @(git ls-files -- output/unreal-rooftop-condenser-enclosure01-20261015/)
    if ($LASTEXITCODE -ne 0 -or $tracked.Count -ne 46) { throw 'Exact enclosure tracked count not46' }
    foreach ($path in $tracked) {
        if ($expected.Contains($path)) { throw "Duplicate $path" }
        $expected[$path] = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $path)).Hash.ToLowerInvariant()
    }
} else {
    $prior = Get-Content -Raw -LiteralPath (Join-Path $owned 'preservation-before.json') | ConvertFrom-Json
    foreach ($entry in $prior.PSObject.Properties) { $expected[$entry.Name] = $entry.Value }
}
if ($expected.Count -ne 535) { throw 'Historical count not535' }
$actual = [ordered]@{}
$mismatches = @()
foreach ($path in $expected.Keys) {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $root $path)).Hash.ToLowerInvariant()
    $actual[$path] = $hash
    if ($hash -ne $expected[$path]) { $mismatches += $path }
}
Receipt "preservation-$Phase.json" $actual
$result = [ordered]@{phase=$Phase;count=$actual.Count;mismatches=$mismatches;utc=[DateTime]::UtcNow.ToString('o');head=(git rev-parse HEAD)}
Receipt "preservation-$Phase-result.json" $result
if ($mismatches.Count -ne 0) { throw "Historical changed $($mismatches -join ',')" }
$result | ConvertTo-Json
