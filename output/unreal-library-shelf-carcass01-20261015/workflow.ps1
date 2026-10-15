param([ValidateSet('launch','seal','commit','closure')][string]$Action)
$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game'
$relative = 'output/unreal-library-shelf-carcass01-20261015'
$out = Join-Path $root $relative
$base = '43a1fd809394fd93a30779b315283c985c40d68d'
function Originals {
    $ps = @(Get-Process blender,UnrealEditor -ErrorAction SilentlyContinue)
    if ($ps.Count -ne 2 -or @($ps | Where-Object { $_.Id -eq 6116 -and $_.SessionId -eq 0 }).Count -ne 1 -or @($ps | Where-Object { $_.Id -eq 10404 -and $_.SessionId -eq 2 }).Count -ne 1) { throw 'Original processes mismatch' }
}
function Hash([string]$path) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing: $path" }
    return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()
}
function Receipt([string]$name,$value) {
    $value | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $out $name) -Encoding UTF8
}
function Preserve([string]$phase) {
    $parent = Join-Path $root 'output/unreal-balcony-railing-panel01-20261015/preservation-before.json'
    if ((Hash $parent) -ne '92df60a1849fb1e94f68c6504b2c4e64df808c76e6bf6ef9cb0548e7e56842d5') { throw '695 manifest hash mismatch' }
    $old = Get-Content -LiteralPath $parent -Raw | ConvertFrom-Json
    if (@($old.PSObject.Properties).Count -ne 695) { throw 'Parent count must be 695' }
    $tracked = @(git -C $root ls-files -- output/unreal-balcony-railing-panel01-20261015/)
    if ($LASTEXITCODE -ne 0 -or $tracked.Count -ne 53) { throw 'Railing tracked count must be 53' }
    $expected = [ordered]@{}
    foreach ($p in $old.PSObject.Properties) { $expected.Add($p.Name,$p.Value) }
    foreach ($path in $tracked) {
        if ($expected.Contains($path)) { throw "Duplicate immutable path: $path" }
        $expected.Add($path,(Hash (Join-Path $root $path)))
    }
    if ($expected.Count -ne 748) { throw 'Immutable count must be 748' }
    if ($phase -ne 'before') {
        $before = Get-Content -LiteralPath (Join-Path $out 'preservation-before.json') -Raw | ConvertFrom-Json
        if (@($before.PSObject.Properties).Count -ne 748) { throw 'Before count mismatch' }
        foreach ($p in $before.PSObject.Properties) {
            if (-not $expected.Contains($p.Name)) { throw "Changed path set: $($p.Name)" }
            $expected[$p.Name] = $p.Value
        }
    }
    $actual = [ordered]@{}
    foreach ($path in $expected.Keys) {
        $sha = Hash (Join-Path $root $path)
        if ($sha -ne $expected[$path]) { throw "Immutable mismatch: $path expected=$($expected[$path]) actual=$sha" }
        $actual.Add($path,$sha)
    }
    if ($phase -ne 'closure') {
        Receipt "preservation-$phase.json" $actual
        Receipt "preservation-$phase-receipt.json" ([ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); count=748; parent=695; railing=53; differences=0; manifestSHA='92df60a1849fb1e94f68c6504b2c4e64df808c76e6bf6ef9cb0548e7e56842d5' })
    }
    Write-Output "Preservation ${phase}: 748 immutable hashes, zero differences"
}
if ($env:COMPUTERNAME -ne 'CRIBTEST11' -or (Get-Location).Path -ne $root) { throw 'Wrong host/root' }
if ($Action -eq 'launch') {
    Originals
    if ((git rev-parse HEAD) -ne $base) { throw 'Wrong HEAD' }
    $status = @(git status --porcelain)
    if ($status.Count -ne 1 -or $status[0] -ne "?? $relative/") { throw 'Unexpected prelaunch tree state' }
    if (Test-Path -LiteralPath (Join-Path $out 'launch-receipt.json')) { throw 'Single launch only; stop' }
    if (-not (Test-Path -LiteralPath (Join-Path $out 'prelaunch-inspection.md'))) { throw 'Manual complete inspection required' }
    if ((Hash (Join-Path $out 'source-excerpt.txt')) -ne '69be9974fd8a32690f8076f7e2e229bd62a7830f7348d22a15c782e2583b20c6') { throw 'Source excerpt mismatch' }
    Preserve before
    $exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
    $arguments = @('--background','--factory-startup','--threads','6','--python-exit-code','23','--python',(Join-Path $out 'author_shelf.py'))
    $p = Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $out 'production.log') -RedirectStandardError (Join-Path $out 'production-error.log')
    Receipt 'launch-receipt.json' ([ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); host=$env:COMPUTERNAME; root=$root; base=$base; pid=$p.Id; executable=$exe; arguments=$arguments; originalBlender=6116; originalUnreal=10404 })
    $p.WaitForExit()
    $numeric = $p.ExitCode
    Receipt 'process-exit.json' ([ordered]@{ utc=[DateTime]::UtcNow.ToString('o'); pid=$p.Id; numericOSExit=$numeric; status=$(if ($null -eq $numeric) {'UNKNOWN'} else {'OBSERVED'}); complete=(Test-Path -LiteralPath (Join-Path $out 'complete.json')) })
    if (Test-Path -LiteralPath (Join-Path $out 'failure.json')) { throw 'Production failed and frozen; no retry' }
    if (-not (Test-Path -LiteralPath (Join-Path $out 'complete.json'))) { throw 'No completion evidence; stop' }
} elseif ($Action -eq 'seal') {
    Originals
    Preserve after
    $hashes = [ordered]@{}
    foreach ($file in Get-ChildItem -LiteralPath $out -File | Sort-Object Name) {
        if ($file.Name -ne 'owned-sha256.json') { $hashes.Add($file.Name,(Hash $file.FullName)) }
    }
    Receipt 'owned-sha256.json' $hashes
    Write-Output "Sealed $($hashes.Count) owned files excluding self-referential manifest"
} elseif ($Action -eq 'commit') {
    Originals
    if ((git rev-parse HEAD) -ne $base) { throw 'Base changed before commit' }
    git -c core.hooksPath=NUL -c maintenance.auto=false -c gc.auto=0 -c core.autocrlf=false add -- "$relative/"
    if ($LASTEXITCODE -ne 0) { throw 'Staging failed' }
    $paths = @(git diff --cached --name-only)
    $files = @(Get-ChildItem -LiteralPath $out -File)
    if ($paths.Count -ne $files.Count) { throw 'Staged file count mismatch' }
    foreach ($path in $paths) {
        if (-not $path.StartsWith("$relative/")) { throw "Outside scope: $path" }
        $raw = git hash-object --no-filters -- $path
        $index = git rev-parse ":$path"
        if ($raw -ne $index) { throw "Raw/index mismatch: $path" }
    }
    git diff --cached --stat
    git -c core.hooksPath=NUL -c maintenance.auto=false -c gc.auto=0 -c core.autocrlf=false commit -m 'Add independent LibraryShelfCarcass01 art and evidence'
    if ($LASTEXITCODE -ne 0) { throw 'Local commit failed' }
} else {
    Originals
    Preserve closure
    $owned = Get-Content -LiteralPath (Join-Path $out 'owned-sha256.json') -Raw | ConvertFrom-Json
    foreach ($p in $owned.PSObject.Properties) {
        if ((Hash (Join-Path $out $p.Name)) -ne $p.Value) { throw "Owned hash mismatch: $($p.Name)" }
    }
    if (@(git status --porcelain).Count -ne 0 -or (git rev-parse HEAD^) -ne $base) { throw 'Postcommit tree/parent mismatch' }
    $paths = @(git diff-tree --no-commit-id --name-only -r HEAD)
    if ($paths.Count -ne @(Get-ChildItem -LiteralPath $out -File).Count -or @($paths | Where-Object { -not $_.StartsWith("$relative/") }).Count -ne 0) { throw 'Commit scope mismatch' }
    Write-Output "Closure UTC $([DateTime]::UtcNow.ToString('o')): clean tree, $($paths.Count) new scoped paths, sealed owned hashes match"
    git rev-parse HEAD
}
