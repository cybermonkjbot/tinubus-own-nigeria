$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$relative = 'output/unreal-rooftop-condenser-enclosure01-20261015/'
$owned = Join-Path $root $relative
Set-Location -LiteralPath $root
function Receipt($name,$value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($value | ConvertTo-Json -Depth 15) + "`n")
    $stream = [IO.File]::Open((Join-Path $owned $name),[IO.FileMode]::CreateNew)
    try { $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
}
$head = git rev-parse HEAD
if ($head -ne '87b750f913f8c6211fa142091da25e0a973cecee') { throw 'BaseHEAD changed' }
$diff = @(git diff --name-only HEAD)
if ($diff.Count -ne 0) { throw 'Tracked worktree changed before owned staging' }
$untracked = @(git ls-files --others --exclude-standard)
foreach ($path in $untracked) { if (-not $path.StartsWith($relative,[StringComparison]::Ordinal)) { throw "Unrelated untracked $path" } }
$images = [ordered]@{}
foreach ($name in @('early-front','front','reverse','grazing','player165','support')) {
    $path = Join-Path $owned ($name + '.png')
    $bytes = [IO.File]::ReadAllBytes($path)
    if ([Convert]::ToBase64String($bytes[0..7]) -ne 'iVBORw0KGgo=') { throw "Invalid PNG signature $name" }
    $width = $bytes[16]*16777216 + $bytes[17]*65536 + $bytes[18]*256 + $bytes[19]
    $height = $bytes[20]*16777216 + $bytes[21]*65536 + $bytes[22]*256 + $bytes[23]
    if ($width -ne 800 -or $height -ne 800) { throw "PNG dimensions $name $width $height" }
    $images[$name] = [ordered]@{width=$width;height=$height;bytes=$bytes.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()}
}
$blend = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned 'RooftopCondenserEnclosure01.blend')).Hash.ToLowerInvariant()
if ($blend -ne '6ec8b9b4d7b0ecc1beec0829a9b4b045ae8482086be2ffafaa7974edd739d117') { throw 'Blend changed' }
$readbacks = [ordered]@{}
foreach ($name in @('before-save-readback.json','reopened-readback.json','postviews-readback.json')) {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned $name)).Hash.ToLowerInvariant()
    if ($hash -ne '795db38c9f40fa40c02cea2dffc335888dd1486644a70fe8ed4812b496c67367') { throw "Readback changed $name" }
    $readbacks[$name] = $hash
}
$after = Get-Content -Raw -LiteralPath (Join-Path $owned 'preservation-after-result.json') | ConvertFrom-Json
if ($after.count -ne 489 -or @($after.mismatches).Count -ne 0) { throw 'Preservation not489 zero mismatches' }
$original = @(Get-Process -Id 6116,10404 | Select-Object Id,ProcessName,SessionId)
Receipt 'artifact-receipt.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');base=$head;blend_sha256=$blend;source_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned 'source-excerpt.txt')).Hash.ToLowerInvariant();images=$images;readbacks=$readbacks;historical_preserved=489;outside_tracked_diff=$diff;outside_untracked=@();original_processes=$original;os_exit_code='unknown: wrapper null'})
$hashes = [ordered]@{}
foreach ($file in Get-ChildItem -LiteralPath $owned -File | Sort-Object Name) {
    if ($file.Name -eq 'owned-file-hashes.json') { throw 'Seal already exists' }
    $hashes[$file.Name] = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
}
Receipt 'owned-file-hashes.json' $hashes
[ordered]@{base=$head;owned_count=$hashes.Count+1;blend=$blend;historical=489;images=$images} | ConvertTo-Json -Depth 6
