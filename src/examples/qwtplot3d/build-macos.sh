#!/bin/sh
# Builds lib/libqwtplot3d_macos.a and demo/surface_demo_macos on macOS, with this repository's
# Cocoa GraphApp. Run `make -f Makefile.cocoa` in the src folder first, or set GRAPHAPP_LIB to
# link another libapp.a (for a universal build, make one with ARCH="arm64 -arch x86_64" and
# BUILD=build-macos-dist, then set ARCHS="-arch arm64 -arch x86_64" here too).
# Needs the Xcode command line tools. OpenGL is Apple's legacy 2.1 context: deprecated, but
# still shipped, and the library needs it.
set -e
cd "$(dirname "$0")"
GA=../..
GRAPHAPP_LIB=${GRAPHAPP_LIB:-$GA/build-macos/libapp.a}
ARCHS=${ARCHS:--arch arm64}
if [ ! -f "$GRAPHAPP_LIB" ]; then
  echo "$GRAPHAPP_LIB not found: run make -f Makefile.cocoa in the src folder first."
  exit 1
fi
BUILD=${BUILD:-build-macos}
mkdir -p "$BUILD" lib

# GL/gl.h and GL/glu.h in src/macos_gl forward to Apple's OpenGL headers
COMMON="$ARCHS -mmacosx-version-min=11.0 -O2 -DNDEBUG -DGL_SILENCE_DEPRECATION -w -Wno-c++11-narrowing"
CXXFLAGS="$COMMON -Iinclude -Isrc/macos_gl"

for f in src/qwt3d_*.cpp; do
  case "$f" in
    *_glwidget_win32.cpp|*_glwidget_x11.cpp) continue ;;
  esac
  clang++ $CXXFLAGS -I"$GA" -c "$f" -o "$BUILD/$(basename "$f" .cpp).o" &
done
clang++ $CXXFLAGS -fobjc-arc -I"$GA" -c src/qwt3d_glwidget_cocoa.mm -o "$BUILD/qwt3d_glwidget_cocoa.o" &
wait
# the one file that sees GraphApp's private headers (and not the library's)
clang $COMMON -I"$GA" -I"$GA/cocoa" -I"$GA/cocoa/shim" -I"$GA/utility" -I"$GA/gui" \
  -c src/qwt3d_cocoa_bridge.c -o "$BUILD/qwt3d_cocoa_bridge.o"
clang $COMMON -c 3rdparty/gl2ps/gl2ps.c -o "$BUILD/gl2ps.o"
rm -f lib/libqwtplot3d_macos.a
ar rcs lib/libqwtplot3d_macos.a "$BUILD"/*.o

clang++ $CXXFLAGS -I"$GA" demo/surface_demo.cpp lib/libqwtplot3d_macos.a "$GRAPHAPP_LIB" \
  -framework Cocoa -framework OpenGL -framework CoreText -framework CoreGraphics \
  -framework QuartzCore -lm -o demo/surface_demo_macos
echo "built demo/surface_demo_macos"
