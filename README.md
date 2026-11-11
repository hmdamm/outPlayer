# outPlayer

A modern, high-performance native Linux media player offering complete functional, layout, and hotkey parity with Daum PotPlayer 64-bit, powered by modern C++20, Qt 6, and `libmpv`.

---

## Overview

**outPlayer** brings the ergonomics, speed, and deep configurability of PotPlayer to desktop Linux without compromise:
- **Clean-Room Parity**: 1:1 functional, hotkey, context menu, and workflow parity documented exclusively via black-box observation.
- **Modern Architecture**: Decoupled C++20 core engine wrapping `libmpv` with Qt 6 Widgets and hardware-accelerated OpenGL rendering via `mpv_render_context`.
- **Format Versatility**: Comprehensive codec, container, network stream, and subtitle support via `libmpv` and FFmpeg.
- **Native Desktop Integration**: Full Wayland and X11 support, MPRIS2 D-Bus remote controls, and XDG Desktop Portals (screensaver inhibit, global shortcuts).
- **Zero Proprietary Assets**: Completely independent, original codebase licensed under GPL-3.0-or-later.

---

## Architecture & Module Structure

```
outplayer/
├── src/
│   ├── engine/       # Headless IPlaybackEngine & MpvEngine (libmpv C++20 wrapper)
│   ├── core/         # Command registry, hotkey routing, and settings persistence
│   ├── ui/           # MainWindow, VideoWidget (OpenGL / mpv_render_context), ControlBar
│   └── main.cpp      # Application entry point and CLI dispatch
├── docs/
│   ├── decisions.md       # Architecture Decision Records (ADR 001, ADR 002)
│   ├── clean-room-log.md  # Clean-room audit trail & observer records
│   └── spec/              # Black-box parity specifications (hotkeys, features, layouts)
├── tests/            # Automated unit and integration test suite (ctest / xvfb)
├── TASKS.md          # Multi-phase roadmap and verification tracking
└── THIRD_PARTY_NOTICES.md # Licensing details & third-party attributions
```

---

## Build Prerequisites

Ensure your Linux distribution or WSL2 environment has the required toolchains installed:

### Ubuntu / Debian / Kali Linux
```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    cmake \
    ninja-build \
    pkg-config \
    qt6-base-dev \
    libqt6openglwidgets6-dev \
    libmpv-dev \
    ffmpeg \
    xvfb
```

### Fedora
```bash
sudo dnf install -y \
    gcc-c++ \
    cmake \
    ninja-build \
    pkgconf-pkg-config \
    qt6-qtbase-devel \
    mpv-libs-devel \
    ffmpeg \
    xorg-x11-server-Xvfb
```

### Arch Linux
```bash
sudo pacman -S --needed \
    base-devel \
    cmake \
    ninja \
    qt6-base \
    mpv \
    ffmpeg \
    xorg-server-xvfb
```

---

## Building and Testing

### 1. Configure with CMake
```bash
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

### 2. Build Targets
```bash
cmake --build build -j$(nproc)
```

### 3. Run Test Suite
```bash
# Standard unit tests
ctest --test-dir build --output-on-failure

# Headless GUI and OpenGL tests via xvfb
xvfb-run -a ctest --test-dir build --output-on-failure
```

### 4. Run outPlayer
```bash
./build/src/outplayer /path/to/media.mkv
```

---

## Governance & Clean-Room Commitment

outPlayer is engineered under strict clean-room separation:
- **No decompilation or disassembly** of proprietary binaries.
- **No proprietary skins, bitmaps, audio, or fonts** in the tree.
- Specifications are generated strictly through external behavioral observation by an observer role and implemented independently by an implementer role.
- Detailed audit logs are maintained in [`docs/clean-room-log.md`](docs/clean-room-log.md).
- Architectural decisions are logged in [`docs/decisions.md`](docs/decisions.md).
- Progress and roadmap gates are tracked in [`TASKS.md`](TASKS.md).

---

## License

outPlayer is free and open-source software licensed under the **GNU General Public License, Version 3 or later (GPL-3.0-or-later)**.
Qt 6 is dynamically linked under the terms of the **GNU Lesser General Public License version 3 (LGPL-3.0)**.
See [`THIRD_PARTY_NOTICES.md`](THIRD_PARTY_NOTICES.md) for full licensing information and third-party notices.
