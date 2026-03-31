#!/bin/bash
set -e  

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"

echo "meow meow"

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

if [ ! -f "build.ninja" ]; then
    echo "Configuring project with Ninja..."
    cmake -G Ninja ..
fi

if [ "$1" = "run" ]; then
    echo "Running..."
    ninja run
    echo "Done! QEMU is running..."
elif [ "$1" = "all" ]; then
    echo "Building and running..."
    ninja iso
    ninja run
else
    echo "Building..."
    ninja iso
    echo "frosty_os.iso is now built"
    echo "Run it with: ./build.sh run"
fi
