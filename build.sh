#!/bin/bash

# Build script for Hit Log ImGui

echo "Building Hit Log ImGui..."

# Create build directory
mkdir -p build
cd build

# Run CMake
cmake ..

# Build
make -j$(nproc)

echo "Build complete! Run with: ./build/hit_log"
