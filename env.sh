# Build environment. Source this from the repository root after unpacking the NDK and LLVM into tools/.
FSB=$(pwd)
export ANDROID_NDK_HOME="$(pwd -W)\tools\android-ndk-r10e"
export LLVM_BIN="$(pwd -W)\tools\llvm\bin"
export PATH="$FSB/tools/llvm/bin:$PATH"
PY=$FSB/tools/py/python.exe
CFG="-T release --android=aarch64,host,21 --togles --use-togl=1 --use-sdl=1 --disable-warns --build-games=portal --prefix=../install --check-c-compiler=clang --check-cxx-compiler=clang++"
export LLVM_W="$(pwd -W)\tools\llvmw\bin"
