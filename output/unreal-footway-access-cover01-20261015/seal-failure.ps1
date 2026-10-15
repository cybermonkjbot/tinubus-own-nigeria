$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$relative = 'output/unreal-footway-access-cover01-20261015/'
$owned = Join-Path $root $relative
Set-Location -LiteralPath $root
function Receipt($name,$value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($value | ConvertTo-Json -Depth 12) + "`n")
    $stream = [IO.File]::Open((Join-Path $owned $name),[IO.FileMode]::CreateNew)
    try { $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
}
$head = git rev-parse HEAD
if ($head -ne '8dd0825c5e8ec14ac8892143faa8f634a95b99fc') { throw 'HEAD changed' }
if (@(git diff --name-only HEAD).Count -ne 0) { throw 'Tracked worktree changed' }
foreach ($path in @(git ls-files --others --exclude-standard)) {
    if (-not $path.StartsWith($relative,[StringComparison]::Ordinal)) { throw "Outside untracked $path" }
}
$failure = Get-Content -Raw -LiteralPath (Join-Path $owned 'failure.json') | ConvertFrom-Json
$preservation = Get-Content -Raw -LiteralPath (Join-Path $owned 'preservation-after-result.json') | ConvertFrom-Json
if ($preservation.count -ne 581 -or @($preservation.mismatches).Count -ne 0) { throw 'Preservation mismatch' }
$unexpected = @('FootwayAccessCover01.blend','early-front.png','completion.json','continue.json','reopened-readback.json')
foreach ($path in $unexpected) { if (Test-Path -LiteralPath (Join-Path $owned $path)) { throw "Unexpected postfailure artifact $path" } }
$original = @(Get-Process -Id 6116,10404 | Select-Object Id,ProcessName,SessionId)
Receipt 'closure-receipt.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');base=$head;status='FAILED_FROZEN_NO_RETRY';failure=$failure;wip_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned 'work-in-progress.blend')).Hash.ToLowerInvariant();source_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned 'source-excerpt.txt')).Hash.ToLowerInvariant();historical_preserved=581;original_processes=$original;os_exit_code='UNKNOWN wrapper null';final_art_delivered=$false;images_delivered=0})
$hashes = [ordered]@{}
foreach ($file in Get-ChildItem -LiteralPath $owned -File | Sort-Object Name) {
    if ($file.Name -eq 'owned-file-hashes.json') { throw 'Already sealed' }
    $hashes[$file.Name] = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
}
Receipt 'owned-file-hashes.json' $hashes
[ordered]@{base=$head;count=$hashes.Count+1;historical=581;status='FAILED_FROZEN_NO_RETRY'} | ConvertTo-Json
