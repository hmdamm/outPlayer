# outPlayer Task & Roadmap Tracker

This document tracks the roadmap, phase progression, and verification evidence for **outPlayer** across Phase 0 through Phase 5.

---

## Roadmap Overview

| Phase | Description | Status | Target Gate |
|---|---|---|---|
| **Phase 0** | Development Environment & "Hello Player" | Complete | Working `outplayer` binary rendering video via `mpv_render_context` and unit test suite passing. |
| **Phase 1** | Black-Box Feature Inventory (Observer Role) | Not Started | Complete UI/hotkey/format spec inventory in `docs/spec/` with evidence logged in `docs/clean-room-log.md`. |
| **Phase 2** | Core Architecture & Command Engine | Not Started | Decoupled architecture (`IPlaybackEngine`, `CommandRegistry`, `SettingsManager`) with passing unit tests. |
| **Phase 3** | Vertical Slices Implementation | Not Started | 12 vertical feature slices implemented via TDD with 100% test coverage for implemented features. |
| **Phase 4** | Parity Hardening & Verification | Not Started | Automated hotkey parity suite passing, offscreen xvfb visual match, and zero ASan/UBSan issues. |
| **Phase 5** | Packaging & Distribution | Not Started | Flatpak, RPM, DEB, Arch PKGBUILD, and AppImage packages verified in container builds. |

---

## Phase 0: Development Environment & "Hello Player"

**Gate Requirement:** Working `outplayer` binary playing video on Linux/WSL and unit test suite passing cleanly under `ctest` / `xvfb-run`.  
**Phase Gate Status:** **CLEARED / COMPLETE** (ADR 003)

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P0-T1** | Scaffolding, Governance Documents & Clean-Room Log | Done | Implementer | `TASKS.md`, `docs/decisions.md`, `docs/clean-room-log.md`, `THIRD_PARTY_NOTICES.md`, `.gitignore`, `README.md`, `tests/test_project_scaffold.py` | `python tests/test_project_scaffold.py` PASS, commit `772c1e2` |
| **P0-T2** | CMake Build System with Qt 6 & libmpv | Done | Implementer | `CMakeLists.txt`, `src/CMakeLists.txt`, `tests/CMakeLists.txt` | `wsl cmake -B build -S . -G Ninja` succeeds, commit `8617263` |
| **P0-T3** | Core Playback Engine Interface & libmpv Wrapper | Done | Implementer | `src/engine/IPlaybackEngine.h`, `src/engine/MpvEngine.h`, `src/engine/MpvEngine.cpp`, `tests/test_mpv_engine.cpp` | `wsl ctest -R test_mpv_engine` PASS, commit `73a084e` |
| **P0-T4** | VideoWidget (mpv_render_context) & MainWindow Shell | Done | Implementer | `src/ui/VideoWidget.h`, `src/ui/VideoWidget.cpp`, `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp`, `src/main.cpp`, `tests/test_video_widget.cpp` | `wsl xvfb-run -a ctest -R test_video_widget` PASS, commits `2e9a280`..`9470275` |
| **P0-T5** | Phase 0 Verification & Gate Clearance | Done | Implementer | Verification runs, synthetic media playback test, status documentation updates | `wsl xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1` exit code 0; 3/3 tests pass (100%) |

### Phase 0 Gate Clearance Evidence
- **Build Cleanliness:**
  - `wsl cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug` & `wsl cmake --build build`: 0 errors, 0 warnings.
- **Unit & Integration Test Suite (`wsl xvfb-run -a ctest --test-dir build --output-on-failure`):**
  - `test_dummy`: PASSED (0.03s)
  - `test_mpv_engine`: PASSED (0.90s)
  - `test_video_widget`: PASSED (1.36s)
  - Result: 100% tests passed, 0 tests failed out of 3 (Total Test time = 2.43s).
- **End-to-End Synthetic Media Playback (`wsl xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1`):**
  - Exit code: `0`
  - Render Surface: `QOpenGLWidget` + `mpv_render_context` using OpenGL FBO (`rgba16f`, Mesa llvmpipe desktop OpenGL 4.6).
  - Stream lifecycle: Demuxed `lavfi` source (640x360 @ 25fps), initialized `libmpv` video output, painted video frames to widget surface, received EOF, exited cleanly.
- **Governance & Scaffolding Verification:**
  - `python tests/test_project_scaffold.py`: PASSED (governance files, licenses, ADRs, clean-room logs, directories verified).

---

## Phase 1: Black-Box Feature Inventory (Observer Role)

**Gate Requirement:** 100% of visible PotPlayer controls and hotkeys categorized and recorded in `docs/spec/` with clean-room audit citations.

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P1-T1** | Reference Installation & Environment Setup | Done | Observer | Setup Win10/11 PotPlayer 64-bit environment, baseline screenshots | Logged in `docs/clean-room-log.md` (CR-001, CR-002, CR-003) |
| **P1-T2** | Context Menu & Main Menu Hierarchy Inventory | Not Started | Observer | `docs/spec/features.csv` (menu section) | Visual observation verification |
| **P1-T3** | Hotkey Map & Keybinding Matrix | Not Started | Observer | `docs/spec/hotkeys.csv` | Full keyboard inspection against PotPlayer default table |
| **P1-T4** | Preferences (F5) Tree & Linux Deviations | Not Started | Observer | `docs/spec/features.csv` (prefs), `docs/spec/linux-deviations.md` | Category-by-category UI tree audit |
| **P1-T5** | Playlist (F6) & Control Panel (F7) Inventory | Not Started | Observer | `docs/spec/features.csv` (aux windows), `docs/spec/ui-layout.md` | Dimensional & behavior layout metrics |
| **P1-T6** | Media Formats, CLI & Network Protocols | Done | Observer | `docs/spec/formats.md`, `docs/spec/cli.md` | `python tests/test_phase1_specs.py` PASS, commit `a2b611d` |

---

## Phase 2: Core Architecture & Command Engine

**Gate Requirement:** Architecture and module interfaces established; unit tests passing for command registry, state dispatch, and configuration management.

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P2-T1** | Production `IPlaybackEngine` & `MpvEngine` Hardening | Not Started | Implementer | Property observation, asynchronous event loop, hardware decoding configuration | Unit tests passing in `tests/test_mpv_engine.cpp` |
| **P2-T2** | Command Registry & Action Dispatcher | Not Started | Implementer | `src/core/CommandRegistry.h`, `src/core/CommandRegistry.cpp` | Unit tests mapping all commands in `hotkeys.csv` |
| **P2-T3** | Settings Engine & Configuration Persistence | Not Started | Implementer | `src/core/SettingsManager.h`, `src/core/SettingsManager.cpp` | INI configuration serialization unit tests |
| **P2-T4** | Signal Bus & State Synchronization | Not Started | Implementer | `src/core/PlaybackState.h`, state propagation infrastructure | Multi-threaded signal verification tests |

---

## Phase 3: Vertical Slices Implementation

**Gate Requirement:** Each vertical slice implemented using strict TDD, verified with automated unit and integration tests.

| Slice | Description | Status | Target Deliverables | Verification Evidence |
|---|---|---|---|---|
| **Slice 1** | Playback Core & Precise Seeking | Not Started | Exact frame step, relative/absolute seek, speed controls (0.2x - 12x) | Automated seek and speed tests |
| **Slice 2** | Control Bar & OSD Engine | Not Started | Autohiding bottom control bar, timeline slider, high-res OSD overlay | UI interaction tests under xvfb |
| **Slice 3** | Hotkey Router & Mouse Gestures | Not Started | Global/local shortcut listener, configurable mouse clicks/wheel actions | Hotkey mapping test suite |
| **Slice 4** | Playlist Model/View (F6) | Not Started | High-performance QAbstractItemModel, sorting, search, drag & drop | 10k item load & search benchmarks |
| **Slice 5** | Audio Subsystem | Not Started | 10-band equalizer, volume boost, normalization, delay, track selection | Audio filter pipe tests |
| **Slice 6** | Video Renderer Controls | Not Started | Aspect ratio, rotation, zoom/pan, deinterlacing, color adjustments | OpenGL shader parameter tests |
| **Slice 7** | Subtitle Engine | Not Started | ASS/SSA, SRT, WebVTT rendering, subtitle sync adjustments, styling | libass subtitle placement tests |
| **Slice 8** | Bookmarks & Screen Capture | Not Started | Bookmark manager, instant frame capture (PNG/JPG/GIF), burst capture | File generation verification tests |
| **Slice 9** | Preferences Dialog (F5) | Not Started | Full categorical preferences tree dialog, search filter, reset defaults | Preference dialog test harness |
| **Slice 10** | Streams, Optical & V4L2 Devices | Not Started | HLS, DASH, RTSP, YouTube/yt-dlp integration, DVD/Blu-ray, webcam input | URL opening & streaming tests |
| **Slice 11** | Theming, Skins & SVG Vector Icons | Not Started | Custom dark skin, frameless window styling, original vector iconography | Visual rendering test under xvfb |
| **Slice 12** | Linux Desktop Integration | Not Started | MPRIS2 D-Bus interface, XDG inhibit portal, StatusNotifierItem tray | D-Bus compliance test suite |

---

## Phase 4: Parity Hardening & Verification

**Gate Requirement:** 100% parity verification suite passing; zero memory leaks or undefined behavior under ASan/UBSan.

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P4-T1** | Automated Hotkey Parity Test Suite | Not Started | Implementer | `tests/test_hotkey_parity.cpp` | 100% execution coverage against `hotkeys.csv` |
| **P4-T2** | UI Layout & Visual Diff Validation | Not Started | Implementer | Headless offscreen frame comparison against `ui-layout.md` | Layout discrepancy tolerance < 1% |
| **P4-T3** | Comprehensive Playback Format Matrix | Not Started | Implementer | Automated test matrix over synthesized audio/video codec test set | Matrix pass report across all formats |
| **P4-T4** | Performance & Latency Benchmarks | Not Started | Implementer | Startup time (< 500ms), seek latency (< 50ms), memory usage check | Automated benchmark suite output |
| **P4-T5** | Sanitizer & Valgrind Audit | Not Started | Implementer | ASan, UBSan, and Valgrind clean test execution runs | 0 errors, 0 memory leaks |

---

## Phase 5: Packaging & Distribution

**Gate Requirement:** Reproducible, installable packages for all target desktop Linux packaging ecosystems.

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P5-T1** | Flatpak Packaging | Not Started | Implementer | `org.outplayer.outPlayer.json` manifest | `flatpak-builder` clean build & run |
| **P5-T2** | Debian / Ubuntu Packaging (DEB) | Not Started | Implementer | `debian/` packaging rules, control, changelog | `dpkg-buildpackage` & lintian clean |
| **P5-T3** | Fedora / Red Hat Packaging (RPM) | Not Started | Implementer | `outplayer.spec` RPM spec file | `rpmbuild` clean build & rpm install test |
| **P5-T4** | Arch Linux PKGBUILD | Not Started | Implementer | `PKGBUILD` for Arch Linux / AUR | `makepkg` execution in Arch container |
| **P5-T5** | AppImage Standalone Bundle | Not Started | Implementer | `AppImageKit` recipe and linuxdeployqt script | Standalone execution on baseline distro |
| **P5-T6** | Release Documentation & Distribution Portal | Not Started | Implementer | Installation guides, changelog, SHA256 checksums | Release checklist verification |
