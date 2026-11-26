#!/usr/bin/env bash
# Assemble the Linux x86_64 release from the installed build and the staged game data.
# Runs inside the Steam Runtime sniper SDK (the same one the game was built in), so bundled libraries match.
# Usage: FSTOP_DATA_DIR=/path/to/F-STOP-GO package.sh [--tar]
#   FSTOP_DATA_DIR    a staged Windows folder (tools/stage_desktop.ps1): its fstop/ and platform/ are used as they are
#   FSTOP_INSTALL_DIR waf install prefix (default .port-build/linux-install)
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
INSTALL="${FSTOP_INSTALL_DIR:-$REPO/.port-build/linux-install}"
DATA="${FSTOP_DATA_DIR:?set FSTOP_DATA_DIR to a staged F-STOP-GO folder}"
ARTIFACTS="${FSTOP_ARTIFACT_DIR:-$REPO/out/releases}"
VERSION="${FSTOP_VERSION:-1.2}"
[[ -f "$DATA/fstop/gameinfo.txt" && -d "$DATA/platform" ]] || { printf 'Missing game data: %s\n' "$DATA" >&2; exit 1; }
[[ "${1:-}" == '' || "${1:-}" == --tar ]] || { printf 'Usage: package.sh [--tar]\n' >&2; exit 1; }
[[ -x "$INSTALL/launcher_main" ]] || { printf 'No installed build in %s\n' "$INSTALL" >&2; exit 1; }
mkdir -p "$ARTIFACTS" "$REPO/.port-build"

# A fresh private staging directory; existing packages and the source data are never changed.
STAGE="$(mktemp -d "$REPO/.port-build/linux-package.XXXXXX")"
P="$STAGE/F-STOP-GO"
mkdir -p "$P/bin/linux64" "$P/fstop/bin/linux64" "$P/lib" "$P/fonts"

# Game data as the Windows build ships it, without its Windows binaries and without anything a run of the game or
# a test leaves behind (the same files tools/stage_desktop.ps1 keeps out, plus per-machine settings).
cp -a "$DATA/platform" "$P/"
(cd "$DATA/fstop" && find . \( -path ./bin -o -path ./SAVE -o -path ./screenshots -o -path ./testscripts \) -prune -o \
    \( -name '*.log' -o -name '*.tmp' -o -name '*.mdmp' -o -name '*.dmp' -o -name modelsounds.cache \
       -o -name fstop_gamestats.dat -o -name voice_ban.dt -o -name stats.txt -o -name glshaders.cfg -o -name README.md -o -name old_portalgun_demo.bsp \
       -o -path ./cfg/config.cfg -o -path ./cfg/video.txt -o -path ./cfg/menu_lifecycle.cfg \) -prune -o -print0 |
    cpio -0pdm --quiet "$P/fstop")

# Engine modules in bin/linux64 (launcher_main loads bin/linux64/launcher.so), game modules in fstop/bin/linux64.
GAME_MODULES='client.so server.so matchmaking.so'
for f in "$INSTALL"/bin/*.so; do
    n="$(basename "$f")"
    case " $GAME_MODULES " in *" $n "*) cp -a "$f" "$P/fstop/bin/linux64/" ;; *) cp -a "$f" "$P/bin/linux64/" ;; esac
done
for n in $GAME_MODULES launcher.so engine.so; do
    [[ -f "$P/fstop/bin/linux64/$n" || -f "$P/bin/linux64/$n" ]] || { printf 'Missing module: %s\n' "$n" >&2; exit 1; }
done
cp -a "$INSTALL/launcher_main" "$P/fstop_linux64"

cp "$REPO"/launcher/assets/fonts/dejavusans*.ttf "$REPO"/launcher/assets/fonts/liberation*.ttf "$P/fonts/"
cp "$REPO/launcher/res/drawable-xxxhdpi/ic_launcher.png" "$P/icon.png"
cp "$REPO/deck/pkg/F-STOP" "$REPO/deck/pkg/Install F-STOP.sh" "$REPO/deck/pkg/README.txt" "$P/"
sed -i 's/\r$//' "$P/F-STOP" "$P/Install F-STOP.sh" "$P/README.txt"
chmod +x "$P/F-STOP" "$P/Install F-STOP.sh" "$P/fstop_linux64"
find "$P/bin" "$P/fstop/bin" -type f -name '*.so' -exec strip --strip-unneeded {} +
strip --strip-unneeded "$P/fstop_linux64"

# Leave the C/C++ runtime and the graphics/window/audio stack to the system (Steam Runtime);
# copy every other dependency from the SDK the game was built in.
SYSTEM_LIBS='^(linux-vdso|ld-linux|libc\.|libm\.|libdl\.|libpthread|librt\.|libresolv|libnsl|libutil|libstdc\+\+|libgcc_s|libGL|libEGL|libOpenGL|libX|libxcb|libdrm|libgbm|libwayland|libz\.|libexpat|libzstd|liblzma|libbz2|libelf|libmvec|libpcre|libdbus|libsystemd|libcap|libgcrypt|libgpg-error|libpulse|libasound|libvulkan)'
export LD_LIBRARY_PATH="$P/bin/linux64:$P/fstop/bin/linux64:$P/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
for pass in 1 2 3; do
    : > "$STAGE/dependencies.txt"
    while IFS= read -r -d '' binary; do
        ldd "$binary" 2>/dev/null | awk '/=>/ {print $1, $3}' >> "$STAGE/dependencies.txt" || true
    done < <(find "$P/bin" "$P/fstop/bin" "$P/lib" "$P/fstop_linux64" -type f -print0)
    while read -r name library; do
        [[ "$library" == "$P"/* || "$library" == not ]] && continue
        if [[ "$name" =~ $SYSTEM_LIBS ]]; then continue; fi
        cp -L "$library" "$P/lib/$name"
    done < <(sort -u "$STAGE/dependencies.txt")
    # The sniper SDK's SDL2 is sdl2-compat, which dlopens libSDL3.so.0 (not visible to ldd).
    if [[ -f "$P/lib/libSDL2-2.0.so.0" ]] && grep -aq 'libSDL3.so.0' "$P/lib/libSDL2-2.0.so.0"; then
        SDL3="$(ldconfig -p | awk '/libSDL3.so.0.*x86-64/ {print $NF}')"
        [[ -n "$SDL3" && -r "$SDL3" ]] || { printf 'SDL2-compat requires libSDL3.so.0\n' >&2; exit 1; }
        cp -L "$SDL3" "$P/lib/libSDL3.so.0"
    fi
done
if grep -q ' not$' "$STAGE/dependencies.txt"; then
    printf 'Unresolved release dependencies:\n' >&2
    grep ' not$' "$STAGE/dependencies.txt" >&2
    exit 1
fi
printf 'Package directory: %s\n' "$P"
du -sh "$P"
if [[ "${1:-}" == --tar ]]; then
    # xz keeps the 1.2 package (about 4 GB) under GitHub's 2 GiB release-file limit; gzip does not.
    ARCHIVE="$ARTIFACTS/${FSTOP_ARCHIVE_NAME:-F-STOP-GO-Linux-x86_64-$VERSION.tar.xz}"
    tar -C "$STAGE" -cf - F-STOP-GO | xz -T0 -7 > "$ARCHIVE.tmp"
    xz -t "$ARCHIVE.tmp"
    mv "$ARCHIVE.tmp" "$ARCHIVE"
    (cd "$ARTIFACTS" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256")
    printf 'Release archive: %s\n' "$ARCHIVE"
fi
