<#
.SYNOPSIS
Starts the staged Windows build, waits, then reports what it shows: every top-level window of the process
with the text of its child controls (so error dialogs can be read), whether it is still alive, and the tail
of fstop/console.log. The process is closed afterwards unless -Keep is given.
.EXAMPLE
./tools/run_win.ps1 -Seconds 30 -GameArgs '+map lab_intro'
#>
[CmdletBinding()]
param(
    [int]$Seconds = 25,
    [string]$GameArgs = '',
    [int]$LogLines = 60,
    [switch]$Keep
)
$ErrorActionPreference = 'Stop'
$root = Join-Path (Split-Path -Parent $PSScriptRoot) '.port-build\win-run'
$exe = Join-Path $root 'F-STOP.exe'
$log = Join-Path $root 'fstop\console.log'
if (Test-Path $log) { Remove-Item $log -Force }

Add-Type @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class WinText {
    public delegate bool EnumProc(IntPtr hWnd, IntPtr lParam);
    [DllImport("user32.dll")] static extern bool EnumWindows(EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] static extern bool EnumChildWindows(IntPtr parent, EnumProc cb, IntPtr lParam);
    [DllImport("user32.dll")] static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint pid);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetWindowText(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll", CharSet = CharSet.Unicode)] static extern int GetClassName(IntPtr hWnd, StringBuilder sb, int max);
    [DllImport("user32.dll")] static extern bool IsWindowVisible(IntPtr hWnd);
    static string Text(IntPtr h) { var sb = new StringBuilder(4096); GetWindowText(h, sb, sb.Capacity); return sb.ToString(); }
    static string Class(IntPtr h) { var sb = new StringBuilder(256); GetClassName(h, sb, sb.Capacity); return sb.ToString(); }
    public static List<string> Describe(uint pid) {
        var lines = new List<string>();
        EnumWindows((h, l) => {
            uint p; GetWindowThreadProcessId(h, out p);
            if (p != pid || !IsWindowVisible(h)) return true;
            lines.Add("window [" + Class(h) + "] " + Text(h));
            EnumChildWindows(h, (c, l2) => { var t = Text(c); if (t.Length > 0) lines.Add("    [" + Class(c) + "] " + t); return true; }, IntPtr.Zero);
            return true;
        }, IntPtr.Zero);
        return lines;
    }
}
'@

$argList = "-insecure -game fstop -novid -console -condebug -windowed -w 1280 -h 720 -noborder $GameArgs"
$proc = Start-Process -FilePath $exe -ArgumentList $argList -WorkingDirectory $root -PassThru
$deadline = (Get-Date).AddSeconds($Seconds)
while ((Get-Date) -lt $deadline -and -not $proc.HasExited) { Start-Sleep -Milliseconds 500 }

if ($proc.HasExited) {
    "process exited with code $($proc.ExitCode) (0x{0:X8})" -f $proc.ExitCode
} else {
    "process alive after $Seconds s, working set $([int]($proc.WorkingSet64 / 1MB)) MB"
    [WinText]::Describe([uint32]$proc.Id) | ForEach-Object { $_ }
    if (-not $Keep) { Stop-Process -Id $proc.Id -Force }
}
if (Test-Path $log) {
    "---- console.log (last $LogLines lines)"
    Get-Content $log -Tail $LogLines
} else {
    '---- no console.log was written'
}
Get-ChildItem $root -Filter '*.mdmp' -Recurse -ErrorAction SilentlyContinue | ForEach-Object { "minidump: $($_.FullName)" }
