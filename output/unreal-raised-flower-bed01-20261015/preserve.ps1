param([ValidateSet('before','after')][string]$Phase)
$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output/unreal-raised-flower-bed01-20261015'
function NewJson($Name,$Value){
    $Bytes=[Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 20)+"`n")
    $Stream=[IO.File]::Open((Join-Path $Out $Name),[IO.FileMode]::CreateNew)
    try{$Stream.Write($Bytes,0,$Bytes.Length)}finally{$Stream.Dispose()}
}
$Manifest=[ordered]@{}
if($Phase -eq 'before'){
    $Old=Get-Content -LiteralPath (Join-Path $Root 'output/unreal-terrace-pergola-module01-20261015/preservation-before.json') -Raw | ConvertFrom-Json
    $Entries=@($Old.PSObject.Properties)
    if($Entries.Count -ne 605){throw 'Expected605prior'}
    foreach($Entry in $Entries){$Manifest[$Entry.Name]=$Entry.Value}
    $Paths=@(git ls-files -- output/unreal-terrace-pergola-module01-20261015/)
    if($LASTEXITCODE -ne 0 -or $Paths.Count -ne 45){throw 'Expected45trackedPergola'}
    foreach($Path in $Paths){
        if(-not $Path.StartsWith('output/unreal-terrace-pergola-module01-20261015/')){throw "Wrongroot $Path"}
        if($Manifest.Contains($Path)){throw "Duplicate $Path"}
        if(-not(Test-Path -LiteralPath (Join-Path $Root $Path) -PathType Leaf)){throw "Missing $Path"}
        $Manifest[$Path]=(Get-FileHash -LiteralPath (Join-Path $Root $Path) -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}else{
    $Old=Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
    foreach($Entry in $Old.PSObject.Properties){$Manifest[$Entry.Name]=$Entry.Value}
}
if($Manifest.Count -ne 650){throw 'Expected650historical'}
$Actual=[ordered]@{}
foreach($Path in $Manifest.Keys){
    $Full=Join-Path $Root $Path
    if(-not(Test-Path -LiteralPath $Full -PathType Leaf)){throw "Missing $Path"}
    $Hash=(Get-FileHash -LiteralPath $Full -Algorithm SHA256).Hash.ToLowerInvariant()
    if($Hash -ne $Manifest[$Path]){throw "Historicalmismatch $Path"}
    $Actual[$Path]=$Hash
}
NewJson "preservation-$Phase.json" $Actual
$Result=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');phase=$Phase;count=650;mismatches=0;base='42b35e250a561e76b3f70e8824ac74754ade4161'}
NewJson "preservation-$Phase-result.json" $Result
$Result | ConvertTo-Json
