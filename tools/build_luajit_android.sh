#!/usr/bin/env bash
set -euo pipefail
: "${ANDROID_NDK_HOME:?Set ANDROID_NDK_HOME}"
ABI="${1:-arm64-v8a}"
API="${2:-24}"
OUT="${3:-$(pwd)/luajit-android}"
ROOT="$(pwd)/third_party/LuaJIT"
if [ ! -d "$ROOT/.git" ]; then
  mkdir -p "$(dirname "$ROOT")"
  git clone --depth 1 --branch v2.1 https://github.com/LuaJIT/LuaJIT.git "$ROOT"
fi
HOST_TAG="linux-x86_64"
case "$ABI" in
  arm64-v8a) TARGET=aarch64-linux-android; ARCH=arm64 ;;
  armeabi-v7a) TARGET=armv7a-linux-androideabi; ARCH=arm ;;
  x86_64) TARGET=x86_64-linux-android; ARCH=x64 ;;
  *) echo "Unsupported ABI: $ABI" >&2; exit 2 ;;
esac
TOOLCHAIN="$ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/bin"
make -C "$ROOT" clean
make -C "$ROOT" -j2 \
  HOST_CC="gcc -m64" \
  CROSS="$TOOLCHAIN/$TARGET$API-" \
  TARGET_SYS=Linux TARGET_FLAGS="--sysroot $ANDROID_NDK_HOME/toolchains/llvm/prebuilt/$HOST_TAG/sysroot" \
  TARGET_CFLAGS="-fPIC" TARGET_LD="$TOOLCHAIN/$TARGET$API-clang"
mkdir -p "$OUT/lib" "$OUT/include"
cp "$ROOT/src/libluajit.so" "$OUT/lib/"
cp "$ROOT/src"/*.h "$OUT/include/"
echo "$OUT/lib/libluajit.so"
