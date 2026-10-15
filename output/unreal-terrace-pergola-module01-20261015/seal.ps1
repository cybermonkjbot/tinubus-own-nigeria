$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output/unreal-terrace-pergola-module01-20261015'
function NewJson($Name,$Value){
    $Bytes=[Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 20)+"`n")
    $Stream=[IO.File]::Open((Join-Path $Out $Name),[IO.FileMode]::CreateNew)
    try{$Stream.Write($Bytes,0,$Bytes.Length)}finally{$Stream.Dispose()}
}
if(Test-Path -LiteralPath (Join-Path $Out 'failure.json')){throw 'Failure; do not seal as success'}
$Completion=Get-Content -LiteralPath (Join-Path $Out 'completion.json') -Raw | ConvertFrom-Json
if($Completion.five_images -ne 5 -or -not $Completion.restored_exact_match){throw 'Incomplete'}
$Readbacks=@('before-save-readback.json','reopened-readback.json','restored-used-readback.json')
$Hashes=@($Readbacks | ForEach-Object {(Get-FileHash -LiteralPath (Join-Path $Out $_) -Algorithm SHA256).Hash.ToLowerInvariant()})
if(@($Hashes | Select-Object -Unique).Count -ne 1){throw 'Readbackbytesmismatch'}
$Images=@('whole-front.png','reverse-low.png','grazing-junction.png','player165.png','foot-bearing.png')
$ImageReceipts=@()
foreach($Name in $Images){
    $Path=Join-Path $Out $Name
    $Bytes=[IO.File]::ReadAllBytes($Path)
    if($Bytes.Length -lt 24){throw "ShortPNG $Name"}
    $Signature=([BitConverter]::ToString($Bytes[0..7]))
    if($Signature -ne '89-50-4E-47-0D-0A-1A-0A'){throw "NotPNG $Name"}
    $Width=$Bytes[16]*16777216+$Bytes[17]*65536+$Bytes[18]*256+$Bytes[19]
    $Height=$Bytes[20]*16777216+$Bytes[21]*65536+$Bytes[22]*256+$Bytes[23]
    if($Width -ne 800 -or $Height -ne 800){throw "WrongPNGdimensions $Name"}
    $ImageReceipts += [ordered]@{name=$Name;width=$Width;height=$Height;bytes=$Bytes.Length;sha256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$Original=@(Get-Process blender,UnrealEditor | Select-Object Id,ProcessName,SessionId)
if($Original.Count -ne 2 -or -not($Original.Id -contains 6116) -or -not($Original.Id -contains 10404)){throw 'Unexpectedremainingprocess'}
$Preserved=Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
if(@($Preserved.PSObject.Properties).Count -ne 605){throw 'Wrongpreservationcount'}
foreach($Entry in $Preserved.PSObject.Properties){
    $Path=Join-Path $Root $Entry.Name
    if(-not(Test-Path -LiteralPath $Path -PathType Leaf)){throw "Missing $Path"}
    if((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Entry.Value){throw "Historicalmismatch $Path"}
}
$Tracked=@(git diff --name-only HEAD)
if($Tracked.Count -ne 0){throw 'Trackedoutsidescopechanges'}
$Receipt=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');status='BOUNDED_ART_COMPLETE';base='19d6ea4d28dfa9435d3af8b0fa0be89f05166225';completion=$Completion;readback_file_sha256=$Hashes[0];final_blend_sha256=(Get-FileHash -LiteralPath (Join-Path $Out 'TerracePergolaModule01.blend') -Algorithm SHA256).Hash.ToLowerInvariant();images=$ImageReceipts;historical_preserved=605;original_processes=$Original;os_exit_code='UNKNOWN wrapper numeric unavailable';native_integration=$false}
NewJson 'closure-receipt.json' $Receipt
$Owned=[ordered]@{}
foreach($File in (Get-ChildItem -LiteralPath $Out -File | Sort-Object Name)){
    $Owned[$File.Name]=(Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
}
NewJson 'owned-file-hashes.json' $Owned
$Receipt | ConvertTo-Json -Depth 10
