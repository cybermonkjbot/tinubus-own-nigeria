param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference = 'Stop'
$Root = 'C:\dev\cribtest-game'
$Out = Join-Path $Root 'output\unreal-waterfront-mooring-bollard01-20261015'
$OldRoot = 'output/unreal-courtyard-sconce01-mounting-finish01-20261015/'
$Pinned = Get-Content -LiteralPath (Join-Path $Root ($OldRoot + 'preservation-before.json')) -Raw | ConvertFrom-Json
$Expected = [ordered]@{}
foreach ($p in $Pinned.PSObject.Properties) { $Expected[$p.Name] = $p.Value }
if ($Expected.Count -ne 235) { throw 'Prior manifest does not contain235files' }
$OldFiles = @(git -C $Root ls-files -- $OldRoot)
if ($LASTEXITCODE -ne 0 -or $OldFiles.Count -ne 9) { throw 'Exact tracked mounting root is not9files' }
foreach ($p in $OldFiles) {
    if ($Expected.Contains($p)) { throw "Duplicate prior path: $p" }
    $Expected[$p] = (Get-FileHash -LiteralPath (Join-Path $Root $p) -Algorithm SHA256).Hash.ToLowerInvariant()
}
if ($Expected.Count -ne 244) { throw 'Historical scope is not244files' }
if ($Phase -eq 'after') {
    $Before = Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
    $Expected = [ordered]@{}
    foreach ($p in $Before.PSObject.Properties) { $Expected[$p.Name] = $p.Value }
}
$Actual = [ordered]@{}
$Mismatches = @()
foreach ($p in $Expected.Keys) {
    $Hash = (Get-FileHash -LiteralPath (Join-Path $Root $p) -Algorithm SHA256).Hash.ToLowerInvariant()
    $Actual[$p] = $Hash
    if ($Hash -cne $Expected[$p]) { $Mismatches += $p }
}
$Target = Join-Path $Out ("preservation-$Phase.json")
if (Test-Path -LiteralPath $Target) { throw "Exclusive receipt already exists: $Target" }
$Json = $Actual | ConvertTo-Json -Depth 5
$Stream = [System.IO.File]::Open($Target,[System.IO.FileMode]::CreateNew,[System.IO.FileAccess]::Write)
$Writer = New-Object System.IO.StreamWriter($Stream,(New-Object System.Text.UTF8Encoding($false)))
try { $Writer.Write($Json) } finally { $Writer.Dispose() }
[ordered]@{phase=$Phase; count=$Actual.Count; mismatches=$Mismatches.Count; utc=[DateTime]::UtcNow.ToString('o')} | ConvertTo-Json
if ($Mismatches.Count) { throw "Historical hash mismatches: $($Mismatches -join ', ')" }
