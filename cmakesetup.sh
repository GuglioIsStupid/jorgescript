#!/usr/bin/env sh
set -eu

BUILD_DIR="build"
BUILD_TYPE="Debug"

if [ "$#" -ge 1 ] && [ -n "$1" ]; then
    BUILD_DIR="$1"
fi

if [ "$#" -ge 2 ] && [ -n "$2" ]; then
    BUILD_TYPE="$2"
fi

cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
