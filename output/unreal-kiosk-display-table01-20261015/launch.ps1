$ErrorActionPreference = 'Stop'
$root = 'C:\dev\cribtest-game\output\unreal-kiosk-display-table01-20261015'
$exe = 'C:\dev\tools\baddie-blender\blender-5.2.2-windows-x64\blender.exe'
foreach ($name in @('production.log','production-error.log','launch-identity.json','work-in-progress.blend')) {
    if (Test-Path -LiteralPath "$root/$name") { throw "Existing launch output: $name" }
}
$memory = Get-CimInstance Win32_OperatingSystem
$p = Start-Process -FilePath $exe -ArgumentList @('--background','--factory-startup','--python-exit-code','1','--python',"$root\author_table.py") -WindowStyle Hidden -PassThru -RedirectStandardOutput "$root\production.log" -RedirectStandardError "$root\production-error.log"
$data = [ordered]@{host=$env:COMPUTERNAME;pid=$p.Id;startUtc=$p.StartTime.ToUniversalTime().ToString('o');exe=$exe;job='kiosk-display-table01';arguments='--background --factory-startup --python-exit-code 1 --python author_table.py';freePhysicalKB=$memory.FreePhysicalMemory;totalPhysicalKB=$memory.TotalVisibleMemorySize}
$stream = [System.IO.File]::Open("$root/launch-identity.json",[System.IO.FileMode]::CreateNew)
$writer = New-Object System.IO.StreamWriter($stream,(New-Object System.Text.UTF8Encoding($false)))
try { $writer.Write(($data | ConvertTo-Json)) } finally { $writer.Dispose() }
$p.Id
