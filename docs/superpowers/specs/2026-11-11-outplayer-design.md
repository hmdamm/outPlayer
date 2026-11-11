# Design Document: outPlayer (Native Linux Media Player)

**Project Name:** outPlayer (`outplayer`)  
**Date:** 2026-11-11  
**Status:** Approved by Builder Check  
**Author:** Lead Engineer & AntiGravity Pair Programmer  

---

## 1. Intent & The "Why"

On desktop Linux (Ubuntu/Debian, Fedora, Arch Linux), media players are largely divided between minimalist frontends (e.g., Celluloid, Clapper) and legacy multi-purpose players (e.g., VLC, SMPlayer). None reproduce the deep workflow efficiency, granular frame/speed/subtitle controls, instant responsive UI, and ubiquitous hotkey ergonomics of Windows Daum PotPlayer.

**outPlayer** fills this void: a native, high-performance Linux media player built with modern C++20, Qt 6, and the `libmpv` render API, delivering complete functional and hotkey parity with PotPlayer 64-bit through a strictly audited clean-room methodology.

---

## 2. Goals, Non-Goals, and Anti-Goals

### 2.1 Goals
- **Clean-room parity**: 1:1 functional, hotkey, context menu, and workflow parity with PotPlayer 64-bit, documented exclusively via black-box observation in `docs/spec/`.
- **Format versatility**: Zero-compromise playback for all video/audio formats, codecs, containers, network streams, and subtitles supported by FFmpeg and `libmpv`.
- **Native performance**: Sub-second startup time, negligible seek latency, hardware acceleration across modern graphics APIs (VA-API, NVDEC, VDPAU, Vulkan).
- **First-class Linux integration**: Dual Wayland and X11 support, MPRIS2 D-Bus interface, XDG Global Shortcuts portal, desktop screensaver/inhibit portal, StatusNotifierItem tray.
- **Universal Linux distribution**: Multi-distro packaging covering Flatpak, RPM (Fedora), DEB (Ubuntu/Debian), PKGBUILD (Arch/AUR), and AppImage.

### 2.2 Non-Goals
- **No Windows/macOS support**: Focus exclusively on modern Linux desktop environments (GNOME, KDE Plasma, XFCE, Sway, Hyprland, etc.).
- **No binary reuse or plugin bridging**: No loading of Windows 32/64-bit DirectShow filters, VapourSynth/AviSynth `.dll`s, or proprietary closed-source codec packs.
- **No invasive cloud telemetry**: Zero phone-home, zero advertising, zero non-user-initiated network requests.

### 2.3 Anti-Goals (What would make this project fail)
- **Copyright or EULA infringement**: Disassembling, decompiling, memory hooking, or copying proprietary assets (icons, skins, logos, strings) from Daum PotPlayer is strictly forbidden.
- **Bloated or sluggish UI**: Sluggish QML-heavy startup or non-responsive UI thread stalls during seeking or media probing.
- **Broken hotkeys**: Diverging from PotPlayer default keybindings or failing to handle edge-case keyboard combinations.

---

## 3. Constraints & Clean-Room Protocols

1. **Clean-Room Enforcement**:
   - **Observer Role**: Operates PotPlayer as an external user on Windows/VM. Records externally visible behaviors, hotkey lists, preferences hierarchies, and layout metrics into `docs/spec/` and maintains `docs/clean-room-log.md`.
   - **Implementer Role**: Implements code solely based on `docs/spec/`, standard Qt 6 documentation, and open-source `libmpv`/FFmpeg APIs.
   - **Forbidden Operations**: Disassembly, decompilation, debugger attach, hooking, memory inspection, string dumping, or copying proprietary binary assets/icons/skins.
2. **Branding Neutrality**:
   - The names "PotPlayer", "Daum", and "Kakao" shall not appear in binary names (`outplayer`), desktop files (`outplayer.desktop`), package IDs (`org.outplayer.outPlayer`), or UI branding strings.
3. **Licensing**:
   - Application codebase licensed under **GPL-3.0-or-later** (compatible with GPL-built libmpv/FFmpeg). Qt 6 linked dynamically under **LGPL-3.0**. Maintain complete attribution in `THIRD_PARTY_NOTICES.md`.

---

## 4. Architectural Approaches & Key Technical Decisions

### 4.1 Approaches Evaluated
- **Approach A (Pure QML/QtQuick)**: Rejected due to heavier cold launch, complex docking of utility dialogs, and non-native desktop integration.
- **Approach B (Hybrid Qt 6 Widgets + libmpv Render API)**: **Selected**. Instant cold startup, deterministic layout rendering, mature native menu/shortcut system, robust multi-window docking (playlist, control box, equalizer), direct low-overhead integration with `libmpv` via `QOpenGLWidget` / Vulkan.
- **Approach C (Electron / Web UI)**: Rejected due to high resource consumption and seek latency.

### 4.2 Window Management on Wayland & X11 (Builder Call)
- PotPlayer default appearance uses a custom dark frameless window with custom title bar and min/max/close buttons.
- On Wayland and X11, window dragging and edge snapping are handled natively using Qt 6's `QWindow::startSystemMove()` and `QWindow::startSystemResize()`.
- An option in Preferences > General > Skin allows switching to native system window decorations (SSD) for users on tiling or strict Wayland compositors.

### 4.3 Preferences Dialog & Linux Deviations (Builder Call)
- The Preferences dialog (F5) will faithfully mirror PotPlayer's hierarchical category tree.
- Settings that directly map to `libmpv`, FFmpeg, audio backends (PipeWire/PulseAudio/ALSA), video renderers (VA-API/NVDEC/Vulkan), and Linux hotkeys are fully active.
- Settings that rely on Windows-specific technologies (DirectShow filters, D3D9/D3D11 exclusive modes, WASAPI exclusive) will be cleanly disabled or documented in `docs/spec/linux-deviations.md` with their nearest Linux equivalent.

---

## 5. System Architecture & Module Decomposition

```
+-------------------------------------------------------------+
|                        outPlayer UI                         |
|  [Frameless Window] [Menu Bar] [OSD Overlay] [Control Bar]  |
|  [Playlist Dock (F6)]   [Control Panel (F7)]   [Prefs (F5)] |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                 Command & Action Registry                   |
|   (Dispatches named commands, hotkey routing, MPRIS, CLI)   |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|                 IPlaybackEngine Interface                   |
+-------------------------------------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|              MpvPlaybackEngine (libmpv C++20)               |
|  - Asynchronous event pump (`mpv_wait_event`)               |
|  - Property observation (`mpv_observe_property`)            |
|  - Hardware decoding selector (vaapi, nvdec, vdpau, vulkan) |
+------------------------------+------------------------------+
                               |
                               v
+-------------------------------------------------------------+
|               Video Render Surface (OpenGL / Vulkan)        |
|  - `mpv_render_context` mapped to QOpenGLWidget             |
+-------------------------------------------------------------+
```

### 5.1 Modules Breakdown
1. **`src/engine/` (`IPlaybackEngine`, `MpvEngine`)**:
   - Zero UI dependencies.
   - Thin, idiomatic C++20 RAII wrapper around `mpv_handle*` and `mpv_render_context*`.
   - Dispatches player state updates (time, duration, pause state, tracks, volume) via Qt signals.
2. **`src/core/` (State, Settings, Command Bus)**:
   - Central command registry (`CommandRegistry`): Every action has an ID, display name, category, default keybinding, and invocation slot.
   - Settings management (`SettingsManager`): Hierarchical configuration engine with `.ini` import/export compatibility.
3. **`src/ui/` (Presentation & Views)**:
   - `MainWindow`: Frameless or system-decorated player window with custom autohide control bar and mouse gesture tracker.
   - `VideoWidget`: Hardware video surface utilizing Qt OpenGL context with `mpv_render_context_render`.
   - `ControlBar`: Time slider, play/pause, stop, frame step, seek buttons, volume slider, track selectors.
   - `OsdRenderer`: High-resolution on-screen display for hotkey feedbacks, timecode, and playback metrics.
   - `PlaylistView`: High-performance `QAbstractItemModel` supporting tens of thousands of items, fuzzy search, and drag-and-drop.
   - `PreferencesDialog`: Multi-tab preference tree reproducing PotPlayer's option structure.
4. **`src/hotkeys/` (Input & Ergonomics)**:
   - Built-in hotkey database loaded from `docs/spec/hotkeys.csv`.
   - Rebindable keymapper supporting multi-key chords and global shortcut triggers.
5. **`src/integration/` (Desktop & System)**:
   - MPRIS2 D-Bus media player interface.
   - XDG Desktop Portals (Screensaver Inhibit, Global Shortcuts, File Chooser).
   - System tray icon via FreeDesktop StatusNotifierItem.

---

## 6. Execution Roadmap & Phase Gates

### Phase 0: Development Environment & "Hello Player" (Current Phase)
- **Scope**:
  1. Initialize repository scaffold: `src/engine/`, `src/core/`, `src/ui/`, `src/hotkeys/`, `docs/spec/`, `tests/`, `packaging/`.
  2. Create governance logs: `TASKS.md`, `docs/decisions.md`, `docs/clean-room-log.md`, `THIRD_PARTY_NOTICES.md`.
  3. Author root `CMakeLists.txt` targeting C++20, Qt 6 Widgets, and `libmpv`.
  4. Build minimal "Hello Player" spike executable verifying `mpv_render_context` OpenGL rendering inside a Qt 6 window.
  5. Setup unit testing harness (`ctest`) with initial engine tests.
- **Gate**: Working `outplayer` binary playing video on Linux/WSL and unit test suite passing.

### Phase 1: Black-Box Feature Inventory (Observer Role)
- **Scope**:
  - Run black-box inspection of installed PotPlayer 64-bit on Windows host.
  - Generate `docs/spec/features.csv`: Full tree of menus, context menus, F5 preferences, F6 playlist, F7 control panel.
  - Generate `docs/spec/hotkeys.csv`: All default hotkeys and unassigned actions.
  - Generate `docs/spec/ui-layout.md`: Layout measurements, color palette, OSD specs.
  - Generate `docs/spec/formats.md`, `docs/spec/cli.md`, and `docs/spec/linux-deviations.md`.
- **Gate**: Coverage report verifying every visible UI control has an entry in `features.csv` with evidence citations in `clean-room-log.md`.

### Phase 2: Core Architecture & Command Engine
- **Scope**:
  - Define `IPlaybackEngine` abstract interface and implement production `MpvEngine`.
  - Build `CommandRegistry` loading commands and bindings from `hotkeys.csv`.
  - Establish `SettingsManager` with `.ini` file serialization.
- **Gate**: Architecture and module dependencies verified; unit tests passing for command routing and settings.

### Phase 3: Vertical Slices Implementation
- **Scope** (Executed in strict slice order with TDD):
  1. Playback core & precise seeking.
  2. Control bar & OSD engine.
  3. Hotkey router & mouse gestures.
  4. Playlist model/view (F6) with drag-and-drop.
  5. Audio subsystem (EQ, normalization, track selection, delay).
  6. Video renderer controls (aspect ratio, rotate, zoom, shaders, deinterlace).
  7. Subtitle engine (libass styling, online search, sync, position).
  8. Bookmarks & screen capture (PNG/JPG/GIF).
  9. Preferences dialog (F5).
  10. Streams, DVD/Blu-ray, and V4L2 devices.
  11. Theming, skins, and original SVG icons.
  12. Linux desktop integration (MPRIS2, StatusNotifierItem, portal inhibit).
- **Gate**: Each vertical slice backed by automated tests before marking `done` in `features.csv`.

### Phase 4: Parity Hardening & Verification
- **Scope**:
  - Hotkey parity suite: Automated QTest asserting every row in `hotkeys.csv`.
  - UI visual comparison: Offscreen xvfb layout diffs against `ui-layout.md`.
  - Playback format matrix: Automated playback tests across sample media generated via ffmpeg.
  - Performance profiling: Startup time < 500ms, seek latency, memory ceiling.
- **Gate**: 100% hotkey test pass rate (or documented deviation), 0 memory leaks in ASan/UBSan.

### Phase 5: Packaging & Distribution
- **Scope**: Flatpak (Flathub manifest), RPM (Fedora), DEB (Ubuntu/Debian), Arch PKGBUILD, AppImage.
- **Gate**: Clean build and installation smoke-tests in all three container environments.

---

## 7. Approval & Next Steps

Following this design document's review:
1. Proceed to invoke `superpowers:writing-plans` to author the concrete Phase 0 implementation plan.
2. Execute Phase 0 tasks in accordance with TDD and verification guidelines.
