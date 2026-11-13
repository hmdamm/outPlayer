#!/usr/bin/env bash
# Standalone AppImage and AppDir bundle builder for outPlayer
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${1:-$REPO_ROOT/dist}"
BUILD_DIR="${REPO_ROOT}/build"
VERSION="0.1.0"
ARCH="$(uname -m)"
APPDIR_NAME="outPlayer-${VERSION}-${ARCH}.AppDir"
APPDIR="${OUTPUT_DIR}/${APPDIR_NAME}"
TARBALL="${OUTPUT_DIR}/${APPDIR_NAME}.tar.gz"
APPIMAGE="${OUTPUT_DIR}/outPlayer-${VERSION}-${ARCH}.AppImage"

echo "==> Building AppImage bundle for outPlayer (${VERSION}, ${ARCH})..."
mkdir -p "$OUTPUT_DIR"

# Ensure outplayer binary is built
if [ ! -f "$BUILD_DIR/src/outplayer" ] && [ ! -f "$BUILD_DIR/outplayer" ]; then
    echo "==> Building outPlayer binaries with CMake & Ninja..."
    cmake -B "$BUILD_DIR" -S "$REPO_ROOT" -G Ninja -DCMAKE_BUILD_TYPE=Release
    cmake --build "$BUILD_DIR" -j"$(nproc)"
fi

# Clean and stage new AppDir
echo "==> Staging AppDir at: $APPDIR"
rm -rf "$APPDIR"
mkdir -p "$APPDIR/usr"

# Install CMake binaries, desktop entry, metainfo, and icons
cmake --install "$BUILD_DIR" --prefix "$APPDIR/usr"

# Deploy AppRun entrypoint
echo "==> Deploying AppRun entrypoint..."
cp "$REPO_ROOT/packaging/appimage/AppRun" "$APPDIR/AppRun"
chmod 755 "$APPDIR/AppRun"

# Deploy root desktop entry and application icons for FreeDesktop compliance
echo "==> Deploying root desktop entry and application icons..."
cp "$REPO_ROOT/data/org.outplayer.outPlayer.desktop" "$APPDIR/org.outplayer.outPlayer.desktop"
chmod 644 "$APPDIR/org.outplayer.outPlayer.desktop"

cp "$REPO_ROOT/data/icons/hicolor/256x256/apps/org.outplayer.outPlayer.png" "$APPDIR/org.outplayer.outPlayer.png"
cp "$REPO_ROOT/data/icons/hicolor/256x256/apps/org.outplayer.outPlayer.png" "$APPDIR/.DirIcon"
chmod 644 "$APPDIR/org.outplayer.outPlayer.png" "$APPDIR/.DirIcon"

# Ensure hicolor index.theme is present if host provides it
if [ -f "/usr/share/icons/hicolor/index.theme" ]; then
    cp "/usr/share/icons/hicolor/index.theme" "$APPDIR/usr/share/icons/hicolor/"
fi

# Collect Qt6 plugins (platform, image formats, icon engines, xcbglintegrations)
echo "==> Collecting Qt plugins..."
QT_PLUGIN_SRC="$(qmake6 -query QT_INSTALL_PLUGINS 2>/dev/null || echo /usr/lib/x86_64-linux-gnu/qt6/plugins)"
mkdir -p "$APPDIR/usr/plugins"

for group in platforms imageformats iconengines xcbglintegrations platformthemes platforminputcontexts wayland-shell-integration wayland-graphics-integration-client; do
    if [ -d "$QT_PLUGIN_SRC/$group" ]; then
        mkdir -p "$APPDIR/usr/plugins/$group"
        cp -a "$QT_PLUGIN_SRC/$group"/* "$APPDIR/usr/plugins/$group/" 2>/dev/null || true
    fi
done

# Collect shared libraries
echo "==> Collecting runtime libraries..."
QT_LIB_SRC="$(qmake6 -query QT_INSTALL_LIBS 2>/dev/null || echo /usr/lib/x86_64-linux-gnu)"
mkdir -p "$APPDIR/usr/lib"
if [ -d "$QT_LIB_SRC" ]; then
    cp -P "$QT_LIB_SRC"/libQt6*.so* "$APPDIR/usr/lib/" 2>/dev/null || true
    cp -P "$QT_LIB_SRC"/libmpv.so* "$APPDIR/usr/lib/" 2>/dev/null || true
fi

# Ensure lib64 symlink is present for 64-bit loaders
if [ ! -e "$APPDIR/usr/lib64" ]; then
    ln -sf lib "$APPDIR/usr/lib64"
fi

# Normalize file and directory permissions
find "$APPDIR" -type d -exec chmod 755 {} +
find "$APPDIR/usr" -type f -exec chmod 644 {} +
chmod 755 "$APPDIR/AppRun"
if [ -f "$APPDIR/usr/bin/outplayer" ]; then
    chmod 755 "$APPDIR/usr/bin/outplayer"
fi

# Archive AppDir bundle
echo "==> Archiving AppDir bundle into ${TARBALL}..."
tar -czf "$TARBALL" -C "$OUTPUT_DIR" "$APPDIR_NAME"

# Build single-file AppImage if appimagetool is available
APPIMAGETOOL_BIN="$(command -v appimagetool 2>/dev/null || true)"
if [ -z "$APPIMAGETOOL_BIN" ] && [ -x "${REPO_ROOT}/tools/appimagetool" ]; then
    APPIMAGETOOL_BIN="${REPO_ROOT}/tools/appimagetool"
fi

if [ -n "$APPIMAGETOOL_BIN" ]; then
    echo "==> Packaging standalone AppImage via $APPIMAGETOOL_BIN..."
    ARCH="$ARCH" "$APPIMAGETOOL_BIN" "$APPDIR" "$APPIMAGE"
    chmod 755 "$APPIMAGE"
    echo "==> Successfully created AppImage: $APPIMAGE"
else
    echo "==> Note: appimagetool not found on PATH. Generated AppDir bundle and tarball in ${OUTPUT_DIR}."
fi

echo "==> Successfully built AppDir bundle: $APPDIR"
ls -ld "$APPDIR"
ls -lh "$TARBALL"
