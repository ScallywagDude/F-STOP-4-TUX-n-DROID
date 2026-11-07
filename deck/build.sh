#!/bin/bash
# Runs inside WSL (Steam Runtime sniper). Copies the engine source in and builds x86-64 Linux F-STOP.
# usage: build.sh
MODE=${1:-release}
SRC="$(cd "$(dirname "$0")/.." && pwd)/engine"
W=/root/engine
mkdir -p $W
cd $SRC
tar c --exclude=./build --exclude=./.git --exclude='*.o' --exclude=./dx9sdk --exclude=./hammer . | tar x -C $W --keep-newer-files 2>/dev/null
cd $W
# Source files keep CRLF from Windows, waf itself must not
sed -i 's/\r$//' waf 2>/dev/null; chmod +x waf
EXTRA=""
if [ ! -f /root/configured_$MODE ]; then
  CFLAGS="$EXTRA" CXXFLAGS="$EXTRA" python3 waf configure -T release --build-games=portal --disable-warns --prefix=/root/install_$MODE > /root/configure_$MODE.log 2>&1
  rc=$?; tail -5 /root/configure_$MODE.log
  [ $rc = 0 ] || { echo CONFIGURE FAILED; exit 1; }
  touch /root/configured_$MODE
fi
python3 waf build -j12 -k > /root/build_$MODE.log 2>&1
echo "build exit $?"
grep -E 'error:|fatal error|Build failed|finished successfully' /root/build_$MODE.log | sed 's/^.*\/root\/engine\///' | sort | uniq -c | sort -rn | head -40 | cut -c1-170
