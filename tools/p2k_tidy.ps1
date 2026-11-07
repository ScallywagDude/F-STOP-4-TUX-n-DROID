# Removes this project's test leftovers from the real Project 2008 game folder. The junction is unlinked, so what it points to stays.
$g = Join-Path $env:P2008_DIR "game"
Get-Process p2008 -ErrorAction SilentlyContinue | ForEach-Object { try { $_.Kill() } catch {} }
Start-Sleep 1
$j = Join-Path $g 'fstop_fixes'
if (Test-Path $j) {
  $item = Get-Item $j -Force
  if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { $item.Delete(); 'unlinked fstop_fixes' } else { 'fstop_fixes is a real folder, left alone' }
}
$t = Join-Path $g '_fixtest'
if (Test-Path $t) { Remove-Item $t -Recurse -Force; 'removed _fixtest' }
(Get-ChildItem $g | Select-Object -ExpandProperty Name) -join ' '
Select-String -Path (Join-Path $g 'aperture\gameinfo.txt') -Pattern 'fstop_fixes' | Measure-Object | Select-Object -ExpandProperty Count
