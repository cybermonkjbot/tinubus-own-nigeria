$ErrorActionPreference='Stop'
$Root='C:\dev\cribtest-game'
$Out=Join-Path $Root 'output/unreal-terrace-pergola-module01-20261015'
$Exe='C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
function NewJson($Name,$Value){
    $Bytes=[Text.UTF8Encoding]::new($false).GetBytes(($Value | ConvertTo-Json -Depth 12)+"`n")
    $Stream=[IO.File]::Open((Join-Path $Out $Name),[IO.FileMode]::CreateNew)
    try{$Stream.Write($Bytes,0,$Bytes.Length)}finally{$Stream.Dispose()}
}
if($env:COMPUTERNAME -ne 'Cribtest11'){throw 'Wronghost'}
if((Get-Location).Path -ne $Root){throw 'Wrongroot'}
$Head=git rev-parse HEAD
if($Head -ne '19d6ea4d28dfa9435d3af8b0fa0be89f05166225'){throw 'WrongHEAD'}
$Tracked=@(git status --porcelain --untracked-files=no)
if($Tracked.Count -ne 0){throw 'Trackedchanges'}
$Original=@(Get-Process blender,UnrealEditor | Select-Object Id,ProcessName,SessionId)
if($Original.Count -ne 2 -or -not($Original.Id -contains 6116) -or -not($Original.Id -contains 10404)){throw 'Unexpectedprocesses'}
$Source=Join-Path $Out 'source-excerpt.txt'
if((Get-Item $Source).Length -ne 7752 -or (Get-FileHash $Source -Algorithm SHA256).Hash.ToLowerInvariant() -ne '8c574f67e7b0e906a12b5709efaeba931f64cfbeb38e3b73c837a686b2b14067'){throw 'Sourceexcerptmismatch'}
foreach($Name in @('production.log','production-error.log','initial-identity.json','work-in-progress.blend','failure.json')){
    if(Test-Path -LiteralPath (Join-Path $Out $Name)){throw "Priorattemptartifact $Name"}
}
NewJson 'launch-before.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');host=$env:COMPUTERNAME;root=$Root;base=$Head;original=$Original;executable=$Exe;job='terrace-pergola-module01';model_selection='requested gpt-6.1-sol/medium not independently observable'})
$Script=Join-Path $Out 'author_pergola.py'
$Arguments=@('--background','--factory-startup','--disable-autoexec','--threads','6','--python-exit-code','1','--python',$Script)
$Process=Start-Process -FilePath $Exe -ArgumentList $Arguments -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $Out 'production.log') -RedirectStandardError (Join-Path $Out 'production-error.log')
NewJson 'launch-identity.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');host=$env:COMPUTERNAME;pid=$Process.Id;arguments=$Arguments;original=$Original})
$Process.WaitForExit()
$Code=$Process.ExitCode
NewJson 'process-exit.json' ([ordered]@{utc=[DateTime]::UtcNow.ToString('o');pid=$Process.Id;exit_code=$Code;numeric_available=($null -ne $Code);original=@(Get-Process -Id 6116,10404 | Select-Object Id,ProcessName,SessionId)})
if(Test-Path -LiteralPath (Join-Path $Out 'failure.json')){throw 'FAILED/FROZEN; no retry'}
if(-not(Test-Path -LiteralPath (Join-Path $Out 'completion.json'))){throw 'No completion; no retry'}
