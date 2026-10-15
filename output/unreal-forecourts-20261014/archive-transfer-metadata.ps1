$ErrorActionPreference = 'Stop'
$taskRepo = 'C:\dev\cribtest-game'
$taskScope = Join-Path $taskRepo 'output\unreal-forecourts-20261014'
$taskArchive = Join-Path $taskRepo '.git\operations\forecourt-transfer-metadata-20261014'
$taskSources = @(
  'apps/companion-unreal/Scripts/StreetFinish/add_forecourts.py',
  'apps/companion-unreal/Scripts/StreetFinish/export_forecourt_kit.py',
  'apps/companion-unreal/Scripts/StreetFinish/add_biomes.py',
  'apps/companion-unreal/Scripts/StreetFinish/README.md',
  'apps/companion-unreal/Content/Python/baddie_street_tools.py',
  'docs/engineering/unreal-eye-level-biomes-2026-10-14.md',
  'docs/engineering/unreal-forecourts-ground-2026-10-14.md'
)
$taskFiles = @(Get-ChildItem -LiteralPath $taskScope -Recurse -File -Filter '._*')
foreach ($taskSource in $taskSources) {
  $taskPath = Join-Path $taskRepo $taskSource
  $taskSidecar = Join-Path (Split-Path $taskPath) ('._' + (Split-Path $taskPath -Leaf))
  if (Test-Path -LiteralPath $taskSidecar) { $taskFiles += Get-Item -LiteralPath $taskSidecar }
}
foreach ($taskFile in $taskFiles) {
  $taskBytes = [System.IO.File]::ReadAllBytes($taskFile.FullName)
  if ($taskBytes.Length -lt 4 -or $taskBytes[0] -ne 0 -or $taskBytes[1] -ne 5 -or $taskBytes[2] -ne 22 -or $taskBytes[3] -ne 7) {
    throw "Not AppleDouble metadata: $($taskFile.FullName)"
  }
  $taskRelative = $taskFile.FullName.Substring($taskRepo.Length + 1)
  if (Test-Path -LiteralPath (Join-Path $taskArchive $taskRelative)) { throw 'Archive target already exists' }
}
foreach ($taskFile in $taskFiles) {
  $taskRelative = $taskFile.FullName.Substring($taskRepo.Length + 1)
  $taskDestination = Join-Path $taskArchive $taskRelative
  [System.IO.Directory]::CreateDirectory((Split-Path $taskDestination)) | Out-Null
  Move-Item -LiteralPath $taskFile.FullName -Destination $taskDestination
}
Write-Output "Archived $($taskFiles.Count) verified transfer metadata files to $taskArchive"
