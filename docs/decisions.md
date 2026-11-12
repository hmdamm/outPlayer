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

---

## ADR 003: Phase 0 Hello Player Gate Clearance & Video Surface Verification

- **Status:** Accepted
- **Date:** 2026-11-11
- **Deciders:** Project Maintainers

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
   - Linux environment: Debian-based Linux (x86_64, Linux kernel 6.6+).
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

---

## ADR 004: Phase 1 Specification Baseline & Clean-Room Feature Inventory

- **Status:** Accepted
- **Date:** 2026-11-12
- **Deciders:** Project Maintainers

### Context
Phase 1 establishes the complete, machine-verifiable black-box feature and UI/hotkey specification inventory for **outPlayer** using an audited clean-room methodology (Observer role). The objective of this phase is to produce an unambiguous technical baseline capturing 100% of visible controls, keybindings, configuration options, formats, CLI switches, and layout metrics from the reference player prior to implementing Core Architecture & Command Engine components in Phase 2.

To clear the Phase 1 Gate, the following conditions must be empirically met:
1. Complete menu and feature hierarchy inventory cataloged in machine-verifiable format.
2. Complete default hotkey map and mouse gesture matrix documented with canonical command identifiers.
3. Complete preferences hierarchy and technical deviations ledger establishing native Linux adaptations for Windows-specific multimedia subsystems.
4. Complete UI layout metrics, color palette tokens, typography rules, and auxiliary window (F6 playlist, F7 control panel) docking behaviors.
5. Supported media formats, codec decoders, network streaming protocols, and CLI switch compatibility documented.
6. Clean-room audit compliance verified via automated testing, asserting zero proprietary branding ("PotPlayer", "Daum", "Kakao") in implementation source code, symbols, target binary paths, or package IDs.
7. 100% automated test pass rate for all specification assertions and existing C++ test suites.

### Decision
We formally accept the Phase 1 Specification Baseline and declare the Phase 1 Gate cleared based on the following verified deliverables and test evidence:

1. **Exhaustive Specification Inventory Deliverables:**
   - **`docs/spec/features.csv` (345 rows):**
     - 258 menu and context menu items across 12 functional areas (`Open`, `Play`, `Subtitles`, `Video`, `Audio`, `Filters`, `Bookmarks`, `Capture`, `Skins`, `Aspect Ratio`, `Window Size`, `Help`) with menu paths, accelerator keys, default values, and observed behaviors.
     - 87 Preferences settings rows across 7 primary categories (`General`, `Playback`, `Video`, `Audio`, `Subtitles`, `Filter`, `Association`) and 21 subcategories.
     - 100% of rows contain non-empty attributes and valid clean-room audit cross-references (`CR-004`, `CR-006`).
   - **`docs/spec/hotkeys.csv` (253 rows):**
     - 253 default keybindings, mouse click actions (`LeftClick`, `LeftDblClick`, `MiddleClick`), and mouse wheel gestures (`WheelUp`, `WheelDown`).
     - Covers playback navigation, A-B repeat, volume/equalizer controls, subtitle sync/placement, frame steps, speed adjustments, and dialog shortcuts mapped to clean, idiomatic `CMD_*` identifiers.
   - **`docs/spec/formats.md` (SPEC-FORMATS-001):**
     - Comprehensive support matrix covering 10 major container formats (MKV, MP4, AVI, WebM, MPEG-TS, FLV, MOV, OGG, FLAC, MP3), 7 video codecs (H.264, HEVC, AV1, VP9, MPEG-2, ProRes, VC-1), 8 audio codecs (AAC, AC-3, E-AC-3, TrueHD, DTS, FLAC, Opus, MP3), 5 subtitle formats (ASS/SSA, SRT, WebVTT, PGS, VobSub), and 6 streaming protocols (HTTP, HTTPS, HLS, DASH, RTSP, RTMP).
     - Hardware decoding acceleration defined via Linux VA-API and NVDEC mapped through `libmpv`/FFmpeg.
   - **`docs/spec/cli.md` (SPEC-CLI-001):**
     - Complete mapping of 28 command-line switches and parameters to GNU long options (`--file-dialog`, `--url-dialog`, `--folder-dialog`, `--seek`, `--sub`, `--audio`, `--add`, `--new-window`, etc.) alongside drop-in compatibility aliases (`/filedlg`, `/seek`, `/sub`, `/aud`, `/volume`, `config=`).
     - Documented stdin pipe handling (`outplayer -`) and wildcard expansion.
   - **`docs/spec/linux-deviations.md` (SPEC-LINUX-001):**
     - Detailed technical architectural ledger documenting 9 fundamental Linux platform adaptations:
       - DirectShow/Direct3D $\to$ `libmpv` `mpv_render_context` with OpenGL / Vulkan and VA-API / NVDEC.
       - DirectSound/WASAPI $\to$ PipeWire, PulseAudio, and ALSA with automatic fallbacks.
       - Exclusive Fullscreen $\to$ Wayland borderless fullscreen (`xdg_toplevel.set_fullscreen()`).
       - Windows Registry $\to$ FreeDesktop XDG Base Directory Specification with `QSettings(QSettings::IniFormat)`.
       - Win32 `RegisterHotKey` $\to$ XDG Desktop Portal (`org.freedesktop.portal.GlobalShortcuts`) and X11 `XGrabKey`.
       - DirectShow Capture $\to$ Video4Linux2 (`/dev/video*`) via `libmpv` `avdevice`.
   - **`docs/spec/ui-layout.md` (SPEC-UI-001):**
     - Deterministic geometry metrics: 320x180 minimum window size, 30px custom titlebar height, 40px bottom control bar, 14px time slider height with 4px groove, 24x24px / 28x28px control buttons.
     - Dark theme color tokens: Background `#1b1b1b` / `#1e1e1e`, Control bar `#252525`, Slider groove `#383838`, Slider progress `#e09b1f` / `#ffaa00`, Text primary `#ffffff`, Text secondary `#aaaaaa`, Accent highlight `#d48800`.
     - Layout, typography, and docking specifications for Main Window, OSD, Playlist Dock (`F6`), and Control Panel (`F7`).

2. **Automated Verification Suite:**
   - `python tests/test_phase1_specs.py`: 100% of test assertions pass across Tasks 1 through 6.
   - Clean-Room Guard (`test_clean_room_guard`): Zero violations detected in `src/` or `docs/spec/`.
   - Linux C++ Test Suite (`wsl xvfb-run -a ctest --test-dir build --output-on-failure`): 3/3 tests pass cleanly (`test_dummy`, `test_mpv_engine`, `test_video_widget`).

3. **Clean-Room Protocol & Legal Compliance:**
   - All observation sessions documented under audit log entries `CR-001` through `CR-008` in `docs/clean-room-log.md`.
   - Complete observer-implementer role separation preserved; zero disassembly, decompilation, or proprietary asset reuse.
   - Target application identity strictly neutral: binary name `outplayer`, package identifier `org.outplayer.outPlayer`.

### Consequences
- **Positive:**
  - Complete, frozen functional and behavioral specification contract ready to guide Phase 2 (Core Architecture & Command Engine) and Phase 3 (Vertical Slices Implementation).
  - High degree of implementation confidence: developers do not need access to reference binaries and can build directly against machine-readable CSVs and markdown specifications.
  - Test-Driven Development (TDD) harness established for testing future command routing and UI components.
- **Negative:**
  - Large specification volume (345 features, 253 hotkeys) requires disciplined prioritization during vertical slice execution in Phase 3.

---

## ADR 005: Core Architecture, Command Engine Foundation & Input Routing

- **Status:** Accepted
- **Date:** 2026-11-12
- **Deciders:** Project Maintainers

### Context
Following the completion of the Phase 1 Specification Baseline (ADR 004), Phase 2 transitions **outPlayer** from a minimal video-rendering prototype into a modular, production-ready multimedia architecture.

To support the 253 cataloged hotkeys, 87 preferences, diverse media formats, and complex multi-window interfaces planned in Phase 3, the architecture must solve five core design challenges:
1. **Central Command Bus vs. Direct UI Couplings:** Direct connections between Qt widget signals and media player slots create high coupling, preventing alternate input sources (CLI, global shortcuts, D-Bus/MPRIS2) from executing actions identically.
2. **Rebindable Input Routing & Mouse Gestures:** Keybindings and mouse gestures (e.g. wheel volume/seeking, left-click pause, double-click fullscreen) require centralized routing, conflict prevention, and runtime rebinding.
3. **Configuration Persistence & Linux Standards:** The application requires a fast, reliable configuration engine adhering to the FreeDesktop XDG Base Directory specification without Windows Registry paradigms.
4. **Non-Blocking Playback Engine Expansion:** The playback engine must expose track enumeration, chapter navigation, aspect ratio, volume/speed controls, and asynchronous command dispatch without locking the main Qt GUI thread.
5. **Strict Component Isolation & Headless Testability:** Core business logic and the playback engine must remain 100% free of UI/Widget dependencies (`QtWidgets`, `QtGui`, `QPainter`) to ensure automated headless testing.

### Decision
We formally accept the Phase 2 Core Architecture and declare the Phase 2 Gate cleared based on the following architectural implementations and empirical verification evidence:

1. **Decoupled Architecture & CMake Target Partitioning:**
   - **`outplayer_core`:** Static library containing `CommandRegistry`, `CommandId`, `SettingsManager`, `IPlaybackEngine`, and `MpvEngine`. Depends solely on `Qt6::Core` and `PkgConfig::MPV`. Zero GUI/Widget headers are permitted, ensuring headless execution without an X11/Wayland display server.
   - **`outplayer_input`:** Static library containing `InputManager`. Links `outplayer_core` and `Qt6::Widgets` to map keyboard shortcuts (`QShortcut`) and mouse event filters to canonical `CommandId` actions.
   - **`outplayer_ui`:** Static library containing `VideoWidget` (OpenGL rendering via `mpv_render_context`) and `MainWindow`.
   - **`outplayer`:** Application executable linking the static libraries.

2. **Automated Command Engine (`CommandRegistry` & Codegen):**
   - Implemented `tools/generate_commands.py` to ingest `docs/spec/hotkeys.csv` and generate `src/core/CommandId.h`, defining 253 canonical `enum class CommandId` values and metadata lookup tables.
   - Implemented singleton `CommandRegistry` in `src/core/CommandRegistry.h`/`.cpp` supporting handler registration (`std::function<void(const QVariantMap&)>`), string-to-enum resolution, and parameterized action dispatch (`execute()`).

3. **XDG-Compliant Configuration Subsystem (`SettingsManager`):**
   - Implemented `SettingsManager` in `src/core/SettingsManager.h`/`.cpp` using `QSettings::IniFormat`.
   - Resolves configuration storage according to XDG specifications: `$XDG_CONFIG_HOME/outplayer/outplayer.ini` (defaulting to `~/.config/outplayer/outplayer.ini`), with runtime override support via `init(path)`.
   - Enforces default preference values matching `docs/spec/features.csv`, provides strongly-typed getters/setters, emits reactive `settingChanged` signals, and supports atomic configuration import/export.

4. **Input Routing & Gesture Interception (`InputManager`):**
   - Implemented `InputManager` in `src/input/InputManager.h`/`.cpp`.
   - Manages window-scoped `QShortcut` instances initialized from default hotkey specifications, supporting dynamic runtime rebinding with conflict safety.
   - Implements an event filter (`QObject::eventFilter`) attachable to `VideoWidget` to translate mouse click actions (`LeftClick`, `LeftDblClick`, `MiddleClick`, `RightClick`) and scroll gestures (`WheelUp`, `WheelDown`) directly into `CommandRegistry` actions.

5. **Hardened Playback Engine (`IPlaybackEngine` & `MpvEngine`):**
   - Expanded `IPlaybackEngine` and `MpvEngine` to support audio/video/subtitle track selection (`MediaTrack`), chapter navigation (`ChapterInfo`), volume, mute, speed adjustments, and aspect ratio controls.
   - Implemented local property caching for playback state, eliminating synchronous blocking calls to `libmpv` from the Qt GUI thread.
   - Implemented asynchronous command dispatch via `mpv_command_async()`.

6. **Automated Test Pass Rate & Verification Evidence:**
   - 100% of unit and integration tests pass cleanly under `xvfb-run -a ctest`:
     - `test_dummy`: Passed
     - `test_command_registry`: Passed (verifies 253 commands, enum resolution, signal dispatch)
     - `test_settings_manager`: Passed (verifies INI persistence, XDG pathing, typed defaults, export/import)
     - `test_mpv_engine`: Passed (verifies track enumeration, volume/speed/aspect ratio, async commands)
     - `test_video_widget`: Passed (verifies OpenGL surface initialization, FBO presentation)
     - `test_input_manager`: Passed (verifies default shortcuts, runtime rebinding, mouse filter)
   - 100% of specification verification assertions pass under `python3 tests/test_phase1_specs.py`.
   - Scaffold assertions pass under `python3 tests/test_project_scaffold.py`.
   - Clean-room audit compliance: zero proprietary branding violations across all source code and documentation.

### Consequences
- **Positive:**
  - High degree of modularity and maintainability; individual subsystems can be developed and tested in complete isolation.
  - Headless test execution remains fast (< 4 seconds for full test suite) and reliable without physical display hardware.
  - Phase 3 vertical feature slices can connect to the unified `CommandRegistry` and `SettingsManager` immediately without refactoring.
  - Input configuration is fully customizable by end-users.
- **Negative:**
  - Additional indirection through `CommandRegistry` requires registering handlers for all commands.
  - Modifying default keybindings requires re-running `tools/generate_commands.py` to regenerate `CommandId.h`.

---

## ADR 006: Interactive Player UI Layer, Timeline Scrubber & Chrome Autohide

- **Status:** Accepted
- **Date:** 2026-11-12
- **Deciders:** Project Maintainers

### Context
Following the completion of the Phase 2 Core Architecture (ADR 005), Phase 3 transitions **outPlayer** into vertical feature slices, beginning with Phase 3A: the interactive player user interface and core playback controls (Slice 1 & Slice 2).

Prior to this phase, the application possessed a headless command engine, configuration subsystem, and raw `VideoWidget` rendering surface, but lacked:
1. **Interactive Control Bar & Navigation:** Visual buttons for play, pause, stop, frame stepping, volume control, track navigation, and auxiliary panel toggles (F5/F6/F7).
2. **High-Precision Timeline Scrubber:** A dedicated timeline slider supporting smooth scrub interaction, sub-second seeking precision, hover timestamp preview, and visual chapter marker ticks according to `docs/spec/ui-layout.md`.
3. **On-Screen Display (OSD) Overlay:** A non-intrusive, transparent HUD layer to display transient playback state notifications (volume, speed, seek offset, aspect ratio) with smooth fadeout animations.
4. **Cinematic Chrome Autohide:** Automatic concealment of the titlebar and control bar during active video playback after user inactivity, including suppressing the mouse cursor to `Qt::BlankCursor` for an unobstructed viewing experience.
5. **Architectural Decoupling:** Ensuring all UI components interact with the underlying media engine strictly through abstract interfaces (`IPlaybackEngine`) and canonical command dispatches (`CommandRegistry`), preserving zero-UI testability for core playback logic.

### Decision
We formally accept the Phase 3A Interactive Player UI Layer architecture and declare the Phase 3A Gate cleared based on the following architectural implementations and empirical verification evidence:

1. **Composite MainWindow Layout & Layering:**
   - Assembled a frameless desktop window (`Qt::FramelessWindowHint`) adhering to the dark palette tokens (`#1b1b1b` / `#1e1e1e` / `#252525`) defined in `docs/spec/ui-layout.md`.
   - Designed custom titlebar (`CustomTitleBar`, 30px height) providing window dragging, title synchronization, and window control buttons (minimize, maximize/restore, close).
   - Designed bottom control bar (`ControlBar`, 40px fixed height) hosting playback buttons, duration labels (`00:00:00 / 00:00:00`), volume slider, mute toggle, and panel shortcuts.
   - Designed transparent, click-through overlay (`OsdOverlay`) layered directly atop `VideoWidget` with `Qt::WA_TransparentForMouseEvents`, `Qt::WA_TranslucentBackground`, and `Qt::WA_NoSystemBackground`. The overlay renders high-contrast outlined typography and auto-dismisses notifications via fadeout animations.
   - Integrated custom edge resize logic and system move support across Linux Wayland and X11 environments.

2. **2.5s Mouse Inactivity Chrome Autohide & Blank Cursor:**
   - Implemented an inactivity timer (`QTimer`) with a 2,500ms timeout active exclusively during playback (`PlaybackState::Playing`).
   - Attached window-level event filtering across all child surfaces to detect `QEvent::MouseMove`, mouse button presses, and key events. Any detected activity resets the timer and immediately restores chrome visibility and normal cursor (`Qt::ArrowCursor`).
   - Upon timer expiration during active playback, `CustomTitleBar` and `ControlBar` hide and `VideoWidget` sets its viewport cursor to `Qt::BlankCursor`.
   - Pausing, stopping, or loading media immediately disarms the autohide timer, ensuring controls and mouse cursor remain persistently visible when user interaction is required.

3. **High-Precision TimelineSlider & Custom QStyle Painting:**
   - Implemented `TimelineSlider` inheriting `QSlider` with custom painting: 14px total height, 4px track groove (`#383838`), orange progress fill (`#e09b1f`), and a 10px circular thumb handle rendered on hover or active scrubbing.
   - High sub-second precision is achieved by mapping continuous media seconds to a 10,000-step discrete integer range, eliminating coarse 1-second truncation during short video seeks.
   - Implemented mouse hover tracking with a floating time tooltip formatted as `hh:mm:ss`.
   - Implemented chapter marker tick visualization rendered directly from chapter positions provided by `IPlaybackEngine::chapters()`.
   - Implemented direct click-to-seek and drag-to-seek with instant non-blocking asynchronous seek dispatch, suppressing position-update feedback loops during active scrubbing.

4. **Strict Separation of UI Controls from Playback Core via `IPlaybackEngine`:**
   - All interactive UI controls (`ControlBar`, `TimelineSlider`, `OsdOverlay`, `CustomTitleBar`) interact with the playback core exclusively through `IPlaybackEngine` signals and slots or canonical `CommandRegistry` actions.
   - Zero `libmpv` headers or symbols exist within UI widget implementations. `VideoWidget` remains the sole component interfacing with the OpenGL rendering context (`mpv_render_context`).
   - Playback state, track changes, duration, position, volume, and mute states propagate asynchronously via Qt signals, ensuring zero UI thread stalls.

5. **Automated Test Pass Rate & Verification Evidence:**
   - 100% of unit and integration tests pass cleanly under `xvfb-run -a ctest`:
     - `test_dummy`: Passed (0.01s)
     - `test_command_registry`: Passed (0.21s)
     - `test_settings_manager`: Passed (0.25s)
     - `test_mpv_engine`: Passed (0.56s)
     - `test_video_widget`: Passed (0.61s)
     - `test_input_manager`: Passed (0.40s)
     - `test_timeline_slider`: Passed (0.33s) — verifies sub-second precision, hover tooltips, chapter marker ticks, and seek dispatch.
     - `test_control_bar`: Passed (0.30s) — verifies playback button toggles, volume slider/mute sync, time formatting, and panel triggers.
     - `test_osd_overlay`: Passed (0.48s) — verifies transparent overlay geometry, message display, styling, and timer fadeout.
     - `test_main_window_ui`: Passed (13.86s) — comprehensive integration test verifying composite layout, play/pause synchronization, volume scrubbing, timeline seeking, 2.5s chrome autohide, blank cursor activation, and mouse motion chrome restoration.
     - Result: 10/10 passed (100%), 0 failures (Total Test time = 17.27s).
   - 100% specification verification assertions pass under `python3 tests/test_phase1_specs.py`.
   - Project scaffold assertions pass under `python3 tests/test_project_scaffold.py`.
   - End-to-end synthetic video playback (`xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1`) executes to completion with exit code 0.
   - Clean-room audit compliance: zero proprietary branding violations across all source code and documentation.

### Consequences
- **Positive:**
  - Complete, functional, dark-themed player UI providing parity with desktop media player conventions.
  - Smooth, accurate timeline seeking with chapter navigation and hover previews.
  - Immersive full-window and fullscreen viewing experience via automated chrome autohide and blank cursor suppression.
  - Deterministic headless UI verification suite under `xvfb-run` guaranteeing regression prevention without physical displays.
  - Clean foundation for implementing auxiliary dialogs (F6 playlist, F7 control panel, F5 preferences) in subsequent Phase 3 slices.
- **Negative:**
  - Frameless window management requires platform-specific window move/resize event handling for edge cases on diverse Linux Wayland compositors and X11 window managers.



