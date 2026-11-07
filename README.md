# F-STOP

My port of F-STOP, Valve's shelved Portal 2 prototype, to Android and Linux. It runs on the Source engine with full touch controls, and it plays from start to finish without the portal gun.

## Builds

Builds are on the Releases page.

- **Android:** arm64. I play it on a OnePlus 13 (Adreno 830). Other Adreno phones should work. Tensor phones may work too, though it is very hit or miss. Exynos has not been tested.
- **Linux / Steam Deck:** x86-64 with native controller support.

## What I changed

- Ported the game to arm64 on OpenGL ES, with touch controls.
- Added a Steam Deck control scheme, including trackpad photo sizing and zoom.
- Fixed water reflections and refractions, moving platform lighting and cubemaps, and recompiled the maps with new cubemaps.
- Added working 4x MSAA on Android and fixed multicore rendering.
- Ported the Portal 2 style menus from P2ASW, using the UI only. Settings apply for real and save between launches.
- Removed the five hybrid maps, the portal gun and the HEV HUD elements.

## Project layout

| Folder | Contents |
| --- | --- |
| `engine/` | The Source engine and game code |
| `engine/game/client/portal2/basemodui/` | The menu system |
| `launcher/` | The Android launcher |
| `deck/` | Linux and Steam Deck build scripts |
| `tools/` | Data and packaging scripts |
| `p2k/` | Project 2008 patch scripts |

This repository does not include the game data. You need your own copy of the F-STOP files to build the game from source.

## Credits

- [**nillerusr/source-engine**](https://github.com/nillerusr/source-engine)**:** the arm64 engine this is built on.
- [**P2ASW**](https://github.com/EpicSentry/P2ASW)**:** the base for the menu system. P2ASW code may not be used for commercial purposes, and anyone redistributing it must credit P2ASW. See `CREDITS.md`.
- **Valve:** F-STOP, Portal 2 and the Source engine are theirs.

This is a non-commercial preservation project with no connection to Valve.
