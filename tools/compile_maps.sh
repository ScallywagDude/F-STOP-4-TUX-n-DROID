#!/bin/bash
# Compiles every F-STOP map from VMF with vbsp, vvis and vrad built from this tree (src/utils), HDR only.
# usage: compile_maps.sh [maps...]     default: every name in .port-build/mapbuild/maplist.txt
# Build the tools first (Waf components raytrace,fgdlib,utils/lzma,utils/vmpi,utils/vbsp,utils/vvis,
# utils/vvis_launcher,utils/vrad,utils/vrad_launcher) and stage them with their DLLs in .port-build/maptools/bin/x64.
# The VMFs are compiled in mapbuild/game/maps, next to the gameinfo that mounts the F-STOP data.
# Finished maps are skipped (done/<map>.ok). Create mapbuild/STOP to end after the current map.
export MSYS_NO_PATHCONV=1
ROOT="$(cd "$(dirname "$0")/.." && pwd -W)"
B="$ROOT/.port-build/mapbuild"
P2="${MAPTOOLS_BIN:-$ROOT/.port-build/maptools/bin/x64}"
[ -x "$P2/vbsp.exe" ] && [ -x "$P2/vvis.exe" ] && [ -x "$P2/vrad.exe" ] || { echo "map tools are not built: $P2"; exit 1; }
THREADS="${MAP_THREADS:-10}"
VIS_TIMEOUT="${VIS_TIMEOUT:-2700}"
RAD_TIMEOUT="${RAD_TIMEOUT:-5400}"
G="$B/game"; M="$G/maps"; L="$B/logs"; D="$B/done"
mkdir -p "$M" "$L" "$D"; rm -f "$B/STOP"
[ $# -gt 0 ] && MAPS="$*" || MAPS=$(tr -d '\r' < "$B/maplist.txt")
GW=$(cygpath -w "$G")
say() { echo "$(date +%T) $*" | tee -a "$B/run.txt"; }
for m in $MAPS; do
  [ -f "$B/STOP" ] && { say "STOPPED before $m"; break; }
  # Versioned source overrides include the release map fixes and new menu map.
  if [ -f "$ROOT/mapsrc/$m.vmf" ] && ! cmp -s "$ROOT/mapsrc/$m.vmf" "$M/$m.vmf"; then
    cp "$ROOT/mapsrc/$m.vmf" "$M/$m.vmf" || exit 1
    rm -f "$D/$m.ok"
  fi
  [ -f "$D/$m.ok" ] && continue
  [ -f "$M/$m.vmf" ] || cp "$B/src/scratch/$m.vmf" "$M/" || { say "NOSRC $m"; continue; }
  V=$(cygpath -w "$M/$m.vmf"); S=$(cygpath -w "$M/$m.bsp"); t0=$SECONDS; note=""
  rm -f "$M/$m.prt"
  "$P2/vbsp.exe" -game "$GW" "$V" > "$L/$m.vbsp.log" 2>&1 || { say "FAIL vbsp $m"; continue; }
  if grep -q '\*\*\*\* leaked \*\*\*\*' "$L/$m.vbsp.log"; then
    note="$note leaked(no-vis)"
  elif [ ! -f "$M/$m.prt" ]; then
    note="$note no-prt(no-vis)"
  else
    timeout "$VIS_TIMEOUT" "$P2/vvis.exe" -threads "$THREADS" -game "$GW" "$S" > "$L/$m.vvis.log" 2>&1
    rc=$?
    if [ $rc = 124 ]; then
      note="$note vis-fast"
      "$P2/vvis.exe" -fast -threads "$THREADS" -game "$GW" "$S" > "$L/$m.vvis.log" 2>&1 || { say "FAIL vvis-fast $m"; continue; }
    elif [ $rc != 0 ]; then say "FAIL vvis $m rc=$rc"; continue; fi
  fi
  timeout "$RAD_TIMEOUT" "$P2/vrad.exe" -hdr -final -StaticPropLighting -StaticPropPolys -TextureShadows -threads "$THREADS" -game "$GW" "$S" > "$L/$m.vrad.log" 2>&1
  rc=$?
  if [ $rc = 124 ]; then
    note="$note rad-nofinal"
    "$P2/vrad.exe" -hdr -StaticPropLighting -StaticPropPolys -TextureShadows -threads "$THREADS" -game "$GW" "$S" > "$L/$m.vrad.log" 2>&1 || { say "FAIL vrad-nofinal $m"; continue; }
  elif [ $rc != 0 ]; then say "FAIL vrad $m rc=$rc"; continue; fi
  sz=$(stat -c %s "$M/$m.bsp")
  echo "$sz$note" > "$D/$m.ok"
  say "ok $m ${sz}B $((SECONDS - t0))s$note"
done
say "ALL DONE"
