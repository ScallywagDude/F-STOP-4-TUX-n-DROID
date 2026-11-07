# Draws the F-STOP launcher icon: a tilted instant photo with the f-number mark on it. usage: apk_icon_draw.ps1 <out.png> <mark.png>
param([string]$out, [string]$mark)
Add-Type -AssemblyName System.Drawing
$S = 512
$bmp = New-Object System.Drawing.Bitmap($S, $S, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit
$g.Clear([System.Drawing.Color]::Transparent)

function RoundRect([float]$x, [float]$y, [float]$w, [float]$h, [float]$r) {
  $p = New-Object System.Drawing.Drawing2D.GraphicsPath
  $d = $r * 2
  $p.AddArc($x, $y, $d, $d, 180, 90)
  $p.AddArc($x + $w - $d, $y, $d, $d, 270, 90)
  $p.AddArc($x + $w - $d, $y + $h - $d, $d, $d, 0, 90)
  $p.AddArc($x, $y + $h - $d, $d, $d, 90, 90)
  $p.CloseFigure()
  return $p
}

# background
$bg = RoundRect 0 0 $S $S 96
$rect = New-Object System.Drawing.Rectangle(0, 0, $S, $S)
$fill = New-Object System.Drawing.Drawing2D.LinearGradientBrush($rect, [System.Drawing.Color]::FromArgb(255, 22, 58, 68), [System.Drawing.Color]::FromArgb(255, 10, 24, 32), 90)
$g.FillPath($fill, $bg)

# the photo, tilted
$g.TranslateTransform(256, 256)
$g.RotateTransform(-9)
$shadow = RoundRect (-146) (-164) 300 348 14
$g.FillPath((New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(110, 0, 0, 0))), $shadow)
$print = RoundRect (-154) (-176) 308 352 14
$g.FillPath((New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 244, 238, 224))), $print)

# the picture in it, orange over a low horizon
$picture = New-Object System.Drawing.RectangleF(-130, -152, 260, 248)
$sky = New-Object System.Drawing.Drawing2D.LinearGradientBrush($picture, [System.Drawing.Color]::FromArgb(255, 250, 150, 40), [System.Drawing.Color]::FromArgb(255, 226, 92, 28), 90)
$g.FillRectangle($sky, $picture)
$g.FillRectangle((New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 22, 58, 68))), -130, 62, 260, 34)

# the mark
$font = New-Object System.Drawing.Font('Georgia', 150, ([System.Drawing.FontStyle]::Bold -bor [System.Drawing.FontStyle]::Italic), [System.Drawing.GraphicsUnit]::Pixel)
$fmt = New-Object System.Drawing.StringFormat
$fmt.Alignment = [System.Drawing.StringAlignment]::Center
$fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
$ink = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 18, 30, 38))
$g.DrawString('f/', $font, $ink, (New-Object System.Drawing.RectangleF(-130, -160, 260, 230)), $fmt)

# the mark on the print's lower left border, turned black with its edges kept soft
$src = New-Object System.Drawing.Bitmap($mark)
$least = 255
for ($y = 0; $y -lt $src.Height; $y++) { for ($x = 0; $x -lt $src.Width; $x++) {
  $c = $src.GetPixel($x, $y)
  if ($c.A -gt 200) { $v = [int](($c.R + $c.G + $c.B) / 3); if ($v -lt $least) { $least = $v } }
} }
$tinted = New-Object System.Drawing.Bitmap($src.Width, $src.Height, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
for ($y = 0; $y -lt $src.Height; $y++) { for ($x = 0; $x -lt $src.Width; $x++) {
  $c = $src.GetPixel($x, $y)
  $v = ($c.R + $c.G + $c.B) / 3
  $cover = [Math]::Min(1.0, [Math]::Max(0.0, (255 - $v) / [Math]::Max(1, 255 - $least)))
  $tinted.SetPixel($x, $y, [System.Drawing.Color]::FromArgb([int]($c.A * $cover), 0, 0, 0))
} }
$g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
$g.DrawImage($tinted, -130, 104, 64, 64)
$src.Dispose(); $tinted.Dispose()

$g.Dispose()
$bmp.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$bmp.Dispose()
'drawn'
