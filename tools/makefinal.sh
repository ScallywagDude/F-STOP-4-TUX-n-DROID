#!/bin/bash
# Builds out/final/F-STOP.apk: stripped libraries, all game data inside, release signed.
set -e
cd "$(cd "$(dirname "$0")/.." && pwd)"
FINAL=out/final; rm -rf $FINAL; mkdir -p $FINAL/libs $FINAL/assets/fstop_data
for f in fstop_libs/*.so; do cp "$f" $FINAL/libs/; done
tools/llvm/bin/llvm-strip.exe --strip-unneeded $FINAL/libs/*.so
echo "libs MB: $(du -sm $FINAL/libs | cut -f1)"
cp -r fsdata/fstop $FINAL/assets/fstop_data/fstop
cp -r fsdata/platform $FINAL/assets/fstop_data/platform
sed -i 's/^mat_queue_mode 0/mat_queue_mode -1/' $FINAL/assets/fstop_data/fstop/cfg/config_default.cfg
# size<TAB>path for every file, in the form MainActivity reads
(cd $FINAL/assets/fstop_data && find . -type f | sed 's#^\./##' | sort | while read -r p; do printf '%s\t%s\n' "$(wc -c < "$p")" "$p"; done) > $FINAL/assets/fstop_manifest.txt
echo "data files: $(wc -l < $FINAL/assets/fstop_manifest.txt), MB: $(du -sm $FINAL/assets | cut -f1)"
bash tools/build_apk.sh $(pwd -W)/$FINAL/libs $(pwd -W)/$FINAL/F-STOP.apk $(pwd -W)/$FINAL/assets
echo FINAL BUILD DONE
