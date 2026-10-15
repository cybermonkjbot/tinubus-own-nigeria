param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output\unreal-minipark-waste-bin01-20261015'
$OldRoot='output/unreal-waterfront-mooring-bollard01-20261015/'
$Expected=[ordered]@{}
if ($Phase -eq 'before') {
    $Pinned=Get-Content -LiteralPath (Join-Path $Root ($OldRoot+'preservation-before.json')) -Raw | ConvertFrom-Json
    foreach ($p in $Pinned.PSObject.Properties) {$Expected[$p.Name]=$p.Value}
    if ($Expected.Count -ne 244) {throw 'Prior scope not244files'}
    $Files=@(git -C $Root ls-files -- $OldRoot)
    if ($LASTEXITCODE -ne 0 -or $Files.Count -ne 14) {throw 'Exact Bollard root not14files'}
    foreach ($p in $Files) {
        if ($Expected.Contains($p)) {throw "Duplicate path: $p"}
        $Expected[$p]=(Get-FileHash -LiteralPath (Join-Path $Root $p) -Algorithm SHA256).Hash.ToLowerInvariant()
    }
} else {
    $Pinned=Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
    foreach ($p in $Pinned.PSObject.Properties) {$Expected[$p.Name]=$p.Value}
}
if ($Expected.Count -ne 258) {throw 'Historical scope not258files'}
$Actual=[ordered]@{}
$Mismatches=@()
foreach ($p in $Expected.Keys) {
    $Hash=(Get-FileHash -LiteralPath (Join-Path $Root $p) -Algorithm SHA256).Hash.ToLowerInvariant()
    $Actual[$p]=$Hash
    if ($Hash -cne $Expected[$p]) {$Mismatches+=$p}
}
$Path=Join-Path $Out "preservation-$Phase.json"
$Stream=[System.IO.File]::Open($Path,[System.IO.FileMode]::CreateNew,[System.IO.FileAccess]::Write)
$Writer=New-Object System.IO.StreamWriter($Stream,(New-Object System.Text.UTF8Encoding($false)))
try {$Writer.Write(($Actual | ConvertTo-Json -Depth 5))} finally {$Writer.Dispose()}
[ordered]@{phase=$Phase;count=$Actual.Count;mismatches=$Mismatches.Count;utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json
if ($Mismatches.Count) {throw "Historical bytes changed: $($Mismatches -join ', ')"}
