$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output/unreal-raised-flower-bed01-20261015'
function NewJson($Name,$Value){
    $Bytes=[Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 20)+"`n")
    $Stream=[IO.File]::Open((Join-Path $Out $Name),[IO.FileMode]::CreateNew)
    try{$Stream.Write($Bytes,0,$Bytes.Length)}finally{$Stream.Dispose()}
}
$Failure=Test-Path -LiteralPath (Join-Path $Out 'failure.json')
$Complete=Test-Path -LiteralPath (Join-Path $Out 'completion.json')
$Receipt=[ordered]@{utc=[DateTime]::UtcNow.ToString('o');base='42b35e250a561e76b3f70e8824ac74754ade4161';status='FAILED_FROZEN_NO_RETRY';historical_preserved=650;native_integration=$false}
if($Failure){
    if($Complete){throw 'Contradictoryfailurecompletion'}
    $Receipt.failure=Get-Content -LiteralPath (Join-Path $Out 'failure.json') -Raw | ConvertFrom-Json
}else{
    if(-not $Complete){throw 'No completion no acceptance'}
    $Completion=Get-Content -LiteralPath (Join-Path $Out 'completion.json') -Raw | ConvertFrom-Json
    if($Completion.images -ne 5 -or $Completion.contacts -ne 13 -or -not $Completion.restored_exact_match){throw 'Incomplete'}
    $Receipt.status='BOUNDED_EDGING_SOIL_ART_COMPLETE'
    $Receipt.completion=$Completion
    $Readbacks=@('before-save-readback.json','reopened-readback.json','restored-used-readback.json')
    $Hashes=@($Readbacks | ForEach-Object {(Get-FileHash -LiteralPath (Join-Path $Out $_) -Algorithm SHA256).Hash.ToLowerInvariant()})
    if(@($Hashes | Select-Object -Unique).Count -ne 1){throw 'Readbackbytesmismatch'}
    $Receipt.readback_file_sha256=$Hashes[0]
    $Receipt.final_blend_sha256=(Get-FileHash -LiteralPath (Join-Path $Out 'RaisedFlowerBed01.blend') -Algorithm SHA256).Hash.ToLowerInvariant()
    $Receipt.images=@()
    foreach($Name in @('whole-high.png','reverse-low.png','corner-joint.png','player165.png','rim-soil.png')){
        $Path=Join-Path $Out $Name
        $Bytes=[IO.File]::ReadAllBytes($Path)
        if($Bytes.Length -lt 24 -or [BitConverter]::ToString($Bytes[0..7]) -ne '89-50-4E-47-0D-0A-1A-0A'){throw "InvalidPNG $Name"}
        $Width=$Bytes[16]*16777216+$Bytes[17]*65536+$Bytes[18]*256+$Bytes[19]
        $Height=$Bytes[20]*16777216+$Bytes[21]*65536+$Bytes[22]*256+$Bytes[23]
        if($Width -ne 800 -or $Height -ne 800){throw "Wrongdimensions $Name"}
        $Receipt.images += [ordered]@{name=$Name;width=$Width;height=$Height;bytes=$Bytes.Length;sha256=(Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()}
    }
}
if(Test-Path -LiteralPath (Join-Path $Out 'work-in-progress.blend')){$Receipt.wip_sha256=(Get-FileHash -LiteralPath (Join-Path $Out 'work-in-progress.blend') -Algorithm SHA256).Hash.ToLowerInvariant()}
$Receipt.source_sha256=(Get-FileHash -LiteralPath (Join-Path $Out 'source-excerpt.txt') -Algorithm SHA256).Hash.ToLowerInvariant()
$Original=@(Get-Process blender,UnrealEditor | Select-Object Id,ProcessName,SessionId)
if($Original.Count -ne 2 -or -not($Original.Id -contains 6116) -or -not($Original.Id -contains 10404)){throw 'Unexpectedremainingprocesses'}
$Receipt.original_processes=$Original
$Exit=Get-Content -LiteralPath (Join-Path $Out 'process-exit.json') -Raw | ConvertFrom-Json
$Receipt.os_exit_code='UNKNOWN numeric unavailable'
if($Exit.numeric_available){$Receipt.os_exit_code=$Exit.exit_code}
$Manifest=Get-Content -LiteralPath (Join-Path $Out 'preservation-before.json') -Raw | ConvertFrom-Json
if(@($Manifest.PSObject.Properties).Count -ne 650){throw 'Wronghistoricalcount'}
foreach($Entry in $Manifest.PSObject.Properties){
    $Path=Join-Path $Root $Entry.Name
    if(-not(Test-Path -LiteralPath $Path -PathType Leaf)){throw "Missing $Path"}
    if((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Entry.Value){throw "Historicalmismatch $Path"}
}
if(@(git diff --name-only HEAD).Count -ne 0){throw 'Existingtrackedchanges'}
NewJson 'closure-receipt.json' $Receipt
$Owned=[ordered]@{}
foreach($File in (Get-ChildItem -LiteralPath $Out -File | Sort-Object Name)){$Owned[$File.Name]=(Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
NewJson 'owned-file-hashes.json' $Owned
$Receipt | ConvertTo-Json -Depth 12
