param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output/unreal-terrace-pergola-module01-20261015'
function NewJson($Name,$Value) {
    $Bytes=[Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 20)+"`n")
    $Stream=[IO.File]::Open((Join-Path $Out $Name),[IO.FileMode]::CreateNew)
    try {$Stream.Write($Bytes,0,$Bytes.Length)} finally {$Stream.Dispose()}
}
if($Phase -eq 'before') {
    $Old=Get-Content -LiteralPath (Join-Path $Root 'output/unreal-footway-access-cover01-20261015/preservation-before.json') -Raw | ConvertFrom-Json
    $Entries=@($Old.PSObject.Properties)
    if($Entries.Count -ne 581){throw 'Expected581historical'}
    $Manifest=[ordered]@{}
    foreach($Entry in $Entries){$Manifest[$Entry.Name]=$Entry.Value}
    $Paths=@(git ls-files -- output/unreal-footway-access-cover01-20261015/)
    if($LASTEXITCODE -ne 0 -or $Paths.Count -ne 24){throw 'Expected24trackedAccessCoverfiles'}
    foreach($Path in $Paths){
        if(-not $Path.StartsWith('output/unreal-footway-access-cover01-20261015/')){throw "Outsideoldroot $Path"}
        if($Manifest.Contains($Path)){throw "Duplicatehistorical $Path"}
        if(-not(Test-Path -LiteralPath (Join-Path $Root $Path) -PathType Leaf)){throw "Missing $Path"}
        $Manifest[$Path]=(Get-FileHash -LiteralPath (Join-Path $Root $Path) -Algorithm SHA256).Hash.ToLowerInvariant()
    }
} else {
    $Old=Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
    $Manifest=[ordered]@{}
    foreach($Entry in $Old.PSObject.Properties){$Manifest[$Entry.Name]=$Entry.Value}
}
if($Manifest.Count -ne 605){throw 'Expected605historical'}
$Actual=[ordered]@{}
foreach($Path in $Manifest.Keys){
    $Full=Join-Path $Root $Path
    if(-not(Test-Path -LiteralPath $Full -PathType Leaf)){throw "Missing $Path"}
    $Hash=(Get-FileHash -LiteralPath $Full -Algorithm SHA256).Hash.ToLowerInvariant()
    if($Hash -ne $Manifest[$Path]){throw "Historicalmismatch $Path"}
    $Actual[$Path]=$Hash
}
NewJson "preservation-$Phase.json" $Actual
$Result=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');phase=$Phase;count=605;mismatches=0;base='19d6ea4d28dfa9435d3af8b0fa0be89f05166225'}
NewJson "preservation-$Phase-result.json" $Result
$Result | ConvertTo-Json
