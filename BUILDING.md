# Building F-STOP 1.2

Extract `src.7z.001` with 7-Zip into the repository folder first. Every build runs Waf from `src/` with Python 3, and you need your own copy of the F-STOP game data (`fstop/` and `platform/`) to stage a playable folder.

## Windows x64

Visual Studio 2022 (MSVC x64). From `src/`:

```
python waf configure --out=../.port-build/win-x64 -T release --disable-warns --build-games=portal --prefix=../.port-build/win-x64-install --components=tier0,tier1,tier2,tier3,vstdlib,mathlib,interfaces,bitmap,vtf,vpklib,filesystem,vphysics,ivp/havana,ivp/havana/havok/hk_base,ivp/havana/havok/hk_math,ivp/ivp_compact_builder,ivp/ivp_physics,appframework,materialsystem,materialsystem/shaderapiempty,materialsystem/shaderlib,materialsystem/stdshaders,raytrace,fgdlib,utils/lzma,utils/vmpi,utils/vbsp,utils/vvis,utils/vvis_launcher,utils/vrad,utils/vrad_launcher,videocfg,bonesetup,choreoobjects,datamodel,dmxloader,particles,responserules/runtime,thirdparty/quickhull,matchmaking,soundsystem/lowlevel,vgui2/vgui_controls,vgui2/vgui_surfacelib,vgui2/matsys_controls,datacache,studiorender,inputsystem,vgui2/src,vguimatsurface,scenefilecache,soundemittersystem,localize,vscript,stub_steam,thirdparty/protobuf-2.5.0,materialsystem/shaderapidx9,launcher,launcher_main,engine,utils/bzip2,avi,matchmaking/cstrike15,utils/dx_proxy,utils/shadercompile,utils/shadercompile_launcher,mathlib/extended,kv3lib,resourcefile,game/client,game/server,engine/voice_codecs/minimp3,utils/vtex,utils/bsppack
python waf build --out=../.port-build/win-x64
```

This also builds the map tools (vbsp, vvis, vrad) the maps are compiled with. Then `FSTOP_DATA=<folder with fstop and platform> tools/stage_win.sh --data` assembles a runnable folder in `.port-build/win-run`, and `tools/stage_desktop.ps1 -Destination <folder>` makes a clean copy with `Play F-STOP.bat`.

## Linux x86-64

Build inside the Steam Runtime 3.0 (sniper) SDK so the bundled libraries match what Steam runs the game with. From `src/`:

```
CC=gcc CXX=g++ python3 waf configure --out=../.port-build/linux-x86_64 -T release --build-games=portal --use-sdl=1 --use-togl=1 --disable-warns --components=external/crypto++-5.61,tier0,tier1,tier2,tier3,vstdlib,mathlib,interfaces,bitmap,vtf,vpklib,filesystem,vphysics,ivp/havana,ivp/havana/havok/hk_base,ivp/havana/havok/hk_math,ivp/ivp_compact_builder,ivp/ivp_physics,appframework,togl,materialsystem,materialsystem/shaderlib,materialsystem/stdshaders,materialsystem/shaderapidx9,videocfg,raytrace,bonesetup,choreoobjects,datamodel,dmxloader,particles,responserules/runtime,thirdparty/quickhull,matchmaking,soundsystem/lowlevel,vgui2/vgui_controls,vgui2/vgui_surfacelib,vgui2/matsys_controls,datacache,studiorender,inputsystem,vgui2/src,vguimatsurface,scenefilecache,soundemittersystem,localize,vscript,stub_steam,thirdparty/protobuf-2.5.0,launcher,launcher_main,engine,engine/voice_codecs/minimp3,utils/bzip2,avi,matchmaking/cstrike15,mathlib/extended,kv3lib,resourcefile,game/client,game/server --prefix=../.port-build/linux-install
python3 waf build --out=../.port-build/linux-x86_64
python3 waf install --out=../.port-build/linux-x86_64
```

Then `FSTOP_DATA_DIR=<staged Windows folder> deck/package.sh --tar` makes the release folder and archive.

## Android arm64

Needs Android NDK r10e, a host Clang/LLVM with `aarch64-linux-android21-clang` wrappers (see `tools/BUILD_ANDROID.md`), an Android SDK and a JDK. From `src/`:

```
python waf configure -T release --android=aarch64,host,21 --togles --use-togl=1 --use-sdl=1 --disable-warns --build-games=portal --prefix=../install --check-c-compiler=clang --check-cxx-compiler=clang++ --components=external/crypto++-5.61,tier0,tier1,tier2,tier3,vstdlib,mathlib,interfaces,bitmap,vtf,vpklib,filesystem,vphysics,ivp/havana,ivp/havana/havok/hk_base,ivp/havana/havok/hk_math,ivp/ivp_compact_builder,ivp/ivp_physics,appframework,togles,materialsystem,materialsystem/shaderlib,materialsystem/stdshaders,materialsystem/shaderapidx9,videocfg,raytrace,bonesetup,choreoobjects,datamodel,dmxloader,particles,responserules/runtime,thirdparty/quickhull,matchmaking,soundsystem/lowlevel,vgui2/vgui_controls,vgui2/vgui_surfacelib,vgui2/matsys_controls,datacache,studiorender,inputsystem,vgui2/src,vguimatsurface,scenefilecache,soundemittersystem,localize,vscript,stub_steam,thirdparty/protobuf-2.5.0,launcher,launcher_main,engine,engine/voice_codecs/minimp3,utils/bzip2,avi,matchmaking/cstrike15,mathlib/extended,kv3lib,resourcefile,game/client,game/server
python waf build
python waf install
```

`tools/stage_android_assets.py` turns a staged Windows folder into the APK's asset set, and, from the repository folder, `tools/build_apk.ps1 -AssetRoot out/android-assets -KeyStorePath <your keystore>` packages the installed libraries and signs the APK.
