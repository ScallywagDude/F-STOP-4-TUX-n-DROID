<#
.SYNOPSIS
Package and sign the Android launcher with staged ARM64 native libraries.
.EXAMPLE
./tools/build_apk.ps1 -ToolchainRoot C:/path/to/tools -KeyStorePath C:/path/to/fstop.keystore -Debuggable
#>
[CmdletBinding()]
param(
    [string]$ToolchainRoot = $PSScriptRoot,
    [string]$LibrariesPath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'install/lib/arm64-v8a'),
    [string]$OutputPath = (Join-Path (Split-Path -Parent $PSScriptRoot) 'out/F-STOP-Android-arm64.apk'),
    [string]$JavaHome = $env:JAVA_HOME,
    [Parameter(Mandatory = $true)][string]$KeyStorePath,
    [string]$KeyAlias = 'fstop',
    [string]$KeyStorePassword = 'fstoplocal',
    [string]$KeyPassword = 'fstoplocal',
    [string]$AssetRoot,
    [switch]$Debuggable
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$launcherRoot = Join-Path $repoRoot 'launcher'
$toolsRoot = (Resolve-Path -LiteralPath $ToolchainRoot).Path
$librariesRoot = (Resolve-Path -LiteralPath $LibrariesPath).Path
$keyStore = (Resolve-Path -LiteralPath $KeyStorePath).Path
$output = [IO.Path]::GetFullPath($OutputPath)
$workRoot = Join-Path $repoRoot ('apkbuild/' + [Guid]::NewGuid().ToString('N'))
$buildTools = Join-Path $toolsRoot 'sdk/android-14'
$androidJar = Join-Path $toolsRoot 'sdk/android-34/android.jar'
$d8Jar = Join-Path $toolsRoot 'sdk/bt36/android-16/lib/d8.jar'
$signerJar = Join-Path $buildTools 'lib/apksigner.jar'
$java = Join-Path $JavaHome 'bin/java.exe'
$javac = Join-Path $JavaHome 'bin/javac.exe'
$jar = Join-Path $JavaHome 'bin/jar.exe'
$aapt = Join-Path $buildTools 'aapt2.exe'
$zipalign = Join-Path $buildTools 'zipalign.exe'
$node = (Get-Command node -ErrorAction Stop).Source
foreach ($requiredFile in @($androidJar, $d8Jar, $signerJar, $java, $javac, $jar, $aapt, $zipalign)) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required packaging tool is missing: $requiredFile"
    }
}
$nativeLibraries = @(Get-ChildItem -LiteralPath $librariesRoot -File -Filter '*.so' | Where-Object Name -ne 'libvtex_dll.so')
foreach ($requiredLibrary in @('liblauncher.so', 'libengine.so', 'libclient.so', 'libserver.so', 'libtogl.so')) {
    if ($requiredLibrary -notin $nativeLibraries.Name) { throw "Missing native library: $requiredLibrary" }
}

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "Tool failed (exit $LASTEXITCODE): $Program" }
}

# Each run has its own work directory. No recursive deletion or external writes
# are needed, and a supplied signing key is only read.
foreach ($subdirectory in @('res', 'classes', 'dex', 'gen', 'stage/lib/arm64-v8a')) {
    [void][IO.Directory]::CreateDirectory((Join-Path $workRoot $subdirectory))
}
[void][IO.Directory]::CreateDirectory((Split-Path -Parent $output))
$resources = Join-Path $workRoot 'res/compiled.zip'
$baseApk = Join-Path $workRoot 'base.apk'
Invoke-Checked $aapt @('compile', '--dir', (Join-Path $launcherRoot 'res'), '-o', $resources)
$linkArguments = @('link', '-o', $baseApk, '-I', $androidJar,
    '--manifest', (Join-Path $launcherRoot 'AndroidManifest.xml'),
    '--min-sdk-version', '24', '--target-sdk-version', '34',
    '--version-code', '2', '--version-name', '1.1', '--replace-version',
    '--java', (Join-Path $workRoot 'gen'))
if ($Debuggable) { $linkArguments += '--debug-mode' }
if (Test-Path -LiteralPath (Join-Path $launcherRoot 'assets') -PathType Container) {
    $linkArguments += @('-A', (Join-Path $launcherRoot 'assets'))
}
$linkArguments += $resources
Invoke-Checked $aapt $linkArguments

$javaFiles = @(Get-ChildItem -LiteralPath (Join-Path $launcherRoot 'src'), (Join-Path $workRoot 'gen') -Recurse -File -Filter '*.java' | ForEach-Object FullName)
$compilerArguments = @('-nowarn', '-Xlint:none', '--release', '8', '-encoding', 'UTF-8', '-cp', $androidJar, '-d', (Join-Path $workRoot 'classes')) + $javaFiles
Invoke-Checked $javac $compilerArguments
$classesJar = Join-Path $workRoot 'classes.jar'
Invoke-Checked $jar @('cf', $classesJar, '-C', (Join-Path $workRoot 'classes'), '.')
Invoke-Checked $java @('-cp', $d8Jar, 'com.android.tools.r8.D8', '--release', '--lib', $androidJar, '--min-api', '24', '--output', (Join-Path $workRoot 'dex'), $classesJar)

Copy-Item -LiteralPath (Join-Path $workRoot 'dex/classes.dex') -Destination (Join-Path $workRoot 'stage/classes.dex')
foreach ($library in $nativeLibraries) {
    Copy-Item -LiteralPath $library.FullName -Destination (Join-Path $workRoot 'stage/lib/arm64-v8a')
}
Copy-Item -LiteralPath (Join-Path $repoRoot 'engine/lib/android/aarch64/libSDL2.so') -Destination (Join-Path $workRoot 'stage/lib/arm64-v8a') -Force
$zipArguments = @((Join-Path $PSScriptRoot 'zipadd.js'), $baseApk, (Join-Path $workRoot 'stage'))
if ($AssetRoot) { $zipArguments += (Resolve-Path -LiteralPath $AssetRoot).Path }
Invoke-Checked $node $zipArguments
$alignedApk = Join-Path $workRoot 'aligned.apk'
Invoke-Checked $zipalign @('-f', '-p', '16384', $baseApk, $alignedApk)

$savedStorePassword = $env:FSTOP_APK_KEYSTORE_PASSWORD
$savedKeyPassword = $env:FSTOP_APK_KEY_PASSWORD
try {
    $env:FSTOP_APK_KEYSTORE_PASSWORD = $KeyStorePassword
    $env:FSTOP_APK_KEY_PASSWORD = $KeyPassword
    Invoke-Checked $java @('-jar', $signerJar, 'sign', '--ks', $keyStore,
        '--ks-key-alias', $KeyAlias, '--ks-pass', 'env:FSTOP_APK_KEYSTORE_PASSWORD',
        '--key-pass', 'env:FSTOP_APK_KEY_PASSWORD', '--min-sdk-version', '24',
        '--out', $output, $alignedApk)
} finally {
    $env:FSTOP_APK_KEYSTORE_PASSWORD = $savedStorePassword
    $env:FSTOP_APK_KEY_PASSWORD = $savedKeyPassword
}
Invoke-Checked $java @('-jar', $signerJar, 'verify', '--print-certs', $output)
Write-Output "Built APK: $output"
