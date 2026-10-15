param([ValidateSet('before','after','owned')][string]$Phase)
$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$owned = 'output/unreal-entrance-stone-portal01-20261015'
Set-Location -LiteralPath $root
function Receipt($Name, $Data) {
    $path = Join-Path $root "$owned/$Name"
    $stream = [System.IO.File]::Open($path, [System.IO.FileMode]::CreateNew)
    $writer = New-Object System.IO.StreamWriter($stream, (New-Object System.Text.UTF8Encoding($false)))
    try { $writer.Write(($Data | ConvertTo-Json -Depth 30)) } finally { $writer.Dispose() }
}
if ($Phase -eq 'owned') {
    $hashes = [ordered]@{}
    Get-ChildItem -LiteralPath "$root/$owned" -File -Recurse | Sort-Object FullName | ForEach-Object {
        $rel = $_.FullName.Substring($root.Length + 1)
        $hashes[$rel] = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    Receipt 'owned-file-hashes.json' $hashes
    Write-Output "Owned hashes: $($hashes.Count)"
    exit
}
if ($Phase -eq 'before') {
    $prior = Get-Content -LiteralPath 'output/unreal-frontage-low-wall-bay01-20261015/preservation-before.json' -Raw | ConvertFrom-Json
    if (@($prior.PSObject.Properties).Count -ne 342) { throw 'Prior manifest count is not 342' }
    $expected = [ordered]@{}
    foreach ($entry in $prior.PSObject.Properties) { $expected[$entry.Name] = $entry.Value }
    $paths = @(git ls-files -- output/unreal-frontage-low-wall-bay01-20261015/)
    if ($LASTEXITCODE -ne 0 -or $paths.Count -ne 42) { throw 'Exact LowWall tracked count is not 42' }
    foreach ($path in $paths) {
        $expected[$path] = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    }
} else {
    $before = Get-Content -LiteralPath "$owned/preservation-before.json" -Raw | ConvertFrom-Json
    $expected = [ordered]@{}
    foreach ($entry in $before.PSObject.Properties) { $expected[$entry.Name] = $entry.Value }
}
if ($expected.Count -ne 384) { throw 'Preservation manifest count is not 384' }
$actual = [ordered]@{}
$mismatches = @()
foreach ($path in $expected.Keys) {
    $hash = (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
    $actual[$path] = $hash
    if ($hash -ne $expected[$path]) { $mismatches += $path }
}
Receipt "preservation-$Phase.json" $actual
Receipt "preservation-$Phase-result.json" ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');count=$actual.Count;mismatches=$mismatches})
if ($mismatches.Count -ne 0) { throw "Historical mismatches: $($mismatches.Count)" }
Write-Output "Preserved $($actual.Count) files, zero mismatches ($Phase)"
