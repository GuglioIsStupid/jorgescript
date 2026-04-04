#!/usr/bin/env sh
set -eu

BUILD_DIR="build"
CONFIG="Debug"

if [ "$#" -ge 1 ] && [ -n "$1" ]; then
    BUILD_DIR="$1"
fi

if [ "$#" -ge 2 ] && [ -n "$2" ]; then
    CONFIG="$2"
fi

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
    exit 1
fi

cmake --build "$BUILD_DIR" --config "$CONFIG"
