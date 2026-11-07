#!/bin/bash
# Inside WSL: assembles the release build, data and libraries into /root/pkg/F-STOP. usage: package.sh [--tar]
set -e
FSB="$(cd "$(dirname "$0")/.." && pwd)"
I=/root/install_release
P=/root/pkg/F-STOP

cd /root/engine
python3 waf install > /root/install_release.log 2>&1 || { tail -5 /root/install_release.log; exit 1; }

rm -rf /root/pkg; mkdir -p "$P/bin" "$P/lib" "$P/fonts" "$P/fstop/bin"
cp -a "$I/hl2_launcher" "$P/"
cp -a "$I"/bin/*.so "$P/bin/"
cp -a "$I"/portal/bin/*.so "$P/fstop/bin/"
cp -r $FSB/fsdata/fstop/. "$P/fstop/"
cp -r $FSB/fsdata/platform "$P/"
cp -r $FSB/deck/data/fstop/. "$P/fstop/"
grep -q '^joystick 1' "$P/fstop/cfg/autoexec.cfg" || printf 'joystick 1\n' >> "$P/fstop/cfg/autoexec.cfg"

cp $FSB/launcher/assets/fonts/dejavusans*.ttf $FSB/launcher/assets/fonts/liberation*.ttf "$P/fonts/"
cp $FSB/launcher/res/drawable-xxxhdpi/ic_launcher.png "$P/icon.png"
cp $FSB/deck/pkg/F-STOP "$FSB/deck/pkg/Install F-STOP.sh" $FSB/deck/pkg/README.txt "$P/"
sed -i 's/\r$//' "$P/F-STOP" "$P/Install F-STOP.sh"
chmod +x "$P/F-STOP" "$P/Install F-STOP.sh" "$P/hl2_launcher"

find "$P/bin" "$P/fstop/bin" "$P/hl2_launcher" -type f -exec strip --strip-unneeded {} + 2>/dev/null || true

# Libraries: everything the game links that SteamOS can't be relied on to carry at this exact version.
# Left to the system: its C and C++ runtimes, the graphics stack, and the common libraries that stack loads itself.
SYS='^(linux-vdso|ld-linux|libc\.|libm\.|libdl\.|libpthread|librt\.|libresolv|libnsl|libutil|libstdc\+\+|libgcc_s|libGL|libEGL|libOpenGL|libX|libxcb|libdrm|libgbm|libwayland|libz\.|libexpat|libzstd|liblzma|libbz2|libelf|libmvec|libpcre|libdbus|libsystemd|libcap|libgcrypt|libgpg-error|libpulse|libasound|libvulkan)'
: > /root/libs.txt
export LD_LIBRARY_PATH="$P/bin:$P/fstop/bin"
for f in "$P"/bin/*.so "$P"/fstop/bin/*.so "$P/hl2_launcher"; do
  ldd "$f" 2>/dev/null | awk '/=>/ {print $1, $3}' >> /root/libs.txt
done
sort -u /root/libs.txt | while read -r name path; do
  case "$path" in "$P"*|not) continue ;; esac
  if echo "$name" | grep -qE "$SYS"; then continue; fi
  cp -L "$path" "$P/lib/$name"
done
echo "unresolved:"; sort -u /root/libs.txt | awk '$2=="not"' | head
echo "bundled:"; ls "$P/lib" | tr '\n' ' '; echo
echo "left to the system:"; sort -u /root/libs.txt | awk '{print $1}' | grep -E "$SYS" | tr '\n' ' '; echo
du -sh "$P" | cut -f1

if [ "$1" = --tar ]; then
  cd /root/pkg
  tar czf /root/F-STOP-SteamDeck.tar.gz F-STOP
  ls -la /root/F-STOP-SteamDeck.tar.gz | cut -c20-
fi
