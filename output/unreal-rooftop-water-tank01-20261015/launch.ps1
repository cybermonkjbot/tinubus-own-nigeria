$ErrorActionPreference='Stop'
$root='C:\dev\cribtest-game\output\unreal-rooftop-water-tank01-20261015'
$exe='C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
foreach($name in @('production.log','production-error.log','launch-identity.json','work-in-progress.blend')) {
    if(Test-Path -LiteralPath "$root/$name"){throw "Existing launch output:$name"}
}
$source=Get-Item -LiteralPath "$root/source-excerpt.txt"
if($source.Length -ne 7491 -or (Get-FileHash -LiteralPath $source.FullName -Algorithm SHA256).Hash.ToLowerInvariant() -ne '108533c8bde50426416bccd081f72c4ced7fcbfdc4fca160157552836a7047a9'){throw 'Exactsource mismatch'}
$memory=Get-CimInstance Win32_OperatingSystem
$p=Start-Process -FilePath $exe -ArgumentList @('--background','--factory-startup','--python-exit-code','1','--python',"$root\author_tank.py") -WindowStyle Hidden -PassThru -RedirectStandardOutput "$root\production.log" -RedirectStandardError "$root\production-error.log"
$data=[ordered]@{host=$env:COMPUTERNAME;pid=$p.Id;startUtc=$p.StartTime.ToUniversalTime().ToString('o');exe=$exe;job='rooftop-water-tank01';arguments='--background --factory-startup --python-exit-code 1 --python author_tank.py';freePhysicalKB=$memory.FreePhysicalMemory;totalPhysicalKB=$memory.TotalVisibleMemorySize}
$stream=[System.IO.File]::Open("$root/launch-identity.json",[System.IO.FileMode]::CreateNew)
$writer=New-Object System.IO.StreamWriter($stream,(New-Object System.Text.UTF8Encoding($false)))
try{$writer.Write(($data|ConvertTo-Json))}finally{$writer.Dispose()}
$p.Id
