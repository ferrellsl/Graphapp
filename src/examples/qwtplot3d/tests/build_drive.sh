#!/bin/sh
# Builds build-macos/surface_demo_drive: the demo with cocoa_drive.mm linked in (see that file).
# Run build-macos.sh first.
set -e
cd "$(dirname "$0")/.."
GA=../..
GRAPHAPP_LIB=${GRAPHAPP_LIB:-$GA/build-macos/libapp.a}
ARCHS=${ARCHS:--arch arm64}
COMMON="$ARCHS -mmacosx-version-min=11.0 -O2 -DNDEBUG -DGL_SILENCE_DEPRECATION -w -Wno-c++11-narrowing"
clang++ $COMMON -fobjc-arc -Iinclude -Isrc/macos_gl -c tests/cocoa_drive.mm -o build-macos/cocoa_drive.o
clang++ $COMMON -Iinclude -Isrc/macos_gl -I"$GA" demo/surface_demo.cpp build-macos/cocoa_drive.o \
  lib/libqwtplot3d_macos.a "$GRAPHAPP_LIB" \
  -framework Cocoa -framework OpenGL -framework CoreText -framework CoreGraphics \
  -framework QuartzCore -lm -o build-macos/surface_demo_drive
echo "built build-macos/surface_demo_drive"
