#!/usr/bin/env bash
# Run inside Linux or the Steam Runtime sniper SDK.
# Usage: build.sh [release|debug] [additional waf configure options]
set -euo pipefail
MODE=${1:-release}
if (( $# )); then shift; fi
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC="${FSTOP_SOURCE_DIR:-$REPO/engine}"
BUILD="${FSTOP_BUILD_DIR:-$REPO/.port-build/linux-x86_64}"
INSTALL="${FSTOP_INSTALL_DIR:-$REPO/.port-build/linux-install}"
LOGS="${FSTOP_LOG_DIR:-$REPO/.port-build/linux-logs}"
mkdir -p "$BUILD" "$INSTALL" "$LOGS"
cd "$SRC"
# Always let waf check the current configuration and dependencies. A separate
# output directory keeps Linux and Android builds independent.
python3 waf configure --out="$BUILD" -T "$MODE" --build-games=portal \
    --use-sdl=1 --use-togl=1 --disable-warns --prefix="$INSTALL" "$@" \
    2>&1 | tee "$LOGS/configure.log"
python3 waf build --out="$BUILD" -j"${FSTOP_JOBS:-12}" -k \
    2>&1 | tee "$LOGS/build.log"
printf 'Build complete: %s\nInstall prefix: %s\n' "$BUILD" "$INSTALL"
