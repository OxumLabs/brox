#!/bin/bash

cmd_exists() {
    command -v "$1" &> /dev/null
}

build_lnux() {
    echo "Do you want to build for Linux? (y/n)"
    read ch
    if [ "$ch" == "y" ]; then
        echo "Building for Linux..."

        if cmd_exists "clang"; then
            echo "Using Clang for Linux build..."
            clang -O3 -march=native -flto -ffast-math -funroll-loops -fomit-frame-pointer \
                -fstrict-aliasing -fno-strict-overflow -fmerge-all-constants -ftree-vectorize \
                -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden \
                -static -o brox.linux brox.c
        else
            echo "Error: Clang not found for Linux build."
            exit 1
        fi

        if [ $? -eq 0 ]; then
            echo "Linux build completed: brox.linux"
        else
            echo "Linux build failed."
        fi
    fi
}

build_wndws() {
    echo "Do you want to build for Windows? (y/n)"
    read ch
    if [ "$ch" == "y" ]; then
        echo "Building for Windows..."

        if cmd_exists "clang"; then
            echo "Using Clang for Windows build..."
            clang -O3 -march=native -flto -ffast-math -funroll-loops -fomit-frame-pointer \
                -fstrict-aliasing -fno-strict-overflow -fmerge-all-constants -ftree-vectorize \
                -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden \
                -static -target x86_64-windows -o brox.exe brox.c
        else
            echo "Error: Clang not found for Windows build."
            exit 1
        fi

        if [ $? -eq 0 ]; then
            echo "Windows build completed: brox.exe"
        else
            echo "Windows build failed. Ensure you have the proper cross-compilation tools installed (e.g., Clang with Windows target)."
        fi
    fi
}

build_mcs() {
    echo "Do you want to build for macOS? (y/n)"
    read ch
    if [ "$ch" == "y" ]; then
        echo "Building for macOS..."

        if cmd_exists "clang"; then
            echo "Using Clang for macOS build..."
            clang -O3 -march=native -flto -ffast-math -funroll-loops -fomit-frame-pointer \
                -fstrict-aliasing -fno-strict-overflow -fmerge-all-constants -ftree-vectorize \
                -fno-exceptions -fno-rtti -fvisibility=hidden -fvisibility-inlines-hidden \
                -static -target x86_64-apple-macos12 -o brox.macos brox.c
        else
            echo "Error: Clang not found for macOS build."
            exit 1
        fi

        if [ $? -eq 0 ]; then
            echo "macOS build completed: brox.macos"
        else
            echo "macOS build failed."
        fi
    fi
}

build_lnux
build_wndws
build_mcs
