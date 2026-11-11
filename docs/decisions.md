# Architecture Decision Records (ADR)

This document records the key architectural, technical, and governance decisions made for **outPlayer**.

---

## ADR 001: Project Name, Binary Identity & Branding Neutrality

- **Status:** Accepted
- **Date:** 2026-11-11
- **Deciders:** Lead Architect & AntiGravity Pair Programmer

### Context
The goal of this project is to build a modern, high-performance native Linux media player offering complete functional and hotkey parity with Daum PotPlayer 64-bit through a strictly audited clean-room methodology. To maintain complete legal and trademark integrity, the project requires an independent identity that does not infringe on third-party trademarks, confuse end users, or couple the project to proprietary branding.

### Decision
1. **Official Project Name:** `outPlayer`
2. **Binary Name:** `outplayer` (lower case)
3. **Application ID / Reverse-DNS Identifier:** `org.outplayer.outPlayer`
4. **Desktop Entry File:** `outplayer.desktop`
5. **Branding Neutrality Rule:**
   - The trade names "PotPlayer", "Daum", and "Kakao" shall not appear in:
     - Binary names (`outplayer`)
     - Desktop entry files (`outplayer.desktop`)
     - Package identifiers (`org.outplayer.outPlayer`)
     - User-visible branding strings, application titles, or about dialogs
   - Reference to external software is permitted solely in technical specification documents (`docs/spec/`) and clean-room observation logs (`docs/clean-room-log.md`) for auditing and functional parity verification.

### Consequences
- **Positive:** Clear, distinct open-source brand identity; zero trademark friction; unambiguous package naming across Flatpak, Debian, Fedora, and Arch Linux ecosystems.
- **Negative:** Requires deliberate naming in all documentation and code structures to ensure no trademark leaks occur.

---

## ADR 002: Core Technology Stack — C++20, Qt 6 Widgets & libmpv Render API

- **Status:** Accepted
- **Date:** 2026-11-11
- **Deciders:** Lead Architect & AntiGravity Pair Programmer

### Context
The media player must provide:
- Sub-second instant cold startup.
- Negligible seeking latency and zero UI stalls during playback or media property probing.
- Granular, complex multi-window workflows (frameless main window, detachable F6 playlist, F7 control box, hierarchical F5 preferences tree with hundreds of controls).
- Hardware-accelerated video rendering across diverse Linux graphics stacks (VA-API, NVDEC, VDPAU, Vulkan, software rasterization fallback).
- First-class desktop integration (Wayland, X11, MPRIS2, XDG Portals).

Three implementation architectures were evaluated:
1. **Approach A: Pure QML / Qt Quick**
   - High visual flexibility and animations, but higher startup overhead, heavier memory baseline, more complex window management for deeply nested multi-window dialogs (Preferences tree, docking tools), and less idiomatic desktop widget behaviors on traditional Linux window managers.
2. **Approach B: Qt 6 Widgets + `libmpv` Render API (`mpv_render_context`)**
   - Instantaneous startup, low memory footprint, deterministic layout rendering, native support for nested tree controls and complex tab dialogs, straightforward multi-window docking, and direct hardware rendering via `QOpenGLWidget` / Vulkan.
3. **Approach C: Electron / Web Technologies**
   - Rapid UI development, but unacceptable cold start times, high memory consumption, non-trivial IPC overhead, and poor audio/video synchronization guarantees.

### Decision
We select **Approach B: Modern C++20 with Qt 6 Widgets and `libmpv` (`mpv_render_context`)**.

Key components:
- **Language Standard:** C++20 (utilizing concepts, designated initializers, `std::span`, and RAII wrappers).
- **UI Framework:** Qt 6 Widgets (`QtWidgets`, `QtOpenGLWidgets`, `QtGui`, `QtCore`).
- **Media Engine:** `libmpv` (v0.35+) accessed via `mpv/client.h` and the modern render API `mpv/render.h` (`mpv_render_context`).
- **Decoupled Architecture:** Zero UI dependencies inside the playback engine (`src/engine/`), enabling isolated headless testing via `ctest` and `xvfb-run`.
- **Render Surface:** `QOpenGLWidget` integration with `mpv_render_context` using OpenGL context sharing, with future Vulkan support pathway.
- **Window Management:** Frameless window support leveraging `QWindow::startSystemMove()` and `QWindow::startSystemResize()` on Wayland/X11, with an option to revert to native system window decorations (SSD).

### Consequences
- **Positive:**
  - Sub-500ms cold startup on typical desktop hardware.
  - Direct zero-copy video frame presentation via OpenGL framebuffer objects (`QOpenGLFramebufferObject` / `mpv_render_context_render`).
  - Native ergonomics for dense control panels, playlists, and settings trees.
  - Excellent headless testability through synthetic video sources (`lavfi://nullsrc`).
- **Negative:**
  - Custom UI skinning and autohiding control overlays require custom widget painting and event filters rather than declarative QML transitions.

---

## ADR 003: Phase 0 Hello Player Gate Clearance & Video Surface Verification

- **Status:** Accepted
- **Date:** 2026-11-11
- **Deciders:** Lead Architect & AntiGravity Pair Programmer

### Context
Phase 0 establishes the baseline development environment, modern C++20 build pipeline, decoupled playback engine (`IPlaybackEngine` / `MpvEngine`), and a working video presentation surface (`VideoWidget`) integrated with `mpv_render_context` inside `MainWindow`.

Before transitioning into Phase 1 (Black-Box Feature Inventory) and Phase 2 (Core Architecture & Command Engine), formal clearance of the Phase 0 Gate requires empirical verification that:
1. The project builds cleanly with zero compiler warnings or errors on Linux/WSL2.
2. The entire unit and integration test suite executes cleanly under an automated headless display server (`xvfb`).
3. The standalone `outplayer` binary successfully initializes `mpv_render_context`, renders video frames to an OpenGL surface, and handles media lifecycle to completion.
4. Clean-room protocol and licensing constraints are strictly upheld.

### Decision
We formally accept Phase 0 as complete and declare the Phase 0 Gate cleared based on the following verified test evidence:
1. **Toolchain & Build Cleanliness:**
   - Linux environment: Kali Linux on WSL2 (x86_64, Linux kernel 6.6+).
   - Toolchain: GCC 15 (C++20), CMake 3.31.6, Ninja, Qt 6.8.2, libmpv 0.41.0.
   - Build command `cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build` executed with zero errors and zero compiler warnings.
2. **100% Automated Test Pass Rate:**
   - Headless test execution via `xvfb-run -a ctest --test-dir build --output-on-failure`:
     - `test_dummy`: Passed (0.03s)
     - `test_mpv_engine`: Passed (0.90s) — verified playback state transitions (IDLE, LOADING, PLAYING, PAUSED, STOPPED), seeking, volume/mute control, speed control, and asynchronous observer callbacks.
     - `test_video_widget`: Passed (1.36s) — verified `VideoWidget` OpenGL surface initialization, FBO setup, `mpv_render_context` binding, and frame update notification signaling.
     - Result: 3/3 passed (100%), 0 failures.
3. **End-to-End Synthetic Media Playback:**
   - Standalone execution command: `xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1`
   - Verified that `outplayer` created the OpenGL context via Mesa llvmpipe (OpenGL 4.6 Compatibility Profile), allocated FBO format `rgba16f`, decoded synthetic video at 640x360 @ 25fps, presented frames through `mpv_render_context_render()`, reached container EOF, and exited cleanly with return code 0.
4. **Governance & Legal Integrity:**
   - Governance structure verified by `python tests/test_project_scaffold.py`.
   - Code is licensed under GPL-3.0-or-later with LGPL-3.0 dynamic linking to Qt 6.
   - No proprietary binary symbols, headers, or decompiled artifacts are present in the repository.

### Consequences
- **Positive:**
  - Robust, verified foundation for both UI rendering and headless engine automation.
  - Test suite operates reliably in headless environments, enabling CI/CD automation without physical GPU displays.
  - Phase 1 (Observer Role black-box specification inventory) and Phase 2 (Command Registry and Settings architecture) can begin on a proven codebase.
- **Negative:**
  - Continuous testing under headless environments requires `xvfb-run` or headless EGL configuration.
