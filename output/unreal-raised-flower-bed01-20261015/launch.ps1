$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output/unreal-raised-flower-bed01-20261015'
$Exe='C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
function NewJson($Name,$Value){
    $Bytes=[Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 20)+"`n")
    $Stream=[IO.File]::Open((Join-Path $Out $Name),[IO.FileMode]::CreateNew)
    try{$Stream.Write($Bytes,0,$Bytes.Length)}finally{$Stream.Dispose()}
}
if($env:COMPUTERNAME -ne 'Cribtest11' -or (Get-Location).Path -ne $Root){throw 'Wronghostroot'}
$Head=git rev-parse HEAD
if($Head -ne '42b35e250a561e76b3f70e8824ac74754ade4161'){throw 'WrongHEAD'}
if(@(git status --porcelain --untracked-files=no).Count -ne 0){throw 'Trackedchanges'}
$Original=@(Get-Process blender,UnrealEditor | Select-Object Id,ProcessName,SessionId)
if($Original.Count -ne 2){throw 'Unexpectedprocesscount'}
if(@($Original | Where-Object {$_.Id -eq 6116 -and $_.SessionId -eq 0 -and $_.ProcessName -eq 'blender'}).Count -ne 1){throw 'WrongoriginalBlender'}
if(@($Original | Where-Object {$_.Id -eq 10404 -and $_.SessionId -eq 2 -and $_.ProcessName -eq 'UnrealEditor'}).Count -ne 1){throw 'WrongoriginalUnreal'}
$Source=Join-Path $Out 'source-excerpt.txt'
if((Get-Item $Source).Length -ne 9953 -or (Get-FileHash $Source -Algorithm SHA256).Hash.ToLowerInvariant() -ne '288fd846ff0af691c57637029d35c2de5d779ee1788577c9b75c87d12b5ecacc'){throw 'Exactsourcefailed'}
foreach($Name in @('initial-identity.json','production.log','production-error.log','work-in-progress.blend','failure.json','completion.json')){
    if(Test-Path -LiteralPath (Join-Path $Out $Name)){throw "Priorattempt $Name"}
}
NewJson 'launch-before.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');host=$env:COMPUTERNAME;root=$Root;base=$Head;original=$Original;job='raised-flower-bed01';model='requested gpt-6.1-sol/medium not independently observable'})
$Script=Join-Path $Out 'author_flower_bed.py'
$Arguments=@('--background','--factory-startup','--disable-autoexec','--threads','6','--python-exit-code','1','--python',$Script)
$Process=Start-Process -FilePath $Exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $Out 'production.log') -RedirectStandardError (Join-Path $Out 'production-error.log')
NewJson 'launch-identity.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');host=$env:COMPUTERNAME;pid=$Process.Id;arguments=$Arguments;executable=$Exe;original=$Original})
$Process.WaitForExit()
$Code=$Process.ExitCode
NewJson 'process-exit.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');pid=$Process.Id;exit_code=$Code;numeric_available=($null -ne $Code);original=@(Get-Process -Id 6116,10404 | Select-Object Id,ProcessName,SessionId)})
if(Test-Path -LiteralPath (Join-Path $Out 'failure.json')){throw 'FAILED/FROZEN no retry'}
if(-not(Test-Path -LiteralPath (Join-Path $Out 'completion.json'))){throw 'No completion no retry'}
