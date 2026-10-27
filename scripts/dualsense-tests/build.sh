#!/usr/bin/env bash
# Builds the DualSense hardware test tools against an existing chiaki-ng build.
# Usage: scripts/dualsense-tests/build.sh [build-dir] [output-dir]
#   build-dir   chiaki-ng CMake build directory (default: build)
#   output-dir  where the test binaries go (default: <build-dir>/dualsense-tests)
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
BUILD="$(realpath "${1:-$ROOT/build}")"
OUT="$(realpath -m "${2:-$BUILD/dualsense-tests}")"
SRC="$ROOT/scripts/dualsense-tests"
LIBCHIAKI="$BUILD/lib/libchiaki.a"

if [ ! -f "$LIBCHIAKI" ]; then
	echo "libchiaki.a not found in $BUILD/lib, build chiaki-ng first" >&2
	exit 1
fi

MOC="$(pkg-config --variable=libexecdir Qt6Core)/moc"
if [ ! -x "$MOC" ]; then
	echo "Qt6 moc not found at $MOC" >&2
	exit 1
fi

mkdir -p "$OUT"
SDL_CFLAGS="$(pkg-config --cflags sdl2)"
SDL_LIBS="$(pkg-config --libs sdl2)"
CHIAKI_INC="-I$ROOT/lib/include -I$BUILD/lib/include"

# Haptics test reuses the real DualSenseBtHaptics class from the GUI
"$MOC" "$ROOT/gui/include/dualsensebthaptics.h" -o "$OUT/moc_dualsensebthaptics.cpp"
g++ -std=c++17 -O1 -I"$ROOT/gui/include" $CHIAKI_INC $(pkg-config --cflags Qt6Core) $SDL_CFLAGS \
	"$SRC/hwtest.cpp" "$OUT/moc_dualsensebthaptics.cpp" "$ROOT/gui/src/dualsensebthaptics.cpp" \
	"$LIBCHIAKI" $(pkg-config --libs Qt6Core) $SDL_LIBS -o "$OUT/hwtest"

# Touchpad test uses the real libchiaki touch tracking
g++ -std=c++17 -O1 $CHIAKI_INC $SDL_CFLAGS "$SRC/padtest.cpp" "$LIBCHIAKI" $SDL_LIBS -o "$OUT/padtest"

g++ -std=c++17 -O1 $SDL_CFLAGS "$SRC/trigtest.cpp" $SDL_LIBS -o "$OUT/trigtest"
g++ -std=c++17 -O1 $SDL_CFLAGS "$SRC/edgetest.cpp" $SDL_LIBS -o "$OUT/edgetest"

echo "Built hwtest, trigtest, padtest and edgetest in $OUT"
