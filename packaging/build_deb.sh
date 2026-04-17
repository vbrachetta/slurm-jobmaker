#!/bin/bash
# =============================================================================
#  Slurm JobMaker — DEB Package Builder
#  -------------------------------------
#  Purpose:
#    Compile the application, assemble the package directory structure,
#    and produce a ready-to-install .deb package.
#
#  Requirements:
#    - build-essential
#    - cmake
#    - qt6-base-dev
#    - rpm
#
#  Usage:
#    ./packaging/build_deb.sh
#
#  Notes:
#    - The package will be created in the project root.
#
#  Developed and tested on: Debian GNU/Linux 13.4 (Trixie)
# =============================================================================

set -e

PACKAGE="slurm-jobmaker"
VERSION="1.0.0"
BUILD_DIR="build_deb"
INSTALL_DIR="${BUILD_DIR}/${PACKAGE}"

# -----------------------------------------------------------------------------
# Check dependencies
# -----------------------------------------------------------------------------
echo "Checking dependencies..."

for cmd in cmake make g++ dpkg-deb; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "ERROR: '$cmd' not found. Install it with:"
        echo "  sudo apt install build-essential cmake dpkg-dev"
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
# Build the binary
# -----------------------------------------------------------------------------
echo "Building..."
mkdir -p "${BUILD_DIR}/build"
cmake -B "${BUILD_DIR}/build" \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_INSTALL_PREFIX=/usr
cmake --build "${BUILD_DIR}/build" --parallel "$(nproc)"

# -----------------------------------------------------------------------------
# Create package directory structure
# -----------------------------------------------------------------------------
echo "Creating package structure..."

mkdir -p "${INSTALL_DIR}/usr/bin"
mkdir -p "${INSTALL_DIR}/usr/share/${PACKAGE}/profiles"
mkdir -p "${INSTALL_DIR}/usr/share/applications"
mkdir -p "${INSTALL_DIR}/usr/share/doc/${PACKAGE}"
mkdir -p "${INSTALL_DIR}/DEBIAN"

# -----------------------------------------------------------------------------
# Install files
# -----------------------------------------------------------------------------
cp "${BUILD_DIR}/build/SlurmJobMaker"        "${INSTALL_DIR}/usr/bin/"
cp profiles/bluebear.conf                    "${INSTALL_DIR}/usr/share/${PACKAGE}/profiles/"
cp profiles/generic.conf                     "${INSTALL_DIR}/usr/share/${PACKAGE}/profiles/"
cp packaging/debian/copyright                "${INSTALL_DIR}/usr/share/doc/${PACKAGE}/"
cp packaging/debian/changelog                "${INSTALL_DIR}/usr/share/doc/${PACKAGE}/"
cp packaging/debian/control                  "${INSTALL_DIR}/DEBIAN/"

for size in 16x16 32x32 48x48 128x128 256x256; do
    mkdir -p "${INSTALL_DIR}/usr/share/icons/hicolor/${size}/apps"
    cp "assets/icons/${size}/slurm-jobmaker.png" \
       "${INSTALL_DIR}/usr/share/icons/hicolor/${size}/apps/"
done

# -----------------------------------------------------------------------------
# Desktop entry
# -----------------------------------------------------------------------------
cat > "${INSTALL_DIR}/usr/share/applications/slurm-jobmaker.desktop" << EOF
[Desktop Entry]
Name=Slurm JobMaker
Comment=Generate Slurm job submission scripts
Exec=/usr/bin/SlurmJobMaker
Icon=slurm-jobmaker
Terminal=false
Type=Application
Categories=Science;Education;
EOF

# -----------------------------------------------------------------------------
# Set permissions
# -----------------------------------------------------------------------------
chmod 755 "${INSTALL_DIR}/usr/bin/SlurmJobMaker"
chmod 644 "${INSTALL_DIR}/usr/share/applications/slurm-jobmaker.desktop"
chmod 644 "${INSTALL_DIR}/usr/share/${PACKAGE}/profiles/"*.conf

# -----------------------------------------------------------------------------
# Build the.deb package
# -----------------------------------------------------------------------------
echo "Building.deb package..."
dpkg-deb --root-owner-group --build "${INSTALL_DIR}" "${PACKAGE}_${VERSION}_amd64.deb"

echo ""
echo "Package built: ${PACKAGE}_${VERSION}_amd64.deb"
echo ""
echo "Install with:"
echo "  sudo dpkg -i ${PACKAGE}_${VERSION}_amd64.deb"
echo ""
echo "Remove with:"
echo "  sudo dpkg -r ${PACKAGE}"
