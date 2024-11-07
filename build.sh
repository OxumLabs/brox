#!/bin/bash

# Function to check if a command exists
command_exists() {
    command -v "$1" &> /dev/null
}

# Function to handle Linux build
build_linux() {
    echo "Do you want to build for Linux? (y/n)"
    read choice
    if [ "$choice" == "y" ]; then
        echo "Building for Linux..."

        if command_exists "zig"; then
            echo "Using Zig for Linux build..."
            zig cc -O3 -march=native -flto -static -o brox.linux brox.c
        elif command_exists "clang"; then
            echo "Using Clang for Linux build..."
            clang -O3 -march=native -flto -static -o brox.linux brox.c
        elif command_exists "gcc"; then
            echo "Using GCC for Linux build..."
            gcc -O3 -march=native -flto -static -o brox.linux brox.c
        else
            echo "Error: No suitable compiler found for Linux build."
            exit 1
        fi

        if [ $? -eq 0 ]; then
            echo "Linux build completed: brox.linux"
        else
            echo "Linux build failed."
        fi
    fi
}

# Function to handle Windows build using Zig, Clang, or GCC
build_windows() {
    echo "Do you want to build for Windows? (y/n)"
    read choice
    if [ "$choice" == "y" ]; then
        echo "Building for Windows..."

        if command_exists "zig"; then
            echo "Using Zig for Windows build..."
            zig cc -O3 -march=native -flto -static -target x86_64-windows -o brox.exe brox.c
        elif command_exists "clang"; then
            echo "Using Clang for Windows build..."
            clang -O3 -march=native -flto -static -target x86_64-windows -o brox.exe brox.c
        elif command_exists "gcc"; then
            echo "Using GCC for Windows build..."
            gcc -O3 -march=native -flto -static -o brox.exe brox.c -L/usr/x86_64-w64-mingw32/lib -lmingw32 -lgcc -lm -lstdc++
        else
            echo "Error: No suitable compiler found for Windows build."
            exit 1
        fi

        if [ $? -eq 0 ]; then
            echo "Windows build completed: brox.exe"
        else
            echo "Windows build failed. Ensure you have the proper cross-compilation tools installed (e.g., MinGW or GCC for Windows)."
        fi
    fi
}

# Function to handle macOS build using Zig, Clang, or GCC
build_macos() {
    echo "Do you want to build for macOS? (y/n)"
    read choice
    if [ "$choice" == "y" ]; then
        echo "Building for macOS..."

        if command_exists "zig"; then
            echo "Using Zig for macOS build..."
            zig cc -O3 -march=native -flto -static -target x86_64-apple-macos12 -o brox.macos brox.c
        elif command_exists "clang"; then
            echo "Using Clang for macOS build..."
            clang -O3 -march=native -flto -static -target x86_64-apple-macos12 -o brox.macos brox.c
        elif command_exists "gcc"; then
            echo "Using GCC for macOS build..."
            gcc -O3 -march=native -flto -static -target x86_64-apple-macos12 -o brox.macos brox.c
        else
            echo "Error: No suitable compiler found for macOS build."
            exit 1
        fi

        if [ $? -eq 0 ]; then
            echo "macOS build completed: brox.macos"
        else
            echo "macOS build failed."
        fi
    fi
}

# Ask for each platform build step by step
build_linux
build_windows
build_macos
