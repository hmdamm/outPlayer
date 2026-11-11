# Architecture Decision Records (ADR)

This document records the key architectural, technical, and governance decisions made for **outPlayer**.

---

## ADR 001: Project Name, Binary Identity & Branding Neutrality

- **Status:** Accepted
- **Date:** 2026-11-11
- **Deciders:** Project Maintainers

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
- **Deciders:** Project Maintainers

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
