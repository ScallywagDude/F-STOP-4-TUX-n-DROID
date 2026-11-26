param(
    [string]$Destination = (Join-Path ([Environment]::GetFolderPath('Desktop')) 'F-STOP-GO')
)

$ErrorActionPreference = 'Stop'
$root = Split-Path $PSScriptRoot -Parent
$source = Join-Path $root '.port-build/win-run'
$built = Join-Path $root '.port-build/win-x64'
$destinationPath = [IO.Path]::GetFullPath($Destination)
if ($destinationPath.TrimEnd('\') -eq $source.TrimEnd('\')) {
    throw 'The desktop build must have a separate destination.'
}

foreach ($name in @('client', 'server')) {
    $compiled = Join-Path $built "game/$name/$name.dll"
    $staged = Join-Path $source "fstop/bin/x64/$name.dll"
    if ((Get-FileHash -LiteralPath $compiled).Hash -ne (Get-FileHash -LiteralPath $staged).Hash) {
        throw "The runtime staging folder has an outdated $name.dll."
    }
}

New-Item -ItemType Directory -Path $destinationPath -Force | Out-Null
$excludedDirectories = @('fstop/SAVE', 'fstop/screenshots', 'fstop/testscripts') |
    ForEach-Object { Join-Path $source $_ }
$excludedFiles = @('old_portalgun_demo.bsp', '*.log', '*.tmp', '*.mdmp', '*.dmp', 'modelsounds.cache', 'fstop_gamestats.dat', 'voice_ban.dt', 'stats.txt', 'glshaders.cfg', 'README.md')
if (Test-Path -LiteralPath (Join-Path $destinationPath 'fstop/cfg/video.txt')) { $excludedFiles += 'video.txt' }
# Preserve settings if this command is later used to update an existing install.
if (Test-Path -LiteralPath (Join-Path $destinationPath 'fstop/cfg/config.cfg')) {
    $excludedFiles += 'config.cfg'
}
$copyLog = Join-Path $root '.port-build/desktop-stage-copy.log'
& robocopy.exe $source $destinationPath /E /COPY:DAT /DCOPY:DAT /MT:8 /R:1 /W:1 /XJ /NP /NFL /NDL /NJH /NJS "/LOG:$copyLog" /XD $excludedDirectories /XF $excludedFiles
if ($LASTEXITCODE -ge 8) { throw "Runtime copy failed; see $copyLog" }

$launcher = @'
@echo off
pushd "%~dp0"
start "" /D "%~dp0" "%~dp0F-STOP.exe" -insecure -game fstop -multirun -novid -nomessagebox -condebug %*
popd
'@
[IO.File]::WriteAllText((Join-Path $destinationPath 'Play F-STOP.bat'), ($launcher -replace "`r?`n", "`r`n"), [Text.Encoding]::ASCII)

# Do not carry an unfocused performance-test setting into the desktop build.
$config = Join-Path $destinationPath 'fstop/cfg/config.cfg'
if (Test-Path -LiteralPath $config) {
    $text = [IO.File]::ReadAllText($config)
    $text = $text -replace '(?m)^engine_no_focus_sleep\s+"?\d+"?\s*$', 'engine_no_focus_sleep "50"'
    [IO.File]::WriteAllText($config, $text, [Text.Encoding]::ASCII)
}

$verifyFiles = @('F-STOP.exe', 'fstop/particles/particles_manifest.txt', 'fstop/gameinfo.txt')
$verifyFiles += 'fstop/materials/dev/fstop_camera_dof.vmt'
$verifyFiles += @('fstop/materials/fstop/elevator_floor.vmt', 'fstop/models/popup/angular_set/wallbuilder_mousetrap.vvd', 'fstop/resource/ui/basemodui/advancedvideo.res', 'fstop/resource/ui/basemodui/video.res', 'fstop/cfg/config_default.cfg')
$verifyFiles += Get-ChildItem -LiteralPath (Join-Path $source 'platform/shaders/fxc') -Filter '*.vcs' |
    ForEach-Object { 'platform/shaders/fxc/' + $_.Name }
$verifyFiles += Get-ChildItem -LiteralPath (Join-Path $source 'bin/x64') -Filter '*.dll' |
    ForEach-Object { 'bin/x64/' + $_.Name }
$verifyFiles += Get-ChildItem -LiteralPath (Join-Path $source 'fstop/bin/x64') -Filter '*.dll' |
    ForEach-Object { 'fstop/bin/x64/' + $_.Name }
$verifyFiles += Get-ChildItem -LiteralPath (Join-Path $source 'fstop/maps') -Filter '*.bsp' |
    ForEach-Object { 'fstop/maps/' + $_.Name }
$verified = foreach ($relative in $verifyFiles) {
    $a = Get-FileHash -LiteralPath (Join-Path $source $relative)
    $b = Get-FileHash -LiteralPath (Join-Path $destinationPath $relative)
    if ($a.Hash -ne $b.Hash) { throw "Content mismatch: $relative" }
    [pscustomobject]@{ path = $relative; sha256 = $b.Hash }
}
foreach ($archive in Get-ChildItem -LiteralPath (Join-Path $source 'fstop') -Filter '*.vpk') {
    $copy = Get-Item -LiteralPath (Join-Path $destinationPath ('fstop/' + $archive.Name))
    if ($copy.Length -ne $archive.Length) { throw "Incomplete archive: $($archive.Name)" }
}
$info = [pscustomobject]@{
    builtAt = (Get-Date).ToString('o')
    maps = @(Get-ChildItem -LiteralPath (Join-Path $destinationPath 'fstop/maps') -Filter '*.bsp').Count
    verifiedFiles = @($verified)
}
$info | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $destinationPath 'build-info.json') -Encoding utf8
[IO.File]::WriteAllText((Join-Path $destinationPath 'README.txt'), "Run Play F-STOP.bat to play.`r`nWindows x64 build with $($info.maps) compiled maps and baked cubemaps.`r`n", [Text.Encoding]::ASCII)
Write-Output "Staged $destinationPath; $($info.maps) maps, $(@($verified).Count) files hash-verified."
# Robocopy uses nonzero success codes; callers should receive success here.
$global:LASTEXITCODE = 0
