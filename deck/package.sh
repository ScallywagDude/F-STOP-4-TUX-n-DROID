#!/usr/bin/env bash
# Assemble a Linux release from the selected build and the caller's game data.
# Usage: FSTOP_DATA_DIR=/path/to/fsdata package.sh [--tar]
set -euo pipefail
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${FSTOP_SOURCE_DIR:-$REPO/engine}"
BUILD="${FSTOP_BUILD_DIR:-$REPO/.port-build/linux-x86_64}"
INSTALL="${FSTOP_INSTALL_DIR:-$REPO/.port-build/linux-install}"
DATA="${FSTOP_DATA_DIR:-$REPO/fsdata}"
ARTIFACTS="${FSTOP_ARTIFACT_DIR:-$REPO/out/releases}"
VERSION="${FSTOP_VERSION:-1.1}"
[[ -d "$DATA/fstop" && -d "$DATA/platform" ]] || { printf 'Missing game data: %s\n' "$DATA" >&2; exit 1; }
[[ "${1:-}" == '' || "${1:-}" == --tar ]] || { printf 'Usage: package.sh [--tar]\n' >&2; exit 1; }
mkdir -p "$ARTIFACTS" "$REPO/.port-build"
cd "$SRC"
python3 waf install --out="$BUILD"

# Use a fresh private staging directory. Existing packages and source data are
# never deleted or changed while preparing a new release.
STAGE="$(mktemp -d "$REPO/.port-build/linux-package.XXXXXX")"
P="$STAGE/F-STOP"
mkdir -p "$P/bin" "$P/lib" "$P/fonts" "$P/fstop/bin"
cp -a "$INSTALL/hl2_launcher" "$P/"
cp -a "$INSTALL"/bin/*.so "$P/bin/"
cp -a "$DATA/fstop/." "$P/fstop/"
cp -a "$DATA/platform" "$P/"
cp -a "$INSTALL"/portal/bin/*.so "$P/fstop/bin/"
cp -a "$REPO/deck/data/fstop/." "$P/fstop/"
for overlay in loose_models loose_materials; do
    if [[ -d "$REPO/tools/$overlay" ]]; then cp -a "$REPO/tools/$overlay/." "$P/fstop/"; fi
 done
mkdir -p "$P/fstop/cfg"
touch "$P/fstop/cfg/autoexec.cfg"
grep -q '^joystick 1' "$P/fstop/cfg/autoexec.cfg" || printf '\njoystick 1\n' >> "$P/fstop/cfg/autoexec.cfg"
cp "$REPO"/launcher/assets/fonts/dejavusans*.ttf "$REPO"/launcher/assets/fonts/liberation*.ttf "$P/fonts/"
cp "$REPO/launcher/res/drawable-xxxhdpi/ic_launcher.png" "$P/icon.png"
cp "$REPO/deck/pkg/F-STOP" "$REPO/deck/pkg/Install F-STOP.sh" "$REPO/deck/pkg/README.txt" "$P/"
sed -i 's/\r$//' "$P/F-STOP" "$P/Install F-STOP.sh"
chmod +x "$P/F-STOP" "$P/Install F-STOP.sh" "$P/hl2_launcher"
find "$P/bin" "$P/fstop/bin" -type f -name '*.so' -exec strip --strip-unneeded {} +
strip --strip-unneeded "$P/hl2_launcher"

# Keep C/C++ runtime and graphics/window/audio stack supplied by the target
# Steam Runtime. Copy other dependencies from the same SDK as the game build.
SYSTEM_LIBS='^(linux-vdso|ld-linux|libc\.|libm\.|libdl\.|libpthread|librt\.|libresolv|libnsl|libutil|libstdc\+\+|libgcc_s|libGL|libEGL|libOpenGL|libX|libxcb|libdrm|libgbm|libwayland|libz\.|libexpat|libzstd|liblzma|libbz2|libelf|libmvec|libpcre|libdbus|libsystemd|libcap|libgcrypt|libgpg-error|libpulse|libasound|libvulkan)'
export LD_LIBRARY_PATH="$P/bin:$P/fstop/bin:$P/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
for pass in 1 2 3; do
    : > "$STAGE/dependencies.txt"
    while IFS= read -r -d '' binary; do
        ldd "$binary" 2>/dev/null | awk '/=>/ {print $1, $3}' >> "$STAGE/dependencies.txt" || true
    done < <(find "$P/bin" "$P/fstop/bin" "$P/lib" "$P/hl2_launcher" -type f -print0)
    while read -r name library; do
        [[ "$library" == "$P"/* || "$library" == not ]] && continue
        if [[ "$name" =~ $SYSTEM_LIBS ]]; then continue; fi
        cp -L "$library" "$P/lib/$name"
    done < <(sort -u "$STAGE/dependencies.txt")
    # Newer sniper SDKs provide SDL2 through sdl2-compat. SDL3 is a required
    # dlopen dependency and therefore does not appear in ldd's output.
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
    ARCHIVE="$ARTIFACTS/${FSTOP_ARCHIVE_NAME:-F-STOP-Linux-x86_64-$VERSION.tar.gz}"
    tar -C "$STAGE" -czf "$ARCHIVE.tmp" F-STOP
    mv "$ARCHIVE.tmp" "$ARCHIVE"
    (cd "$ARTIFACTS" && sha256sum "$(basename "$ARCHIVE")" > "$(basename "$ARCHIVE").sha256")
    printf 'Release archive: %s\n' "$ARCHIVE"
fi
