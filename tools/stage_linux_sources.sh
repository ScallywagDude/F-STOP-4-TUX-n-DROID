#!/usr/bin/env bash
# Make an independent, case-sensitive source snapshot without repository metadata
# or generated/prebuilt binaries. Use a new destination for each validation run.
set -euo pipefail
repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
stage_root="${1:?Usage: stage_linux_sources.sh /absolute/new/source/directory}"
if [[ "$stage_root" != /* || -e "$stage_root" ]]; then
    printf 'The staging destination must be a new absolute directory: %s\n' "$stage_root" >&2
    exit 1
fi
mkdir -p "$stage_root"
tar -C "$repo_root" \
    --exclude='.git' --exclude='.port-build' --exclude='install' \
    --exclude='src/build' --exclude='src/.waf*' --exclude='src/.lock-waf*' \
    --exclude='src/lib' --exclude='__pycache__' --exclude='*.pyc' \
    --exclude='*.o' --exclude='*.obj' --exclude='*.a' --exclude='*.so' \
    --exclude='*.dll' --exclude='*.exe' --exclude='*.lib' --exclude='*.pdb' \
    --exclude='*.dylib' --exclude='*.apk' --exclude='*.log' \
    -cf - src tools tests | tar -C "$stage_root" -xf -
printf 'Staged source snapshot: %s\n' "$stage_root"
