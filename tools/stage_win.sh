#!/bin/bash
# Assembles a runnable Windows x64 folder from the Waf build output and the F-STOP data.
# usage: stage_win.sh [--data]      --data also (re)copies the loose game files; binaries are always refreshed
#   FSTOP_DATA  folder holding fstop/ and platform/ (default: the 1.1 release assets in the sibling checkout)
#   layout:  .port-build/win-run/F-STOP.exe, bin/x64/*.dll, fstop/bin/x64/{client,server,matchmaking}.dll,
#            fstop/gameinfo.txt, fstop/pak01_*.vpk (hard links), fstop/{cfg,materials,models,resource,scripts,maps}
export MSYS_NO_PATHCONV=1
ROOT="$(cd "$(dirname "$0")/.." && pwd -W)"
B="$ROOT/.port-build/win-x64"
R="$ROOT/.port-build/win-run"
DATA="${FSTOP_DATA:-$ROOT/../F-STOP-4-TUX-n-DROID/out/release-assets/fstop_data}"
mkdir -p "$R/bin/x64" "$R/fstop/bin/x64" "$R/platform"

# engine binaries; game modules go under the game folder
GAME_DLLS="client.dll server.dll matchmaking.dll"
find "$B" -name '*.dll' | while read f; do
  n=$(basename "$f")
  case "$n" in vvis_dll.*|vrad_dll.*|dx_proxy.*|shadercompile_dll.*) continue;; esac
  d="$R/bin/x64"
  for g in $GAME_DLLS; do [ "$n" = "$g" ] && d="$R/fstop/bin/x64"; done
  cp -u "$f" "$d/"
  [ -f "${f%.dll}.pdb" ] && cp -u "${f%.dll}.pdb" "$d/"
done
cp -u "$ROOT/src/lib/win32/amd64/SDL2.dll" "$R/bin/x64/" 2>/dev/null
cp -u "$B/launcher_main/launcher_main.exe" "$R/F-STOP.exe"
[ -f "$B/launcher_main/launcher_main.pdb" ] && cp -u "$B/launcher_main/launcher_main.pdb" "$R/F-STOP.pdb"

# packed data: hard links under the name the partner filesystem mounts by itself
for f in "$DATA"/fstop/fstop_pak_*.vpk; do
  n=$(basename "$f" | sed 's/fstop_pak/pak01/')
  [ -e "$R/fstop/$n" ] || cmd /c "mklink /H $(cygpath -w "$R/fstop/$n") $(cygpath -w "$f")" > /dev/null
done

if [ "$1" = "--data" ] || [ ! -d "$R/fstop/cfg" ]; then
  for d in cfg materials models resource scripts; do
    [ -d "$DATA/fstop/$d" ] && cp -ru "$DATA/fstop/$d" "$R/fstop/"
  done
  cp -ru "$DATA/platform/." "$R/platform/"
fi

# Refresh loose 1.1 fixes as well as the port's UI and material changes.
for d in materials resource scripts; do
  [ -d "$DATA/fstop/$d" ] && cp -ru "$DATA/fstop/$d" "$R/fstop/"
done
cp -r "$ROOT/tools/partner_ui/." "$R/fstop/"

# Content the original game got from its HL2/TF2/Portal mounts, copied out of those installs (only files the
# F-STOP data lacks) so the game is self-contained; see the resolver output in .port-build/mount_content_*.txt
if [ -d "$ROOT/.port-build/mount_content" ]; then
  cp -ru "$ROOT/.port-build/mount_content/." "$R/fstop/"
fi

# Locally compiled shader bytecode accompanies its loose camera material.
if [ -d "$ROOT/.port-build/shaders/fxc" ]; then
  mkdir -p "$R/platform/shaders/fxc"
  cp -u "$ROOT/.port-build/shaders/fxc/"*.vcs "$R/platform/shaders/fxc/"
fi

# maps: the ones compiled with this tree's vbsp/vvis/vrad
mkdir -p "$R/fstop/maps"
for ok in "$ROOT"/.port-build/mapbuild/done/*.ok; do
  [ -f "$ok" ] || continue
  m=$(basename "$ok" .ok)
  # old_portalgun_demo is a leftover test map that stays out of the release
  [ "$m" = old_portalgun_demo ] && continue
  cp -u "$ROOT/.port-build/mapbuild/game/maps/$m.bsp" "$R/fstop/maps/"
done

cat > "$R/fstop/gameinfo.txt" <<'EOF'
"GameInfo"
{
	game	"F-STOP"
	title	""
	type	singleplayer_only
	nodifficulty	1
	hasportals	1
	FileSystem
	{
		SteamAppId	400
		SearchPaths
		{
			Game	|gameinfo_path|.
			Game	platform
		}
	}
}
EOF
echo 400 > "$R/steam_appid.txt"
echo "staged: $(ls "$R/bin/x64" | grep -c '\.dll$') engine dlls, $(ls "$R/fstop/bin/x64" 2>/dev/null | grep -c '\.dll$') game dlls, $(ls "$R/fstop/maps" | grep -c '\.bsp$') maps"
