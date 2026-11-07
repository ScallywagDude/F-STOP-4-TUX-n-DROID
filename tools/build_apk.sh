#!/bin/bash
# Builds the F-STOP APK. usage: build_apk.sh <libs-dir> <out.apk> [asset-root-with-fstop_data]
set -e
FSB="$(cd "$(dirname "$0")/.." && pwd)"
LIBS=$1; OUT=$2; ASSETROOT=$3
BT=$FSB/tools/sdk/android-14
JAR=$(cygpath -m $FSB/tools/sdk/android-34/android.jar)
JDK="/c/Program Files/Eclipse Adoptium/jdk-21.0.12.101-hotspot/bin"
W=$FSB/apkbuild; rm -rf $W; mkdir -p $W/res $W/classes $W/dex $W/stage/lib/arm64-v8a
cd $FSB/launcher
"$BT/aapt2.exe" compile --dir res -o $W/res/compiled.zip
AS=(); [ -d assets ] && AS=(-A assets)
"$BT/aapt2.exe" link -o $W/base.apk -I "$JAR" --manifest AndroidManifest.xml --min-sdk-version 24 --target-sdk-version 34 ${APK_DEBUG:+--debug-mode} --version-code 2 --version-name 1.1 --java $W/gen "${AS[@]}" $W/res/compiled.zip
"$JDK/javac.exe" -nowarn -Xlint:none --release 8 -encoding UTF-8 -cp "$JAR" -d $W/classes $(find src $W/gen -name '*.java') 2>&1 | grep -v 'warning\|^Note' || true
(cd $W/classes && "$JDK/jar.exe" cf ../classes.jar .)
"$JDK/java.exe" -cp "$(cygpath -w $FSB/tools/sdk/bt36/android-16/lib/d8.jar)" com.android.tools.r8.D8 --release --lib "$JAR" --min-api 24 --output "$(cygpath -w $W/dex)" "$(cygpath -w $W/classes.jar)"
cp $W/base.apk $W/unaligned.apk
cp $W/dex/classes.dex $W/stage/
for f in $LIBS/*.so; do cp "$f" $W/stage/lib/arm64-v8a/; done
rm -f $W/stage/lib/arm64-v8a/libvtex_dll.so
cp $FSB/engine/lib/android/aarch64/libSDL2.so $W/stage/lib/arm64-v8a/
node $FSB/tools/zipadd.js "$(cygpath -m $W/unaligned.apk)" "$(cygpath -m $W/stage)" ${ASSETROOT:+"$(cygpath -m $ASSETROOT)"}
"$BT/zipalign.exe" -f -p 16384 $W/unaligned.apk $W/aligned.apk
K=$FSB/keys/fstop.keystore
if [ ! -f $K ]; then mkdir -p $FSB/keys; "$JDK/keytool.exe" -genkeypair -keystore "$(cygpath -w $K)" -storepass fstoplocal -keypass fstoplocal -alias fstop -keyalg RSA -keysize 2048 -validity 10000 -dname "CN=F-STOP personal build" 2>/dev/null; fi
PATH="$JDK:$PATH" JAVA_HOME="C:\\Program Files\\Eclipse Adoptium\\jdk-21.0.12.101-hotspot" cmd //c "$(cygpath -w "$BT/apksigner.bat")" sign --ks "$(cygpath -w $K)" --ks-pass pass:fstoplocal --key-pass pass:fstoplocal --min-sdk-version 24 --out "$(cygpath -w "$OUT")" "$(cygpath -w $W/aligned.apk)"
ls -la "$OUT"
