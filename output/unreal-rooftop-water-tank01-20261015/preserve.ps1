param([ValidateSet('before','after','owned')][string]$Phase)
$ErrorActionPreference='Stop'
$root='C:\dev\cribtest-game'
$owned='output/unreal-rooftop-water-tank01-20261015'
Set-Location -LiteralPath $root
function Receipt($Name,$Data) {
    $stream=[System.IO.File]::Open("$root/$owned/$Name",[System.IO.FileMode]::CreateNew)
    $writer=New-Object System.IO.StreamWriter($stream,(New-Object System.Text.UTF8Encoding($false)))
    try {$writer.Write(($Data|ConvertTo-Json -Depth 30))} finally {$writer.Dispose()}
}
if($Phase -eq 'owned') {
    $hashes=[ordered]@{}
    Get-ChildItem -LiteralPath "$root/$owned" -File -Recurse | Sort-Object FullName | ForEach-Object {
        $path=$_.FullName.Substring($root.Length+1)
        $hashes[$path]=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
    Receipt 'owned-file-hashes.json' $hashes
    Write-Output "Owned hashes=$($hashes.Count)"
    exit
}
$expected=[ordered]@{}
if($Phase -eq 'before') {
    $prior=Get-Content -LiteralPath 'output/unreal-kiosk-display-table01-provenance-addendum-20261015/preservation-before.json' -Raw|ConvertFrom-Json
    if(@($prior.PSObject.Properties).Count -ne 439){throw 'Prior manifest count not439'}
    foreach($e in $prior.PSObject.Properties){$expected[$e.Name]=$e.Value}
    $paths=@(git ls-files -- output/unreal-kiosk-display-table01-provenance-addendum-20261015/)
    if($LASTEXITCODE -ne 0 -or $paths.Count -ne 8){throw 'Exact addendum count not8'}
    foreach($p in $paths){$expected[$p]=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
} else {
    $prior=Get-Content -LiteralPath "$owned/preservation-before.json" -Raw|ConvertFrom-Json
    foreach($e in $prior.PSObject.Properties){$expected[$e.Name]=$e.Value}
}
if($expected.Count -ne 447){throw 'Manifest count not447'}
$actual=[ordered]@{}
$mismatches=@()
foreach($p in $expected.Keys){
    $hash=(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()
    $actual[$p]=$hash
    if($hash -ne $expected[$p]){$mismatches+=$p}
}
Receipt "preservation-$Phase.json" $actual
Receipt "preservation-$Phase-result.json" ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');count=$actual.Count;mismatches=$mismatches})
if($mismatches.Count -ne 0){throw 'Historical mismatch'}
Write-Output "Preserved447 files, zero mismatches ($Phase)"
