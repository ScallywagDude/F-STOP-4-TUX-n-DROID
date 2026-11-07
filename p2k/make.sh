#!/bin/bash
# Builds the whole Project 2008 patch folder from the port's data: p2k/fstop_fixes/{maps/,pak01_dir.vpk}
set -e
cd "$(cd "$(dirname "$0")/.." && pwd)"
rm -rf p2k/fstop_fixes.tmp; mkdir -p p2k/fstop_fixes.tmp
# the game may hold the old archive open, and the test junction points at this folder
rm -f p2k/fstop_fixes/pak01_dir.vpk
find p2k/fstop_fixes -mindepth 1 -maxdepth 1 -exec rm -rf {} +
node --max-old-space-size=6000 tools/p2k_build.js --maps | tail -3 | cut -c1-200
node tools/p2k_zombie.js | tail -1
node tools/p2k_radio.js
node tools/p2k_lift.js
node tools/p2k_glass.js
node tools/p2k_seamat.js p2k/fstop_fixes
node tools/p2k_names.js
node tools/p2k_hud.js
node tools/p2k_vpk.js p2k/fstop_fixes/ p2k/fstop_fixes.tmp/pak01_dir.vpk '^(maps/|pak01)'
find p2k/fstop_fixes -mindepth 1 -maxdepth 1 ! -name maps -exec rm -rf {} +
mv p2k/fstop_fixes.tmp/pak01_dir.vpk p2k/fstop_fixes/pak01_dir.vpk
rmdir p2k/fstop_fixes.tmp
ls p2k/fstop_fixes p2k/fstop_fixes/maps | wc -l
du -sh p2k/fstop_fixes | cut -f1
