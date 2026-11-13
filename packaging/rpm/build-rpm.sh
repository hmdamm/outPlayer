#!/usr/bin/env bash
# Standalone Fedora/RPM package builder for outPlayer
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
OUTPUT_DIR="${1:-$REPO_ROOT/dist}"
VERSION="0.1.0"
RELEASE="1"

echo "==> Building RPM packages for outPlayer (${VERSION}-${RELEASE})..."
mkdir -p "$OUTPUT_DIR"

# Create isolated RPM topdir build environment
RPM_TOPDIR="$(mktemp -d /tmp/outplayer-rpmbuild.XXXXXX)"
trap 'rm -rf "$RPM_TOPDIR"' EXIT

mkdir -p "$RPM_TOPDIR"/{BUILD,BUILDROOT,RPMS,SOURCES,SPECS,SRPMS}

echo "==> Packaging source tarball into $RPM_TOPDIR/SOURCES/outplayer-${VERSION}.tar.gz..."
tar -czf "$RPM_TOPDIR/SOURCES/outplayer-${VERSION}.tar.gz" \
    --transform "s,^,outplayer-${VERSION}/," \
    -C "$REPO_ROOT" \
    CMakeLists.txt src tests data packaging README.md

echo "==> Staging RPM spec file..."
cp "$REPO_ROOT/packaging/rpm/outplayer.spec" "$RPM_TOPDIR/SPECS/"

echo "==> Executing rpmbuild..."
rpmbuild -ba --nodeps --define "_topdir $RPM_TOPDIR" "$RPM_TOPDIR/SPECS/outplayer.spec"

echo "==> Copying RPM artifacts to $OUTPUT_DIR..."
find "$RPM_TOPDIR/RPMS" "$RPM_TOPDIR/SRPMS" -type f -name "*.rpm" -exec cp {} "$OUTPUT_DIR/" \;

echo "==> Successfully built RPM packages:"
find "$OUTPUT_DIR" -type f -name "outplayer-*.rpm" -exec ls -lh {} \;
