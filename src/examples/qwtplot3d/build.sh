#!/bin/sh
# Builds lib/libqwtplot3d.a and demo/surface_demo on Linux, with this repository's GraphApp.
# Run make in the src folder first, or set GRAPHAPP_LIB to link another libapp.a.
# Needs the OpenGL, GLU and X11 development packages (libgl-dev libglu1-mesa-dev libx11-dev).
set -e
cd "$(dirname "$0")"
GA=../..
GRAPHAPP_LIB=${GRAPHAPP_LIB:-$GA/libapp.a}
if [ ! -f "$GRAPHAPP_LIB" ]; then
  echo "$GRAPHAPP_LIB not found: run make in the src folder first."
  exit 1
fi
BUILD=${BUILD:-build}
mkdir -p "$BUILD" lib

CXXFLAGS="-O2 -DNDEBUG -Iinclude -w"
for f in src/qwt3d_*.cpp; do
  g++ $CXXFLAGS -I"$GA" -c "$f" -o "$BUILD/$(basename "$f" .cpp).o" &
done
wait
gcc -O2 -w -I"$GA" -I"$GA/x11" -I"$GA/utility" -I"$GA/gui" -c src/qwt3d_x11_bridge.c -o "$BUILD/qwt3d_x11_bridge.o"
gcc -O2 -w -c 3rdparty/gl2ps/gl2ps.c -o "$BUILD/gl2ps.o"
rm -f lib/libqwtplot3d.a
ar rcs lib/libqwtplot3d.a "$BUILD"/*.o

g++ $CXXFLAGS -no-pie -I"$GA" demo/surface_demo.cpp lib/libqwtplot3d.a "$GRAPHAPP_LIB" \
  -lGL -lGLU -lX11 -lm -o demo/surface_demo
