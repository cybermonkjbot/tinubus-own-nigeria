param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'; $Out=Join-Path $Root 'output\unreal-frontage-low-wall-bay01-20261015'
$OldRoot='output/unreal-street-drain-channel01-20261015/'; $Expected=[ordered]@{}
if($Phase -eq 'before'){
    $Pinned=Get-Content -LiteralPath (Join-Path $Root ($OldRoot+'preservation-before.json')) -Raw | ConvertFrom-Json
    foreach($p in $Pinned.PSObject.Properties){$Expected[$p.Name]=$p.Value}
    if($Expected.Count -ne 300){throw 'Prior scope not300files'}
    $Files=@(git -C $Root ls-files -- $OldRoot)
    if($LASTEXITCODE -ne 0 -or $Files.Count -ne 42){throw 'Exact Channel root not42files'}
    foreach($p in $Files){if($Expected.Contains($p)){throw "Duplicate path: $p"}; $Expected[$p]=(Get-FileHash -LiteralPath (Join-Path $Root $p) -Algorithm SHA256).Hash.ToLowerInvariant()}
}else{
    $Pinned=Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
    foreach($p in $Pinned.PSObject.Properties){$Expected[$p.Name]=$p.Value}
}
if($Expected.Count -ne 342){throw 'Historical scope not342files'}
$Actual=[ordered]@{}; $Bad=@()
foreach($p in $Expected.Keys){$Hash=(Get-FileHash -LiteralPath (Join-Path $Root $p) -Algorithm SHA256).Hash.ToLowerInvariant(); $Actual[$p]=$Hash; if($Hash -cne $Expected[$p]){$Bad+=$p}}
$Stream=[System.IO.File]::Open((Join-Path $Out "preservation-$Phase.json"),[System.IO.FileMode]::CreateNew,[System.IO.FileAccess]::Write)
$Writer=New-Object System.IO.StreamWriter($Stream,(New-Object System.Text.UTF8Encoding($false)))
try{$Writer.Write(($Actual | ConvertTo-Json -Depth 5))}finally{$Writer.Dispose()}
[ordered]@{phase=$Phase;count=$Actual.Count;mismatches=$Bad.Count;utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json
if($Bad.Count){throw "Historical bytes changed: $($Bad -join ', ')"}
