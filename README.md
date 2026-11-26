# F-STOP

My port of F-STOP, Valve's shelved Portal 2 prototype. Since 1.2 it runs on the CS:GO engine (the March 2019 partner source) on Windows, Linux and Android, with full touch controls and native controller support, and it plays from start to finish without the portal gun.

## Credits
- **freon1996 (Discord):** Social Preview art
- **CS:GO partner source (March 2019):** the engine 1.2 is built on. See the end of this file.
- [**nillerusr/source-engine**](https://github.com/nillerusr/source-engine)**:** the base of the Android launcher and the OpenGL ES renderer (ToGLES), and the engine the 1.0 and 1.1 releases were built on.
- [**P2ASW**](https://github.com/EpicSentry/P2ASW)**:** the base for the menu system. P2ASW code may not be used for commercial purposes, and anyone redistributing it must credit P2ASW. See `CREDITS.md`.
- **Valve:** F-STOP, Portal 2, CS:GO and the Source engine are theirs.

## Builds

Builds are on the Releases page.

- **Windows:** x64. Extract `F-STOP-GO-Windows-x64-1.2.7z` with 7-Zip, then run `Play F-STOP.bat`.
- **Linux / Steam Deck:** x86-64 with native controller support. Extract `F-STOP-GO-Linux-x86_64-1.2.tar.xz` (`tar xf F-STOP-GO-Linux-x86_64-1.2.tar.xz`), then run `F-STOP`. On a Steam Deck, `Install F-STOP.sh` adds it to your Steam library (see `README.txt` in the folder).
- **Android:** arm64, OpenGL ES. Open `F-STOP-GO-Android-arm64-1.2.7z.001` with 7-Zip and install the APK inside.

No Steam is needed, and several copies can run at once.

## What I changed

- Moved the whole game to the CS:GO engine, with Windows, Linux and Android builds from one codebase.
- Recompiled every map with the CS:GO engine's own vbsp, vvis and vrad, with baked cubemaps.
- Ported the game to arm64 on OpenGL ES, with touch controls.
- Added a Steam Deck control scheme, including trackpad photo sizing and zoom.
- Fixed water reflections and refractions, moving platform lighting and cubemaps.
- Ported the Portal 2 style menus from P2ASW, using the UI only. Settings apply for real and save between launches.
- Removed the five hybrid maps, the portal gun and the HEV HUD elements. It caused quite a few issues and cutting a lot of it from the game just seemed like the easiest option in the moment.

## Building from source

First extract `src.7z.001` with 7-Zip into the repository folder. It holds the prebuilt libraries that are over GitHub's 100 MB file limit. The exact configure commands for each platform are in `BUILDING.md`.

## Project layout

| Folder | Contents |
| --- | --- |
| `src/` | The CS:GO engine and the F-STOP game code |
| `src/game/client/portal2/basemodui/` | The menu system |
| `launcher/` | The Android launcher |
| `deck/` | Linux and Steam Deck packaging |
| `tools/` | Data, map and packaging scripts |
| `tools/partner_ui/` | Menu and HUD files added to the game data |
| `mapsrc/` | Map sources |
| `p2k/` | Project 2008 patch scripts |

This repository does not include the game data. You need your own copy of the F-STOP files to build the game from source.

This is a non-commercial preservation project with no connection to Valve.

## CS:GO partner source

The engine is the March 2019 CS:GO partner source:

- 730 (11 March 2019, 21:50:44 UTC): 731 (Assets) `8269807393781859603`, 732 (Windows) `1439837285866999416`, 733 (macOS) `8771889840000773945`, 734 (Linux) `2907398298184722507`
- 740 (8 March 2019, 01:09:37 UTC): 740 (DS) `5632566164910295033`
- 745 (6 March 2019, 23:20:08 UTC): 745 (SDK) `822365065306396956`

Files originally acquired from https://archive.org/details/csgo_partner

## AI Disclosure
Used for placeholder art and CODE AUDITING, theres still a man in there. Lurking. Fixing.
