#!/bin/bash
# =============================================================================
#  Slurm JobMaker — Build Script
#  ------------------------------
#  Purpose:
#    Compile the application from source and place the binary in
#    the build/ directory.
#
#  Requirements:
#    - build-essential
#    - cmake
#    - qt6-base-dev
#
#  Usage:
#    ./build.sh
#
#  Notes:
#    - The binary will be placed in ./build/SlurmJobMaker.
#
#  Developed and tested on: Debian GNU/Linux 13.4 (Trixie)
# =============================================================================

set -e  # Exit immediately on any error

PROJECT="SlurmJobMaker"
BUILD_DIR="build"

# -----------------------------------------------------------------------------
# Check dependencies
# -----------------------------------------------------------------------------
echo "Checking dependencies..."

for cmd in cmake make g++; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "ERROR: '$cmd' not found. Install it with:"
        echo "  sudo apt install build-essential cmake"
        exit 1
    fi
done

if ! dpkg -s qt6-base-dev &> /dev/null; then
    echo "ERROR: Qt6 development package not found. Install it with:"
    echo "  sudo apt install qt6-base-dev"
    exit 1
fi

echo "All dependencies found."

# -----------------------------------------------------------------------------
# Create and enter build directory
# -----------------------------------------------------------------------------
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# -----------------------------------------------------------------------------
# Configure and build
# -----------------------------------------------------------------------------
echo "Configuring..."
cmake .. -DCMAKE_BUILD_TYPE=Release

echo "Building..."
make -j"$(nproc)"

# -----------------------------------------------------------------------------
# Done
# -----------------------------------------------------------------------------
echo ""
echo "Build complete. Run the application with:"
echo "  ./${BUILD_DIR}/${PROJECT}"
