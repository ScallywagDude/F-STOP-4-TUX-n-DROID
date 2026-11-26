<#
.SYNOPSIS
Runs the staged Windows build under a minimal debugger: prints OutputDebugString text, and on an access
violation (or any second-chance exception) prints the faulting address and a symbolised scan of the stack,
using the .pdb files next to each module. No WinDbg needed.
.EXAMPLE
./tools/crashtrace.ps1 -GameArgs '+map lab_intro'
#>
[CmdletBinding()]
param([string]$GameArgs = '', [int]$Seconds = 60, [int]$Frames = 40, [switch]$Quiet, [string]$Exe = '', [string]$ExeArgs = '')
if ($Exe) {
    # any other program (-Exe path -ExeArgs '...'): run it from its own folder, symbols from there
    $exe = (Resolve-Path $Exe).Path
    $root = Split-Path -Parent $exe
    $cmd = "`"$exe`" $ExeArgs"
    $search = $root
} else {
    $root = Join-Path (Split-Path -Parent $PSScriptRoot) '.port-build\win-run'
    $exe = Join-Path $root 'F-STOP.exe'
    $cmd = "`"$exe`" -insecure -game fstop -novid -console -condebug -windowed -w 1280 -h 720 -noborder $GameArgs"
    $search = "$root;$root\bin\x64;$root\fstop\bin\x64"
}

Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Collections.Generic;
using System.Runtime.InteropServices;
public static class MiniDbg {
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)] struct STARTUPINFO { public int cb; public string r, d, t; public int x, y, w, h, cx, cy, fa, fl; public short sw, cb2; public IntPtr r2, i, o, e; }
    [StructLayout(LayoutKind.Sequential)] struct PROCINFO { public IntPtr hp, ht; public int pid, tid; }
    [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)] static extern bool CreateProcess(string app, StringBuilder cmd, IntPtr pa, IntPtr ta, bool inh, uint flags, IntPtr env, string dir, ref STARTUPINFO si, out PROCINFO pi);
    [DllImport("kernel32")] static extern bool WaitForDebugEvent(IntPtr ev, uint ms);
    [DllImport("kernel32")] static extern bool ContinueDebugEvent(int pid, int tid, uint status);
    [DllImport("kernel32")] static extern bool ReadProcessMemory(IntPtr hp, ulong addr, byte[] buf, int n, out IntPtr read);
    [DllImport("kernel32")] static extern IntPtr OpenThread(uint access, bool inh, int tid);
    [DllImport("kernel32")] static extern bool GetThreadContext(IntPtr ht, IntPtr ctx);
    [DllImport("kernel32")] static extern bool TerminateProcess(IntPtr hp, uint code);
    [DllImport("kernel32")] static extern bool CloseHandle(IntPtr h);
    [DllImport("dbghelp", CharSet = CharSet.Unicode)] static extern bool SymInitializeW(IntPtr hp, string path, bool invade);
    [DllImport("dbghelp")] static extern uint SymSetOptions(uint o);
    [DllImport("dbghelp", CharSet = CharSet.Unicode)] static extern ulong SymLoadModuleExW(IntPtr hp, IntPtr hf, string img, string mod, ulong bas, uint size, IntPtr data, uint flags);
    [DllImport("dbghelp", CharSet = CharSet.Unicode)] static extern bool SymFromAddrW(IntPtr hp, ulong addr, out ulong disp, IntPtr sym);
    [DllImport("dbghelp", CharSet = CharSet.Unicode)] static extern bool SymGetLineFromAddrW64(IntPtr hp, ulong addr, out uint disp, IntPtr line);
    [DllImport("kernel32", CharSet = CharSet.Unicode)] static extern uint GetFinalPathNameByHandleW(IntPtr h, StringBuilder sb, uint n, uint f);

    static IntPtr hp;
    static List<KeyValuePair<ulong, string>> mods = new List<KeyValuePair<ulong, string>>();
    static string ModOf(ulong a) { string best = null; ulong bb = 0; foreach (var m in mods) if (m.Key <= a && m.Key >= bb) { bb = m.Key; best = m.Value; } return best == null ? "?" : best + "+0x" + (a - bb).ToString("x"); }
    static string Sym(ulong a, out bool ok) {
        ok = false;
        IntPtr s = Marshal.AllocHGlobal(88 + 2 * 512); try {
            for (int i = 0; i < 88; i++) Marshal.WriteByte(s, i, 0);
            Marshal.WriteInt32(s, 0, 88); Marshal.WriteInt32(s, 80, 512);
            ulong d; if (!SymFromAddrW(hp, a, out d, s)) return ModOf(a);
            ok = true;
            string name = Marshal.PtrToStringUni(s + 84);
            string r = ModOf(a).Split('+')[0] + "!" + name + "+0x" + d.ToString("x");
            IntPtr ln = Marshal.AllocHGlobal(40); try {
                for (int i = 0; i < 40; i++) Marshal.WriteByte(ln, i, 0);
                Marshal.WriteInt32(ln, 0, 40); uint ld;
                if (SymGetLineFromAddrW64(hp, a, out ld, ln)) r += "  " + Marshal.PtrToStringUni(Marshal.ReadIntPtr(ln, 24)) + ":" + Marshal.ReadInt32(ln, 16);
            } finally { Marshal.FreeHGlobal(ln); }
            return r;
        } finally { Marshal.FreeHGlobal(s); }
    }
    static string PathOf(IntPtr hf) { var sb = new StringBuilder(1024); GetFinalPathNameByHandleW(hf, sb, 1024, 0); var p = sb.ToString(); return p.StartsWith(@"\\?\") ? p.Substring(4) : p; }

    public static void Run(string cmd, string dir, string search, int seconds, int frames, bool quiet) {
        var si = new STARTUPINFO(); si.cb = Marshal.SizeOf(si); PROCINFO pi;
        if (!CreateProcess(null, new StringBuilder(cmd), IntPtr.Zero, IntPtr.Zero, false, 2 /*DEBUG_ONLY_THIS_PROCESS*/, IntPtr.Zero, dir, ref si, out pi)) { Console.WriteLine("CreateProcess failed " + Marshal.GetLastWin32Error()); return; }
        hp = pi.hp;
        SymSetOptions(0x2 | 0x10 | 0x200 /*UNDNAME|LOAD_LINES|FAIL_CRITICAL_ERRORS*/);
        SymInitializeW(hp, search, false);
        IntPtr ev = Marshal.AllocHGlobal(256);
        IntPtr ctxRaw = Marshal.AllocHGlobal(1232 + 16); IntPtr ctx = new IntPtr((ctxRaw.ToInt64() + 15) & ~15L);
        var deadline = DateTime.Now.AddSeconds(seconds);
        bool done = false; int traces = 0;
        while (!done) {
            if (DateTime.Now > deadline) { Console.WriteLine("-- still running after " + seconds + " s, terminating"); TerminateProcess(hp, 1); deadline = DateTime.MaxValue; }
            if (!WaitForDebugEvent(ev, 500)) continue;
            int code = Marshal.ReadInt32(ev, 0), pid = Marshal.ReadInt32(ev, 4), tid = Marshal.ReadInt32(ev, 8);
            uint cont = 0x00010002; // DBG_CONTINUE
            IntPtr u = ev + 16;
            switch (code) {
            case 3: { IntPtr hf = Marshal.ReadIntPtr(u, 0); ulong b = (ulong)Marshal.ReadInt64(u, 24); string p = PathOf(hf); mods.Add(new KeyValuePair<ulong, string>(b, System.IO.Path.GetFileName(p))); SymLoadModuleExW(hp, hf, p, null, b, 0, IntPtr.Zero, 0); break; }
            case 6: { IntPtr hf = Marshal.ReadIntPtr(u, 0); ulong b = (ulong)Marshal.ReadInt64(u, 8); string p = PathOf(hf); mods.Add(new KeyValuePair<ulong, string>(b, System.IO.Path.GetFileName(p))); SymLoadModuleExW(hp, hf, p, null, b, 0, IntPtr.Zero, 0); if (!quiet) Console.WriteLine("load " + p); CloseHandle(hf); break; }
            case 8: {
                ulong a = (ulong)Marshal.ReadInt64(u, 0); bool uni = Marshal.ReadInt16(u, 8) != 0; int n = Marshal.ReadInt16(u, 10) & 0xffff;
                var buf = new byte[n]; IntPtr rd; ReadProcessMemory(hp, a, buf, n, out rd);
                Console.Write((uni ? Encoding.Unicode.GetString(buf) : Encoding.Default.GetString(buf)).TrimEnd('\0'));
                break; }
            case 1: {
                uint ec = (uint)Marshal.ReadInt32(u, 0); ulong addr = (ulong)Marshal.ReadInt64(u, 16); bool first = Marshal.ReadInt32(u, 152) != 0;
                if (ec == 0x80000003 || ec == 0x4000001F) break; // breakpoints: continue
                if (ec == 0x406D1388 || ec == 0x40010006 || ec == 0x4001000A || ec == 0xE06D7363 && first) { cont = 0x80010001; break; } // thread names, debug strings, C++ throws
                cont = 0x80010001; // DBG_EXCEPTION_NOT_HANDLED
                if (traces >= 3 || (first && ec != 0xC0000005 && ec != 0xC000001D && ec != 0xC0000409)) break;
                traces++;
                Console.WriteLine();
                Console.WriteLine("== exception 0x" + ec.ToString("X8") + (first ? " (first chance)" : " (second chance)") + " thread " + tid + " at " + ModOf(addr));
                if (ec == 0xC0000005) Console.WriteLine("   " + (Marshal.ReadInt64(u, 32) == 0 ? "reading" : Marshal.ReadInt64(u, 32) == 8 ? "executing" : "writing") + " 0x" + ((ulong)Marshal.ReadInt64(u, 40)).ToString("x"));
                IntPtr ht = OpenThread(0x1FFFFF, false, tid);
                for (int i = 0; i < 1232; i++) Marshal.WriteByte(ctx, i, 0);
                Marshal.WriteInt32(ctx, 0x30, 0x10000B); // CONTEXT_FULL (AMD64)
                if (GetThreadContext(ht, ctx)) {
                    ulong rip = (ulong)Marshal.ReadInt64(ctx, 0xF8), rsp = (ulong)Marshal.ReadInt64(ctx, 0x98);
                    bool ok; Console.WriteLine("   rip " + Sym(rip, out ok));
                    var stk = new byte[8 * 1024]; IntPtr rd; ReadProcessMemory(hp, rsp, stk, stk.Length, out rd);
                    int shown = 0;
                    for (int o = 0; o + 8 <= (int)rd && shown < frames; o += 8) {
                        ulong v = BitConverter.ToUInt64(stk, o);
                        if (v < 0x10000) continue;
                        string s = Sym(v, out ok); if (!ok) continue;
                        Console.WriteLine("   [rsp+0x" + o.ToString("x") + "] " + s); shown++;
                    }
                }
                CloseHandle(ht);
                if (!first) { TerminateProcess(hp, 1); }
                break; }
            case 5: Console.WriteLine("-- process exited with code 0x" + ((uint)Marshal.ReadInt32(u, 0)).ToString("X8")); done = true; break;
            }
            ContinueDebugEvent(pid, tid, cont);
        }
    }
}
'@

[MiniDbg]::Run($cmd, $root, $search, $Seconds, $Frames, $Quiet.IsPresent)
