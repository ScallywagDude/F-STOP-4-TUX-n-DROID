# Android ARM64 native build on Windows

Run the PowerShell helper from the checkout:

```powershell
./tools/build_android.ps1 -ToolchainRoot C:/path/to/existing/tools -Install
```

`ToolchainRoot` is the directory containing `android-ndk-r10e/`,
`llvm/bin/`, `llvmw/bin/`, and `py/python.exe`. It defaults to this
checkout's `tools/` directory. An existing toolchain can live outside the
checkout; the helper only reads it. Pass `-PythonPath` to use a different
Python 3 interpreter. The host Clang wrappers must be named
`aarch64-linux-android21-clang.exe` and
`aarch64-linux-android21-clang++.exe`, matching the existing Waf
cross-compiler configuration.

The configuration builds the Portal/F-STOP game, SDL2 and ToGLES renderer
for Android ARM64/API 21 in release mode. The launcher requires Android
API 24 or later. Native dependency archives and SDL2 are under
`engine/lib/android/aarch64/`; their headers are under `engine/thirdparty/`.
The NDK, host LLVM compiler, SDK and Python are external build tools.

Compilation output stays under `engine/build/`. `-Install` stages native
libraries under `install/` in this checkout; it does not install anything
on an Android device. Use `-Jobs 8` to change build parallelism or
`-Targets client` to rebuild just the client and its dependencies.

```powershell
./tools/build_android.ps1 -ToolchainRoot C:/path/to/existing/tools -Targets client
```

This command builds the existing F-STOP engine. A successful build does
not validate the separate CSGO partner migration or prove rendering is
correct. APK packaging additionally needs an Android SDK, JDK, signing
key, and the user's F-STOP game data. See `build_apk.sh` for the existing
packaging process.

To package on Windows without Git Bash, use the matching PowerShell helper:

```powershell
./tools/build_apk.ps1 -ToolchainRoot C:/path/to/existing/tools -KeyStorePath C:/path/to/fstop.keystore
```

It uses `JAVA_HOME` (or `-JavaHome`), the staged `install/lib/arm64-v8a/*.so` libraries
(or `-LibrariesPath`), and the SDK layout used by the existing shell script.
The supplied keystore is read without modification. Password parameters
default to the existing personal-build values; pass `-KeyStorePassword`
and `-KeyPassword` for a different key. `-Debuggable` permits the launcher's
existing `fstop_args.txt` override for local tests. It is omitted by default.
`-AssetRoot` optionally bundles a prepared asset directory beneath APK
`assets/`. An APK without bundled game data requires data already installed
on the device. The helper writes the signed APK to `out/` and verifies its
signature, but does not install it on a device.

For local device testing only, build with `-TestHarness`, package with
`-Debuggable`, and add `-fstopcmd` to `fstop_args.txt`. All three switches
are required for the Android command-file hook. The debug launcher selects
the app-private `files/fstop_cmd.txt` path; it can be updated using `adb
shell run-as com.fstop.game`. The hook consumes the file before executing
each changed command value, preventing replay after an app restart. There
is no shared-storage fallback or network listener. Ordinary builds omit
the native hook entirely. Remove `-TestHarness` when building for release.
