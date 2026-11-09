<#
.SYNOPSIS
Build the Android ARM64 engine using the existing NDK r10e / host Clang toolchain.
.EXAMPLE
./tools/build_android.ps1 -ToolchainRoot C:/path/to/toolchain/tools
.EXAMPLE
./tools/build_android.ps1 -ToolchainRoot C:/path/to/toolchain/tools -Targets client
#>
[CmdletBinding()]
param(
    [string]$ToolchainRoot = $PSScriptRoot,
    [string]$PythonPath,
    [ValidateRange(1, 128)][int]$Jobs = 12,
    [string[]]$Targets,
    [switch]$TestHarness,
    [switch]$Install
)

$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path -Parent $PSScriptRoot
$engineRoot = Join-Path $repoRoot 'engine'
$resolvedTools = (Resolve-Path -LiteralPath $ToolchainRoot).Path
$ndkRoot = Join-Path $resolvedTools 'android-ndk-r10e'
$llvmRoot = Join-Path $resolvedTools 'llvm/bin'
$wrapperRoot = Join-Path $resolvedTools 'llvmw/bin'
if (-not $PythonPath) {
    $PythonPath = Join-Path $resolvedTools 'py/python.exe'
}
$requiredFiles = @(
    $PythonPath,
    (Join-Path $ndkRoot 'RELEASE.TXT'),
    (Join-Path $llvmRoot 'llvm-strip.exe'),
    (Join-Path $wrapperRoot 'aarch64-linux-android21-clang.exe'),
    (Join-Path $wrapperRoot 'aarch64-linux-android21-clang++.exe')
)
foreach ($requiredFile in $requiredFiles) {
    if (-not (Test-Path -LiteralPath $requiredFile -PathType Leaf)) {
        throw "Required tool is missing: $requiredFile. Pass -ToolchainRoot to the tools directory containing the NDK, LLVM wrappers, and Python."
    }
}

# Keep all source, configuration, and build output in this checkout. External
# toolchain files are only read, and the caller's process environment is restored.
$savedEnvironment = @{}
foreach ($name in @('ANDROID_NDK_HOME', 'LLVM_BIN', 'LLVM_W', 'PATH')) {
    $savedEnvironment[$name] = [Environment]::GetEnvironmentVariable($name, 'Process')
}
Push-Location -LiteralPath $engineRoot
try {
    $env:ANDROID_NDK_HOME = $ndkRoot
    $env:LLVM_BIN = $llvmRoot
    $env:LLVM_W = $wrapperRoot
    $env:PATH = "$llvmRoot;$env:PATH"
    $configureArguments = @('waf', 'configure', '-T', 'release', '--android=aarch64,host,21', '--togles', '--use-togl=1', '--use-sdl=1', '--disable-warns', '--build-games=portal', '--prefix=../install', '--check-c-compiler=clang', '--check-cxx-compiler=clang++')
    if ($TestHarness) { $configureArguments += '--fstop-test-harness' }
    & $PythonPath @configureArguments
    if ($LASTEXITCODE -ne 0) { throw "Android configuration failed (exit $LASTEXITCODE)." }

    $buildArguments = @('waf', 'build', "-j$Jobs")
    if ($Targets) { $buildArguments += "--targets=$($Targets -join ',')" }
    & $PythonPath @buildArguments
    if ($LASTEXITCODE -ne 0) { throw "Android build failed (exit $LASTEXITCODE)." }

    if ($Install) {
        $installArguments = @('waf', 'install')
        if ($Targets) { $installArguments += "--targets=$($Targets -join ',')" }
        & $PythonPath @installArguments
        if ($LASTEXITCODE -ne 0) { throw "Android staging failed (exit $LASTEXITCODE)." }
    }
} finally {
    Pop-Location
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}
