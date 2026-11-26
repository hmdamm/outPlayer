# outPlayer Distribution & Packaging Guide

This guide details installation, building, sandbox configuration, desktop integration, and maintenance for **outPlayer** across all supported Linux distribution ecosystems: **Flatpak**, **Debian/Ubuntu (.deb)**, **Fedora/RHEL (.rpm)**, **Arch Linux (AUR / PKGBUILD)**, and **Standalone AppImage**.

---

## Table of Contents

1. [Package Architecture & Distribution Matrix](#package-architecture--distribution-matrix)
2. [Desktop Integration Standards](#desktop-integration-standards)
3. [Flatpak / Flathub](#flatpak--flathub)
4. [Debian / Ubuntu (.deb)](#debian--ubuntu-deb)
5. [Fedora / RHEL / CentOS Stream (.rpm)](#fedora--rhel--centos-stream-rpm)
6. [Arch Linux & AUR (PKGBUILD)](#arch-linux--aur-pkgbuild)
7. [Standalone AppImage & AppDir](#standalone-appimage--appdir)
8. [Desktop Integration Verification](#desktop-integration-verification)
9. [Release Integrity, Checksums & Reproducibility](#release-integrity-checksums--reproducibility)
10. [Uninstallation & User Configuration](#uninstallation--user-configuration)

---

## Package Architecture & Distribution Matrix

outPlayer provides native packaging manifests and automated build recipes for all tier-one Linux distribution channels:

| Format / Channel | Target Environments | Sandboxed | Dependencies | Build Manifest / Script |
|---|---|---|---|---|
| **Flatpak / Flathub** | Universal Linux (GNOME, KDE, XFCE, Sway, Hyprland) | Yes (bwrap) | Bundled via `org.kde.Platform` 6.8 | `packaging/flatpak/org.outplayer.outPlayer.json` |
| **Debian / Ubuntu (.deb)** | Debian 12+, Ubuntu 22.04+, Linux Mint, Pop!_OS | No (Native) | `libqt6widgets6`, `libmpv2`, `qt6-base` | `packaging/debian/build-deb.sh` (`debian/control`) |
| **Fedora / RHEL (.rpm)** | Fedora 39+, RHEL 9+, CentOS Stream 9+, Rocky | No (Native) | `qt6-qtbase`, `mpv-libs`, `qt6-qtsvg` | `packaging/rpm/build-rpm.sh` (`packaging/rpm/outplayer.spec`) |
| **Arch Linux / AUR** | Arch Linux, Manjaro, EndeavourOS | No (Native) | `qt6-base`, `qt6-svg`, `mpv` | `packaging/arch/PKGBUILD` |
| **Standalone AppImage** | Distro-agnostic portable binary (glibc 2.35+) | Self-contained | Bundled Qt 6 & libmpv | `packaging/appimage/build-appimage.sh` (`packaging/appimage/AppRun`) |

Reverse-DNS identifier: **`org.outplayer.outPlayer`**  
Canonical binary executable: **`outplayer`**  
Current Release: **`v0.1.1`**

---

## Desktop Integration Standards

outPlayer conforms strictly to FreeDesktop.org (XDG) specifications:

- **Desktop Entry (`data/org.outplayer.outPlayer.desktop`)**:
  - Specification: FreeDesktop Desktop Entry Specification 1.5.
  - Categories: `AudioVideo;Player;Recorder;TV;`.
  - MIME Associations: 40+ media containers and streaming playlists (`video/mp4`, `video/x-matroska`, `video/webm`, `video/quicktime`, `video/x-msvideo`, `audio/mpeg`, `audio/flac`, `audio/x-wav`, `audio/ogg`, `application/x-mpegURL`, `application/dash+xml`, `text/vtt`, `application/x-subrip`, etc.).
  - Quicklist Desktop Actions:
    - `OpenMedia`: Open file browser dialog.
    - `OpenURL`: Stream from network URL.
    - `OpenDevice`: Ingest webcam or capture card device.
    - `Preferences`: Direct access to preferences tree.

- **AppStream Metainfo (`data/org.outplayer.outPlayer.metainfo.xml`)**:
  - Specification: AppStream 1.0 component metadata (`type="desktop-application"`).
  - Validated by `appstreamcli validate --no-net`.
  - Comprehensive feature summaries, release notes, hardware requirements, developer identity, and OARS 1.1 content rating.

- **Hicolor Vector and Raster Icons (`data/icons/hicolor/`)**:
  - Scalable vector icon: `data/icons/hicolor/scalable/apps/org.outplayer.outPlayer.svg`.
  - Multi-resolution raster icons: `16x16`, `32x32`, `48x48`, `64x64`, `128x128`, `256x256`, `512x512`.

- **CMake Install Rules (`CMakeLists.txt` / `src/CMakeLists.txt`)**:
  - Utilizes standard `GNUInstallDirs`:
    - Binary: `${CMAKE_INSTALL_BINDIR}/outplayer` (`/usr/bin/outplayer`)
    - Desktop entry: `${CMAKE_INSTALL_DATADIR}/applications/org.outplayer.outPlayer.desktop`
    - Metainfo XML: `${CMAKE_INSTALL_DATADIR}/metainfo/org.outplayer.outPlayer.metainfo.xml`
    - Icon hierarchy: `${CMAKE_INSTALL_DATADIR}/icons/hicolor/...`

---

## Flatpak / Flathub

Flatpak provides an isolated, sandboxed environment that runs identically on any modern Linux distribution.

### 1. Installation from Flathub

Once published to Flathub:

```bash
# Install outPlayer from Flathub
flatpak install flathub org.outplayer.outPlayer

# Launch outPlayer
flatpak run org.outplayer.outPlayer

# Launch with media target
flatpak run org.outplayer.outPlayer /path/to/media.mkv
```

### 2. Sandbox Permissions & Security Architecture

The manifest (`packaging/flatpak/org.outplayer.outPlayer.json`) configures least-privilege permissions required for high-performance audio/video presentation:

- `--socket=wayland` and `--socket=fallback-x11`: Native Wayland presentation with seamless X11 fallback.
- `--share=ipc`: Shared memory IPC for high-throughput video buffer transfers.
- `--socket=pulseaudio` and `--filesystem=xdg-run/pipewire-0`: Native low-latency audio via PulseAudio and PipeWire.
- `--device=all`: Direct Rendering Infrastructure (DRI) GPU acceleration (`/dev/dri/*`) and optical disc drives (`/dev/sr*`, `/dev/cdrom`).
- `--filesystem=host:ro`: Safe, read-only access to host media libraries and external storage mounts.
- `--filesystem=xdg-download:ro`, `--filesystem=xdg-videos:ro`, `--filesystem=xdg-music:ro`, `--filesystem=xdg-pictures:ro`: Standard XDG media folders.
- `--talk-name=org.mpris.MediaPlayer2.*`: Host desktop media key and system tray integration.
- `--talk-name=org.freedesktop.ScreenSaver`: Power management inhibition preventing screen lock during media playback.

### 3. Filesystem & Custom Mount Overrides

To grant write access or expose custom storage arrays (e.g., dedicated NAS mount points):

```bash
# Grant access to custom storage mount
flatpak override --user --filesystem=/mnt/storage:ro org.outplayer.outPlayer

# Grant write access to a directory for saving snapshots
flatpak override --user --filesystem=~/Pictures/Captures:create org.outplayer.outPlayer
```

### 4. Local Build with `flatpak-builder`

To build and verify the Flatpak package from source:

```bash
# Ensure flatpak and flatpak-builder are installed
sudo apt-get install -y flatpak flatpak-builder # Debian/Ubuntu
# or: sudo dnf install -y flatpak flatpak-builder # Fedora

# Install KDE 6.8 Platform and Sdk runtimes
flatpak install -y flathub org.kde.Platform//6.8 org.kde.Sdk//6.8

# Build and install locally
flatpak-builder --user --install --force-clean build-flatpak packaging/flatpak/org.outplayer.outPlayer.json

# Test launch installed build
flatpak run org.outplayer.outPlayer
```

---

## Debian / Ubuntu (.deb)

The Debian packaging hierarchy supports Debian 12 (Bookworm), Ubuntu 22.04 LTS (Jammy), Ubuntu 24.04 LTS (Noble), and derivatives (Linux Mint, Pop!_OS).

### 1. Installation via `.deb` Binary Package

```bash
# Install binary deb package
sudo dpkg -i outplayer_0.1.1-1_amd64.deb

# If dependencies are missing, automatically fetch and install them:
sudo apt-get install -f -y
```

### 2. Runtime Dependencies

- `libc6 (>= 2.35)`
- `libqt6core6`, `libqt6gui6`, `libqt6widgets6`, `libqt6openglwidgets6`, `libqt6svg6` (>= 6.2.0)
- `libmpv2` (>= 0.35.0)
- `libgl1`

### 3. Automated Package Build (`build-deb.sh`)

The repository includes a standalone automated packaging script:

```bash
# Run the automated deb package builder
bash packaging/debian/build-deb.sh dist/

# Output package:
# dist/outplayer_0.1.1-1_amd64.deb
```

### 4. Native Debian Source Packaging (`debian/rules`)

To build with standard Debian packaging toolchains (`dpkg-buildpackage`):

```bash
# Install packaging build dependencies
sudo apt-get update
sudo apt-get install -y debhelper-compat cmake ninja-build pkg-config \
    qt6-base-dev libqt6opengl6-dev qt6-declarative-dev libqt6svg6-dev libmpv-dev

# Prepare upstream source tarball
tar --exclude-vcs -czf ../outplayer_0.1.1.orig.tar.gz .

# Copy debian control files to project root if building natively
cp -r packaging/debian .

# Build binary package without signing
dpkg-buildpackage -us -uc -b

# Verify package structure
dpkg-deb --contents ../outplayer_0.1.1-1_*.deb
dpkg-deb --info ../outplayer_0.1.1-1_*.deb
```

---

## Fedora / RHEL / CentOS Stream (.rpm)

Fedora 39+, Red Hat Enterprise Linux 9+, and CentOS Stream packages are built using modern RPM spec standards.

### 1. Installation via `.rpm` Binary Package

```bash
# Install using dnf (resolves dependencies automatically)
sudo dnf install ./outplayer-0.1.1-1.x86_64.rpm
```

### 2. Runtime Dependencies

- `qt6-qtbase`
- `qt6-qtsvg`
- `mpv-libs (>= 0.35.0)`
- `hicolor-icon-theme`

### 3. Automated Package Build (`build-rpm.sh`)

```bash
# Run automated rpm package builder
bash packaging/rpm/build-rpm.sh dist/

# Output package:
# dist/outplayer-0.1.1-1.x86_64.rpm
```

### 4. Building with `rpmbuild`

```bash
# Install rpm build tools and dependencies
sudo dnf install -y rpm-build rpmdevtools cmake ninja-build gcc-c++ \
    qt6-qtbase-devel qt6-qtsvg-devel mpv-libs-devel

# Setup standard rpmbuild directory hierarchy
rpmdev-setuptree

# Archive source into rpmbuild SOURCES
tar --exclude-vcs -czf ~/rpmbuild/SOURCES/outplayer-0.1.1.tar.gz .

# Execute rpmbuild
rpmbuild -ba packaging/rpm/outplayer.spec

# Generated RPM will be in:
# ~/rpmbuild/RPMS/x86_64/outplayer-0.1.1-1.*.x86_64.rpm
```

---

## Arch Linux & AUR (PKGBUILD)

Arch Linux users can install via AUR helpers or build directly using `makepkg`.

### 1. Installation via AUR Helper

Once published to the AUR:

```bash
# Install using yay
yay -S outplayer

# Or install using paru
paru -S outplayer
```

### 2. Manual Build with `makepkg`

```bash
# Navigate to Arch packaging recipe
cd packaging/arch

# Synchronize dependencies, compile and install
makepkg -si
```

### 3. PKGBUILD Specification Details

The `PKGBUILD` adheres to Arch Packaging Standards:
- `pkgname`: `outplayer`
- `pkgver`: `0.1.1`
- `pkgrel`: `1`
- `arch`: `('x86_64' 'aarch64')`
- `depends`: `('qt6-base' 'qt6-svg' 'mpv' 'hicolor-icon-theme')`
- `makedepends`: `('cmake' 'ninja' 'pkgconf')`
- Standard `build()` invoking CMake with Ninja and `package()` deploying via `DESTDIR`.

---

## Standalone AppImage & AppDir

AppImage bundles all necessary runtime components, Qt plugins, and libraries into a portable executable that runs without installation or root privileges.

### 1. Direct Execution

```bash
# Make executable
chmod +x outPlayer-0.1.1-x86_64.AppImage

# Run directly
./outPlayer-0.1.1-x86_64.AppImage

# Play file directly
./outPlayer-0.1.1-x86_64.AppImage ~/Videos/sample.mkv
```

### 2. Portable AppDir Directory Archive

If you prefer extracting or running from an extracted folder without FUSE:

```bash
# Extract portable tarball
tar -xzf outPlayer-0.1.1-x86_64.AppDir.tar.gz

# Execute via AppRun
./outPlayer-0.1.1-x86_64.AppDir/AppRun ~/Videos/sample.mkv
```

### 3. Automated AppImage & AppDir Build (`build-appimage.sh`)

```bash
# Run automated AppDir/AppImage builder
bash packaging/appimage/build-appimage.sh dist/

# Staged directory: dist/outPlayer-0.1.1-x86_64.AppDir
# Portable tarball: dist/outPlayer-0.1.1-x86_64.AppDir.tar.gz
# Standalone AppImage: dist/outPlayer-0.1.1-x86_64.AppImage (when appimagetool is present)
```

### 4. Custom AppRun Launcher Architecture

The bundle entrypoint (`packaging/appimage/AppRun`) dynamically resolves:
- Relative bundle path via `dirname $(readlink -f $0)`
- `APPDIR` environment definition
- `PATH`: prepends `${APPDIR}/usr/bin`
- `LD_LIBRARY_PATH`: prepends `${APPDIR}/usr/lib` and `${APPDIR}/usr/lib64`
- `QT_PLUGIN_PATH`: points to `${APPDIR}/usr/plugins` (containing platform xcb/wayland plugins, imageformats, and iconengines)
- `XDG_DATA_DIRS`: prepends `${APPDIR}/usr/share` for icon and metainfo resolution.

---

## Desktop Integration Verification

Verify proper system-wide desktop integration after installing outPlayer:

### 1. FreeDesktop Desktop File Validation

```bash
desktop-file-validate /usr/share/applications/org.outplayer.outPlayer.desktop
```
Expected output: No output (exit code 0 indicates success).

### 2. AppStream Metadata Validation

```bash
appstreamcli validate --no-net /usr/share/metainfo/org.outplayer.outPlayer.metainfo.xml
```
Expected output: Validation successful.

### 3. MIME Associations Check

Verify default association for video containers:

```bash
# Query default handler for MP4 video
xdg-mime query default video/mp4
# Expected: org.outplayer.outPlayer.desktop

# Associate outPlayer with video files manually
xdg-mime default org.outplayer.outPlayer.desktop video/mp4 video/x-matroska video/webm video/quicktime
```

### 4. MPRIS2 D-Bus Verification

While outPlayer is running:

```bash
# Query active MPRIS2 service
qdbus org.mpris.MediaPlayer2.outplayer /org/mpris/MediaPlayer2

# Check playback status
qdbus org.mpris.MediaPlayer2.outplayer /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.PlaybackStatus

# Trigger remote play/pause
qdbus org.mpris.MediaPlayer2.outplayer /org/mpris/MediaPlayer2 org.mpris.MediaPlayer2.Player.PlayPause

# Or query using playerctl CLI:
playerctl --player=outplayer status
playerctl --player=outplayer play-pause
```

### 5. Screensaver Inhibition Verification

While media is actively playing:

```bash
# Query active inhibitor cookies
qdbus org.freedesktop.ScreenSaver /ScreenSaver org.freedesktop.ScreenSaver.GetActive
```
outPlayer inhibits the screensaver during active playback and automatically releases the inhibitor when paused or playback stops.

---

## Release Integrity, Checksums & Reproducibility

Every outPlayer release publishes cryptographic checksums and digital signatures.

### 1. Verifying SHA-256 Checksums

```bash
# Download SHA256SUMS file alongside releases
sha256sum -c SHA256SUMS
```

Example `SHA256SUMS` manifest:
```text
68f2c3de9851a798a760ba26c6db24e3a479ff7038cf56094593976ff9d8ff72  outplayer-0.1.0-1.x86_64.rpm
da823d722d36ab2c40c6b12a58b6883ba5381fce727b12d54ad56a16ca31cb92  outplayer_0.1.0-1_amd64.deb
5a9609a5f403980df9b4f738f6b6eb384c5a939021eb3e0b7fc6fca1249ca343  outPlayer-0.1.0-x86_64.AppDir.tar.gz
```

### 2. GPG Signature Verification

Official binaries are signed by the project release key:

```bash
gpg --verify outplayer_0.1.0-1_amd64.deb.sig outplayer_0.1.0-1_amd64.deb
```

### 3. Reproducible Builds

outPlayer builds deterministically when `SOURCE_DATE_EPOCH` is defined:

```bash
export SOURCE_DATE_EPOCH="$(git log -1 --pretty=%ct)"
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

---

## Uninstallation & User Configuration

### 1. Removing Packages

- **Flatpak**:
  ```bash
  flatpak uninstall org.outplayer.outPlayer
  ```

- **Debian / Ubuntu**:
  ```bash
  sudo apt-get remove outplayer
  # Or purge configuration and desktop associations:
  sudo apt-get purge outplayer
  ```

- **Fedora / RHEL**:
  ```bash
  sudo dnf remove outplayer
  ```

- **Arch Linux**:
  ```bash
  sudo pacman -R outplayer
  ```

- **AppImage**:
  Simply delete the `outPlayer-*.AppImage` or `outPlayer-*.AppDir` file/folder.

### 2. User Data and Settings Locations

In accordance with XDG Base Directory standards:
- Configuration: `~/.config/outplayer/outplayer.ini`
- Bookmarks: `~/.config/outplayer/bookmarks.ini`
- Saved Playlists: `~/.local/share/outplayer/playlists/`
- Frame Snapshots: `~/Pictures/outPlayer/` (default)
- Flatpak App Data: `~/.var/app/org.outplayer.outPlayer/`

To completely reset outPlayer to default settings:
```bash
rm -rf ~/.config/outplayer ~/.local/share/outplayer
```
