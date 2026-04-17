#!/bin/bash
# =============================================================================
#  Slurm JobMaker — RPM Package Builder
#  -------------------------------------
#  Purpose:
#    Create a source tarball from the Git repository, compile the
#    application inside the rpmbuild environment, and produce a
#    ready-to-install .rpm package.
#
#  Requirements:
#    - cmake
#    - gcc-c++
#    - qt6-base-dev
#    - rpm
#    - git
#
#  Usage:
#    ./packaging/build_rpm.sh
#
#  Notes:
#    - Requires the project to be inside a Git repository with at
#      least one commit.
#    - The package will be created in the project root.
#
#  Developed and tested on: Debian GNU/Linux 13.4 (Trixie)
# =============================================================================

set -e

PACKAGE="slurm-jobmaker"
VERSION="1.0.0"
RPM_BUILD_DIR="${HOME}/rpmbuild"

# -----------------------------------------------------------------------------
# Check dependencies
# -----------------------------------------------------------------------------
echo "Checking dependencies..."

for cmd in cmake make g++ rpmbuild git; do
    if ! command -v "$cmd" &> /dev/null; then
        echo "ERROR: '$cmd' not found. Install it with:"
        echo "  sudo apt install build-essential cmake rpm git"
        exit 1
    fi
done

echo "All dependencies found."

# -----------------------------------------------------------------------------
# Set up rpmbuild directory structure
# -----------------------------------------------------------------------------
echo "Setting up rpmbuild environment..."
mkdir -p "${RPM_BUILD_DIR}"/{BUILD,RPMS,SOURCES,SPECS,SRPMS}

# -----------------------------------------------------------------------------
# Create source tarball
# -----------------------------------------------------------------------------
echo "Creating source tarball..."
git archive --format=tar.gz \
            --prefix="${PACKAGE}-${VERSION}/" \
            HEAD > "${RPM_BUILD_DIR}/SOURCES/${PACKAGE}-${VERSION}.tar.gz"

# -----------------------------------------------------------------------------
# Copy spec file
# -----------------------------------------------------------------------------
cp packaging/rpm/${PACKAGE}.spec "${RPM_BUILD_DIR}/SPECS/"

# -----------------------------------------------------------------------------
# Build the RPM
# -----------------------------------------------------------------------------
echo "Building RPM..."
rpmbuild -ba "${RPM_BUILD_DIR}/SPECS/${PACKAGE}.spec" \
         --nodeps

# -----------------------------------------------------------------------------
# Copy result to project directory
# -----------------------------------------------------------------------------
find "${RPM_BUILD_DIR}/RPMS" -name "${PACKAGE}-${VERSION}*.rpm" \
     -exec cp {} . \;

echo ""
echo "Package built: ${PACKAGE}-${VERSION}-1.x86_64.rpm"
echo ""
echo "Install with:"
echo "  sudo rpm -i ${PACKAGE}-${VERSION}-1.x86_64.rpm"
echo ""
echo "Remove with:"
echo "  sudo rpm -e ${PACKAGE}"
