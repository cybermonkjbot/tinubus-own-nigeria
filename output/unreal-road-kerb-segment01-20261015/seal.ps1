$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$relative = 'output/unreal-road-kerb-segment01-20261015/'
$owned = Join-Path $root $relative
Set-Location -LiteralPath $root
function Receipt($name,$value) {
    $bytes = [Text.UTF8Encoding]::new($false).GetBytes(($value | ConvertTo-Json -Depth 12) + "`n")
    $stream = [IO.File]::Open((Join-Path $owned $name),[IO.FileMode]::CreateNew)
    try { $stream.Write($bytes,0,$bytes.Length) } finally { $stream.Dispose() }
}
$head = git rev-parse HEAD
if ($head -ne '2fd4b8bb677fc3a68f644219bd5cef8f21869f3d') { throw 'BaseHEAD changed' }
$diff = @(git diff --name-only HEAD)
if ($diff.Count -ne 0) { throw 'Tracked worktree changed' }
foreach ($path in @(git ls-files --others --exclude-standard)) {
    if (-not $path.StartsWith($relative,[StringComparison]::Ordinal)) { throw "Outside untracked $path" }
}
$images = [ordered]@{}
foreach ($name in @('early-front','front','reverse','grazing','player165','end-contact')) {
    $path = Join-Path $owned ($name+'.png')
    $bytes = [IO.File]::ReadAllBytes($path)
    if ([Convert]::ToBase64String($bytes[0..7]) -ne 'iVBORw0KGgo=') { throw "PNG signature $name" }
    $width = $bytes[16]*16777216+$bytes[17]*65536+$bytes[18]*256+$bytes[19]
    $height = $bytes[20]*16777216+$bytes[21]*65536+$bytes[22]*256+$bytes[23]
    if ($width -ne 800 -or $height -ne 800) { throw "PNG dimensions $name" }
    $images[$name] = [ordered]@{width=$width;height=$height;bytes=$bytes.Length;sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath $path).Hash.ToLowerInvariant()}
}
$blend = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned 'RoadKerbSegment01.blend')).Hash.ToLowerInvariant()
if ($blend -ne '29bcf2b8d5cc35b244e45f3b86c434e14c7d8ff98bccf131be9d2305bee813c3') { throw 'Blend changed' }
$readbacks = [ordered]@{}
foreach ($name in @('before-save-readback.json','reopened-readback.json','postviews-readback.json')) {
    $hash = (Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned $name)).Hash.ToLowerInvariant()
    if ($hash -ne '91b128900e1abe10c3b98df0451611ad5f3e2fe5b1944b7b7875a3abeefab8c5') { throw "Readback changed $name" }
    $readbacks[$name] = $hash
}
$after = Get-Content -Raw -LiteralPath (Join-Path $owned 'preservation-after-result.json') | ConvertFrom-Json
if ($after.count -ne 535 -or @($after.mismatches).Count -ne 0) { throw 'Historical preservation not535 zero' }
$original = @(Get-Process -Id 6116,10404 | Select-Object Id,ProcessName,SessionId)
Receipt 'artifact-receipt.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');base=$head;blend_sha256=$blend;source_sha256=(Get-FileHash -Algorithm SHA256 -LiteralPath (Join-Path $owned 'source-excerpt.txt')).Hash.ToLowerInvariant();images=$images;readbacks=$readbacks;historical_preserved=535;outside_tracked_diff=$diff;outside_untracked=@();original_processes=$original;os_exit_code='UNKNOWN wrapper null'})
$hashes = [ordered]@{}
foreach ($file in Get-ChildItem -LiteralPath $owned -File | Sort-Object Name) {
    if ($file.Name -eq 'owned-file-hashes.json') { throw 'Already sealed' }
    $hashes[$file.Name] = (Get-FileHash -Algorithm SHA256 -LiteralPath $file.FullName).Hash.ToLowerInvariant()
}
Receipt 'owned-file-hashes.json' $hashes
[ordered]@{base=$head;owned_count=$hashes.Count+1;historical=535;blend_sha256=$blend;images=$images} | ConvertTo-Json -Depth 6
