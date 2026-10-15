$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game\output\unreal-entrance-stone-portal01-20261015'
$exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
foreach ($name in @('production.log','production-error.log','launch-identity.json','work-in-progress.blend')) {
    if (Test-Path -LiteralPath "$root/$name") { throw "Launch output already exists: $name" }
}
$p = Start-Process -FilePath $exe -ArgumentList @('--background','--factory-startup','--python-exit-code','1','--python',"$root\author_portal.py") -WindowStyle Hidden -PassThru -RedirectStandardOutput "$root\production.log" -RedirectStandardError "$root\production-error.log"
$memory = Get-CimInstance Win32_OperatingSystem
$data = [ordered]@{host=$env:COMPUTERNAME;pid=$p.Id;startUtc=$p.StartTime.ToUniversalTime().ToString('o');exe=$exe;job='entrance-stone-portal01';arguments='--background --factory-startup --python-exit-code 1 --python author_portal.py';freePhysicalKB=$memory.FreePhysicalMemory;totalPhysicalKB=$memory.TotalVisibleMemorySize}
$stream = [System.IO.File]::Open("$root/launch-identity.json",[System.IO.FileMode]::CreateNew)
$writer = New-Object System.IO.StreamWriter($stream,(New-Object System.Text.UTF8Encoding($false)))
try { $writer.Write(($data | ConvertTo-Json)) } finally { $writer.Dispose() }
$p.Id
