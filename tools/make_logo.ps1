# Draws the menu logo on a transparent background and writes it as a VTF. usage: make_logo.ps1 <out.vtf> <preview.png> [r g b]
param([string]$out, [string]$preview, [int]$r = 18, [int]$g2 = 30, [int]$b = 38)
Add-Type -AssemblyName System.Drawing
$S = 512
$bmp = New-Object System.Drawing.Bitmap($S, $S, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$g = [System.Drawing.Graphics]::FromImage($bmp)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
$g.Clear([System.Drawing.Color]::Transparent)

$path = New-Object System.Drawing.Drawing2D.GraphicsPath
$family = New-Object System.Drawing.FontFamily('Georgia')
$style = [int]([System.Drawing.FontStyle]::Bold -bor [System.Drawing.FontStyle]::Italic)
$fmt = New-Object System.Drawing.StringFormat
$fmt.Alignment = [System.Drawing.StringAlignment]::Center
$fmt.LineAlignment = [System.Drawing.StringAlignment]::Center
$fmt.FormatFlags = [System.Drawing.StringFormatFlags]::NoWrap
$path.AddString('f/', $family, $style, 360, (New-Object System.Drawing.RectangleF(0, 0, $S, $S)), $fmt)
# centre what was actually drawn
$box = $path.GetBounds()
$move = New-Object System.Drawing.Drawing2D.Matrix
$move.Translate(($S - $box.Width) / 2 - $box.X, ($S - $box.Height) / 2 - $box.Y)
$path.Transform($move)
$g.FillPath((New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, $r, $g2, $b))), $path)
$g.Dispose()
$bmp.Save($preview, [System.Drawing.Imaging.ImageFormat]::Png)

# VTF 7.2, RGBA8888, no mips, clamped, eight bit alpha
$header = 80
$bytes = New-Object byte[] ($header + $S * $S * 4)
$w = New-Object IO.BinaryWriter(New-Object IO.MemoryStream(, $bytes))
$w.Write([byte[]][char[]]"VTF`0")
$w.Write([int]7); $w.Write([int]2); $w.Write([int]$header)
$w.Write([uint16]$S); $w.Write([uint16]$S)
$w.Write([int](0x4 -bor 0x8 -bor 0x100 -bor 0x200 -bor 0x2000))
$w.Write([uint16]1); $w.Write([uint16]0)
$w.Write([int]0)
$w.Write([single]0.2); $w.Write([single]0.2); $w.Write([single]0.2)
$w.Write([int]0)
$w.Write([single]1)
$w.Write([int]0)          # RGBA8888
$w.Write([byte]1)         # one mip
$w.Write([int]-1)         # no low-res image
$w.Write([byte]0); $w.Write([byte]0)
$w.Write([uint16]1)       # depth
$rect = New-Object System.Drawing.Rectangle(0, 0, $S, $S)
$data = $bmp.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::ReadOnly, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$raw = New-Object byte[] ($S * $S * 4)
[Runtime.InteropServices.Marshal]::Copy($data.Scan0, $raw, 0, $raw.Length)
$bmp.UnlockBits($data); $bmp.Dispose()
for ($i = 0; $i -lt $raw.Length; $i += 4) {
  $o = $header + $i
  $bytes[$o] = $raw[$i + 2]; $bytes[$o + 1] = $raw[$i + 1]; $bytes[$o + 2] = $raw[$i]; $bytes[$o + 3] = $raw[$i + 3]
}
[IO.File]::WriteAllBytes($out, $bytes)
'logo written'
