#!/bin/bash

# Ensure the build directory exists
mkdir -p build

# Check if the first argument is 'meow'
if [ "$1" != "meow" ]; then
    nasm -f bin bootloader.asm -o build/bootloader.bin
fi
kvm build/bootloader.bin

