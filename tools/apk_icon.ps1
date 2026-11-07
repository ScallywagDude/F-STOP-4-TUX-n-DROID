# Scales a square PNG into the launcher icon sizes. usage: apk_icon.ps1 <in.png> <launcher res folder>
param([string]$src, [string]$res)
Add-Type -AssemblyName System.Drawing
$img = [System.Drawing.Image]::FromFile($src)
$sizes = @{ 'drawable' = 48; 'drawable-hdpi' = 72; 'drawable-xhdpi' = 96; 'drawable-xxhdpi' = 144; 'drawable-xxxhdpi' = 192 }
foreach ($d in $sizes.Keys) {
  $n = $sizes[$d]
  $bmp = New-Object System.Drawing.Bitmap($n, $n, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
  $g = [System.Drawing.Graphics]::FromImage($bmp)
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
  $g.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
  $g.DrawImage($img, 0, 0, $n, $n)
  $g.Dispose()
  $bmp.Save((Join-Path $res "$d\ic_launcher.png"), [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  "$d $n"
}
$img.Dispose()
