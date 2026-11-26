#!/usr/bin/env bash
# Standalone Debian/Ubuntu .deb package builder for outPlayer
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${1:-$REPO_ROOT/dist}"
BUILD_DIR="${REPO_ROOT}/build"
VERSION="0.1.1-1"

# Determine host architecture
ARCH="$(dpkg --print-architecture 2>/dev/null || uname -m)"
case "$ARCH" in
    x86_64)  ARCH="amd64" ;;
    aarch64) ARCH="arm64" ;;
    armv7l)  ARCH="armhf" ;;
esac

echo "==> Building Debian package for outPlayer (${VERSION}, ${ARCH})..."
mkdir -p "$OUTPUT_DIR"

# Ensure outplayer binary is built
if [ ! -f "$BUILD_DIR/src/outplayer" ] && [ ! -f "$BUILD_DIR/outplayer" ]; then
    echo "==> Building outPlayer binaries with CMake & Ninja..."
    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

# Create isolated staging root
STAGING_DIR="$(mktemp -d /tmp/outplayer-deb-staging.XXXXXX)"
trap 'rm -rf "$STAGING_DIR"' EXIT

echo "==> Installing files into staging directory: $STAGING_DIR"
cmake --install "$BUILD_DIR" --prefix "$STAGING_DIR/usr"

# Create DEBIAN metadata directory
mkdir -p "$STAGING_DIR/DEBIAN"
chmod 755 "$STAGING_DIR/DEBIAN"

# Calculate installed size in KB
INSTALLED_SIZE="$(du -sk "$STAGING_DIR/usr" 2>/dev/null | cut -f1 || echo 18000)"

# Generate binary package control file
cat <<EOF > "$STAGING_DIR/DEBIAN/control"
Package: outplayer
Version: ${VERSION}
Section: video
Priority: optional
Architecture: ${ARCH}
Maintainer: outPlayer Developers <dev@outplayer.org>
Installed-Size: ${INSTALLED_SIZE}
Depends: libc6 (>= 2.34), libqt6core6 | libqt6core6t64, libqt6gui6 | libqt6gui6t64, libqt6widgets6 | libqt6widgets6t64, libqt6opengl6 | libqt6opengl6t64, libqt6openglwidgets6 | libqt6openglwidgets6t64, libqt6qml6 | libqt6qml6t64, libqt6quick6 | libqt6quick6t64, libqt6svg6 | libqt6svg6t64, libmpv2 (>= 0.35.0)
Homepage: https://github.com/hmdamm/outPlayer
Description: Modern, high-performance Linux media player
 outPlayer is a feature-rich, high-performance Linux media player
 powered by Qt 6 and libmpv. It provides hardware-accelerated
 video playback, a responsive frameless interface, customizable hotkeys,
 advanced playlist management, and deep desktop integration.
EOF

chmod 644 "$STAGING_DIR/DEBIAN/control"

# Normalize directory and file permissions
find "$STAGING_DIR" -type d -exec chmod 755 {} +
find "$STAGING_DIR/usr" -type f -exec chmod 644 {} +
if [ -f "$STAGING_DIR/usr/bin/outplayer" ]; then
    chmod 755 "$STAGING_DIR/usr/bin/outplayer"
fi

DEB_PACKAGE="${OUTPUT_DIR}/outplayer_${VERSION}_${ARCH}.deb"

echo "==> Packaging .deb using dpkg-deb..."
dpkg-deb --build --root-owner-group "$STAGING_DIR" "$DEB_PACKAGE"

echo "==> Successfully built Debian package: $DEB_PACKAGE"
ls -lh "$DEB_PACKAGE"
