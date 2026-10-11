param([string]$OutputDirectory = $PSScriptRoot, [string]$FileName = 'typography-overlay.png')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$target = Join-Path $OutputDirectory ('textures/' + $FileName)
if (Test-Path -LiteralPath $target) { throw "Refusing to overwrite $target" }
[void][System.IO.Directory]::CreateDirectory((Split-Path $target))
$bitmap = New-Object System.Drawing.Bitmap(2048, 2048)
$graphics = [System.Drawing.Graphics]::FromImage($bitmap)
$graphics.Clear([System.Drawing.Color]::Transparent)
$graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$random = New-Object System.Random(419)
function Write-Tile([int]$Tile, [string[]]$Lines, [float]$Size, [System.Drawing.Color]$Color) {
    $x = ($Tile % 4) * 512 + 44
    $y = [math]::Floor($Tile / 4) * 512 + 50
    $font = New-Object System.Drawing.Font('Arial', $Size, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    while (($Lines | ForEach-Object { $graphics.MeasureString($_, $font).Width } | Measure-Object -Maximum).Maximum -gt 420) {
        $font.Dispose()
        $Size -= 1
        $font = New-Object System.Drawing.Font('Arial', $Size, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    }
    $brush = New-Object System.Drawing.SolidBrush($Color)
    try {
        foreach ($line in $Lines) {
            $graphics.DrawString($line, $font, $brush, [float]$x, [float]$y)
            $y += $Size * 1.22
        }
    } finally { $brush.Dispose(); $font.Dispose() }
}
Write-Tile 4 @('PURE WATER', 'DRINKING WATER', '500 ml') 39 ([System.Drawing.Color]::FromArgb(150, 24, 87, 111))
Write-Tile 5 @('SAUSAGE', 'ROLL', 'FRESHLY BAKED') 50 ([System.Drawing.Color]::FromArgb(235, 245, 224, 148))
Write-Tile 6 @('COMMUNITY', 'FIRST', 'ADE FOLARIN', 'FOR COUNCIL') 48 ([System.Drawing.Color]::FromArgb(220, 32, 66, 55))
Write-Tile 7 @('A NIGHT OF', 'PRAISE', 'NEW DAWN CHAPEL', 'FRIDAY / 6 PM') 44 ([System.Drawing.Color]::FromArgb(225, 66, 34, 66))
Write-Tile 8 @('IN LOVING MEMORY', 'TUNDE ADEYEMI', 'FOREVER IN', 'OUR HEARTS') 39 ([System.Drawing.Color]::FromArgb(215, 54, 48, 43))
Write-Tile 9 @('THIS HOUSE IS', 'NOT FOR SALE', 'BEWARE OF 419') 40 ([System.Drawing.Color]::FromArgb(225, 47, 39, 32))
Write-Tile 10 @('POST', 'NO BILL') 76 ([System.Drawing.Color]::FromArgb(215, 53, 40, 29))
Write-Tile 11 @('PHONE REPAIRS', 'SCREENS / CHARGING', 'ACCESSORIES') 35 ([System.Drawing.Color]::FromArgb(225, 239, 221, 171))
# Wear removes ink from the overlay only, preserving unlit underlying paper.
$graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$clear = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::Transparent)
try {
    for ($i = 0; $i -lt 5200; $i++) {
        $graphics.FillRectangle($clear, $random.Next(0,2048), $random.Next(512,1536), $random.Next(1,7), $random.Next(1,4))
    }
    $bitmap.Save($target, [System.Drawing.Imaging.ImageFormat]::Png)
} finally { $clear.Dispose(); $graphics.Dispose(); $bitmap.Dispose() }
