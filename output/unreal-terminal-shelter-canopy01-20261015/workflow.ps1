param([ValidateSet('launch','seal','commit','closure')][string]$Action)
$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$rel = 'output/unreal-terminal-shelter-canopy01-20261015'
$out = Join-Path $root $rel
$base = '82945ff8a86ba54b9e8d2982bb3c5dd208525423'
function SHA([string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing $path" }
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function WriteReceipt([string]$name,$data) {
    $data | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $out $name) -Encoding UTF8
}
function Originals {
    $p = @(Get-Process blender,UnrealEditor -ErrorAction SilentlyContinue)
    if ($p.Count -ne 2 -or @($p | Where-Object { $_.Id -eq 6116 -and $_.SessionId -eq 0 }).Count -ne 1 -or @($p | Where-Object { $_.Id -eq 10404 -and $_.SessionId -eq 2 }).Count -ne 1) { throw 'Original process prestate changed' }
}
function Preserve([string]$phase) {
    $parent = Join-Path $root 'output/unreal-library-shelf-carcass01-20261015/preservation-before.json'
    if ((SHA $parent) -ne '162914480727d6f09091ff270e3f25784e49221a9c9c536e19cd3b317d593961') { throw '748 parent manifest hash mismatch' }
    $old = Get-Content -LiteralPath $parent -Raw | ConvertFrom-Json
    if (@($old.PSObject.Properties).Count -ne 748) { throw 'Expected 748 parent files' }
    $paths = @(git -C $root ls-files -- output/unreal-library-shelf-carcass01-20261015/)
    if ($LASTEXITCODE -ne 0 -or $paths.Count -ne 51) { throw 'Expected 51 tracked shelf files' }
    $expected = [ordered]@{}
    foreach ($p in $old.PSObject.Properties) { $expected.Add($p.Name,$p.Value) }
    foreach ($path in $paths) {
        if ($expected.Contains($path)) { throw "Duplicate $path" }
        $raw = git -C $root hash-object --no-filters -- $path
        $pinned = git -C $root rev-parse "${base}:$path"
        if ($LASTEXITCODE -ne 0 -or $raw -ne $pinned) { throw "Shelf bytes not pinned: $path" }
        $expected.Add($path,(SHA (Join-Path $root $path)))
    }
    if ($expected.Count -ne 799) { throw 'Expected 799 unique immutable files' }
    if ($phase -ne 'before') {
        $before = Get-Content -LiteralPath (Join-Path $out 'preservation-before.json') -Raw | ConvertFrom-Json
        if (@($before.PSObject.Properties).Count -ne 799) { throw 'Before count mismatch' }
        foreach ($p in $before.PSObject.Properties) {
            if (-not $expected.Contains($p.Name)) { throw "Path set changed: $($p.Name)" }
            $expected[$p.Name] = $p.Value
        }
    }
    $actual = [ordered]@{}
    foreach ($path in $expected.Keys) {
        $hash = SHA (Join-Path $root $path)
        if ($hash -ne $expected[$path]) { throw "Immutable mismatch: $path expected=$($expected[$path]) actual=$hash" }
        $actual.Add($path,$hash)
    }
    if ($phase -ne 'closure') {
        WriteReceipt "preservation-$phase.json" $actual
        WriteReceipt "preservation-$phase-receipt.json" ([ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); count=799; parentCount=748; shelfCount=51; pinnedBase=$base; differences=0; parentSHA='162914480727d6f09091ff270e3f25784e49221a9c9c536e19cd3b317d593961' })
    }
    Write-Output "Preservation ${phase}: 799 hashes unchanged, shelf raw bytes pinned"
}
if ($env:COMPUTERNAME -ne 'CRIBTEST11' -or (Get-Location).Path -ne $root) { throw 'Wrong host/root' }
if ($Action -eq 'launch') {
    Originals
    if ((git rev-parse HEAD) -ne $base) { throw 'HEAD mismatch' }
    $state = @(git status --porcelain)
    if ($state.Count -ne 1 -or $state[0] -ne "?? $rel/") { throw 'Unexpected worktree changes' }
    if ((Test-Path -LiteralPath (Join-Path $out 'launch-receipt.json')) -or (Test-Path -LiteralPath (Join-Path $out 'initial-identity.json'))) { throw 'Already launched: no retry' }
    if (-not (Test-Path -LiteralPath (Join-Path $out 'prelaunch-inspection.md'))) { throw 'Manual full inspection required' }
    if ((SHA (Join-Path $out 'source-excerpt.txt')) -ne '1c34c0a02f7eb54fd241beb8e334d14ccabeff6a2faa0d33f6146efbbf6a7309') { throw 'Source mismatch' }
    Preserve before
    $exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
    $arguments = @('--background','--factory-startup','--threads','6','--python-exit-code','23','--python',(Join-Path $out 'author_canopy.py'))
    $p = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $out 'production.log') -RedirectStandardError (Join-Path $out 'production-error.log')
    WriteReceipt 'launch-receipt.json' ([ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); host=$env:COMPUTERNAME; root=$root; base=$base; pid=$p.Id; executable=$exe; arguments=$arguments; originals=@(6116,10404) })
    $p.WaitForExit()
    $code = $p.ExitCode
    WriteReceipt 'process-exit.json' ([ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); pid=$p.Id; numericOSExit=$code; status=$(if ($null -eq $code) {'UNKNOWN'} else {'OBSERVED'}); complete=(Test-Path -LiteralPath (Join-Path $out 'complete.json')) })
    if (Test-Path -LiteralPath (Join-Path $out 'failure.json')) { throw 'Production frozen after failure; no retry' }
    if (-not (Test-Path -LiteralPath (Join-Path $out 'complete.json'))) { throw 'No completion receipt; stop' }
} elseif ($Action -eq 'seal') {
    Originals
    Preserve after
    $hashes = [ordered]@{}
    foreach ($f in Get-ChildItem -LiteralPath $out -File | Sort-Object Name) {
        if ($f.Name -ne 'owned-sha256.json') { $hashes.Add($f.Name,(SHA $f.FullName)) }
    }
    WriteReceipt 'owned-sha256.json' $hashes
    Write-Output "Sealed $($hashes.Count) files excluding self-referential manifest"
} elseif ($Action -eq 'commit') {
    Originals
    if ((git rev-parse HEAD) -ne $base) { throw 'Base changed' }
    if (@(git diff --cached --name-only).Count -ne 0) { throw 'Unexpected preexisting staging' }
    git -c core.hooksPath=NUL -c maintenance.auto=false -c gc.auto=0 -c core.autocrlf=false add -- "$rel/"
    if ($LASTEXITCODE -ne 0) { throw 'Stage failure' }
    $paths = @(git diff --cached --name-only)
    if ($paths.Count -ne @(Get-ChildItem -LiteralPath $out -File).Count) { throw 'Staged count mismatch' }
    foreach ($path in $paths) {
        if (-not $path.StartsWith("$rel/")) { throw "Outside scope $path" }
        if ((git hash-object --no-filters -- $path) -ne (git rev-parse ":$path")) { throw "Raw/index mismatch $path" }
    }
    git diff --cached --stat
    git -c core.hooksPath=NUL -c maintenance.auto=false -c gc.auto=0 -c core.autocrlf=false commit -m 'Add independent TerminalShelterCanopy01 art and evidence'
    if ($LASTEXITCODE -ne 0) { throw 'Commit failure' }
} else {
    Originals
    Preserve closure
    $owned = Get-Content -LiteralPath (Join-Path $out 'owned-sha256.json') -Raw | ConvertFrom-Json
    foreach ($p in $owned.PSObject.Properties) {
        if ((SHA (Join-Path $out $p.Name)) -ne $p.Value) { throw "Owned mismatch $($p.Name)" }
    }
    if (@(git status --porcelain).Count -ne 0 -or (git rev-parse HEAD^) -ne $base) { throw 'Closure tree/parent mismatch' }
    $paths = @(git diff-tree --no-commit-id --name-only -r HEAD)
    if ($paths.Count -ne @(Get-ChildItem -LiteralPath $out -File).Count -or @($paths | Where-Object { -not $_.StartsWith("$rel/") }).Count -ne 0) { throw 'Closure scope mismatch' }
    Write-Output "Closure $([DateTime]::UtcNow.ToString('o')): clean tree; $($paths.Count) new scoped paths; owned hashes match"
    git rev-parse HEAD
}
