#!/usr/bin/env bash
# Build the partner source tree natively on Linux/Steam Deck or inside WSL.
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source_root="$repo_root/src"
mode="${1:-release}"
build_root="${FSTOP_BUILD_DIR:-$repo_root/.port-build/linux-$(uname -m)}"
install_root="${FSTOP_INSTALL_DIR:-$repo_root/install/linux-$(uname -m)}"
jobs="${JOBS:-$(nproc)}"
components="${FSTOP_COMPONENTS:-}"

case "$mode" in release|debug|profile) ;; *) echo "Unknown build mode: $mode" >&2; exit 2 ;; esac
mkdir -p "$build_root"
cd "$source_root"
configure_args=(configure --out="$build_root" -T "$mode" --build-games=portal
    --use-sdl=1 --use-togl=1 --disable-warns --prefix="$install_root")
if [[ -n "$components" ]]; then configure_args+=(--components="$components"); fi
python3 "$source_root/waf" "${configure_args[@]}"
build_args=(build --out="$build_root" -j"$jobs")
if [[ -n "${FSTOP_TARGETS:-}" ]]; then build_args+=(--targets="$FSTOP_TARGETS"); fi
python3 "$source_root/waf" "${build_args[@]}"
if [[ "${FSTOP_INSTALL:-0}" == 1 ]]; then
    install_args=(install --out="$build_root")
    if [[ -n "${FSTOP_TARGETS:-}" ]]; then install_args+=(--targets="$FSTOP_TARGETS"); fi
    python3 "$source_root/waf" "${install_args[@]}"
fi
