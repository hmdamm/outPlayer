# outPlayer Task & Roadmap Tracker

This document tracks the roadmap, phase progression, and verification evidence for **outPlayer** across Phase 0 through Phase 5.

---

## Roadmap Overview

| Phase | Description | Status | Target Gate |
|---|---|---|---|
| **Phase 0** | Development Environment & "Hello Player" | Complete | Working `outplayer` binary rendering video via `mpv_render_context` and unit test suite passing. |
| **Phase 1** | Black-Box Feature Inventory (Observer Role) | Complete | Complete UI/hotkey/format spec inventory in `docs/spec/` with evidence logged in `docs/clean-room-log.md`. |
| **Phase 2** | Core Architecture & Command Engine | Complete | Decoupled architecture (`IPlaybackEngine`, `CommandRegistry`, `SettingsManager`, `InputManager`) with passing unit tests. |
| **Phase 3** | Vertical Slices Implementation | In Progress | 12 vertical feature slices implemented via TDD with 100% test coverage for implemented features. |
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
**Phase Gate Status:** **CLEARED / COMPLETE** (ADR 004)

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P1-T1** | Reference Installation & Environment Setup | Done | Observer | Setup Win10/11 reference environment, baseline screenshots | Logged in `docs/clean-room-log.md` (CR-001) |
| **P1-T2** | Context Menu & Main Menu Hierarchy Inventory | Done | Observer | `docs/spec/features.csv` (menu section) | `python tests/test_phase1_specs.py` PASS (258 items), commit `116a356` |
| **P1-T3** | Hotkey Map & Keybinding Matrix | Done | Observer | `docs/spec/hotkeys.csv` | `python tests/test_phase1_specs.py` PASS (253 shortcuts), commit `e264b83` |
| **P1-T4** | Preferences (F5) Tree & Linux Deviations | Done | Observer | `docs/spec/features.csv` (prefs), `docs/spec/linux-deviations.md` | `python tests/test_phase1_specs.py` PASS (87 prefs items, 9 adaptations), commit `50aa00e` |
| **P1-T5** | Playlist (F6) & Control Panel (F7) Inventory | Done | Observer | `docs/spec/features.csv` (aux windows), `docs/spec/ui-layout.md` | `python tests/test_phase1_specs.py` PASS (geometry & palette), commit `f5c8c06` |
| **P1-T6** | Media Formats, CLI & Network Protocols | Done | Observer | `docs/spec/formats.md`, `docs/spec/cli.md` | `python tests/test_phase1_specs.py` PASS, commit `a2b611d` |
| **P1-T7** | Clean-Room Audit Verification & Phase 1 Gate Clearance | Done | Observer / Implementer | `docs/decisions.md` (ADR 004), `docs/clean-room-log.md` (CR-008), clean-room test guard in `tests/test_phase1_specs.py` | `python tests/test_phase1_specs.py` PASS (100% assertions, 0 clean-room violations) |

### Phase 1 Gate Clearance Evidence
- **Complete Specification Deliverables:**
  - `docs/spec/features.csv`: 345 cataloged feature specifications (258 menu actions across 12 functional areas + 87 preference settings across 7 categories) with default values, observed behaviors, and clean-room evidence refs (`CR-004`, `CR-006`).
  - `docs/spec/hotkeys.csv`: 253 keybinding and mouse gesture mappings with canonical shortcuts, modifiers, and clean `CMD_*` command IDs (`CR-005`).
  - `docs/spec/formats.md`: Comprehensive media format and streaming protocol support matrix across containers, video codecs, audio codecs, subtitles, and hardware acceleration with `libmpv`/FFmpeg mappings (`CR-003`).
  - `docs/spec/cli.md`: Full CLI switch mapping (28 switches and arguments) from reference syntax to `outplayer` GNU long options and compatibility aliases (`CR-002`).
  - `docs/spec/linux-deviations.md`: Complete architectural ledger documenting 9 subsystem adaptations (Wayland borderless, PipeWire/ALSA, XDG base directories, XDG Desktop Portals) with technical rationales (`CR-006`).
  - `docs/spec/ui-layout.md`: Pixel-accurate layout metrics (320x180 min window, 30px titlebar, 40px control bar, 14px/4px slider), dark theme color tokens, typography, OSD rules, and auxiliary window (F6 playlist, F7 control panel) docking behavior (`CR-007`).
- **Automated Specification & Clean-Room Verification Suite (`python tests/test_phase1_specs.py`):**
  - All Phase 1 spec test suites pass (100% assertions satisfied).
  - Clean-room branding guard (`test_clean_room_guard`): zero forbidden proprietary branding ("PotPlayer", "Daum", "Kakao") in `src/`, code symbols, binary paths, or package IDs.
- **Linux/WSL Test Suite (`wsl xvfb-run -a ctest --test-dir build --output-on-failure`):**
  - 3/3 tests pass (100%), 0 failures.
- **Clean-Room Audit Compliance:**
  - Complete observation entries CR-001 through CR-008 logged in `docs/clean-room-log.md`.

---

## Phase 2: Core Architecture & Command Engine

**Gate Requirement:** Architecture and module interfaces established; unit tests passing for command registry, state dispatch, and configuration management.  
**Phase Gate Status:** **CLEARED / COMPLETE** (ADR 005)

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P2-T1** | Command Codegen & CommandRegistry Core | Done | Implementer | `tools/generate_commands.py`, `src/core/CommandId.h`, `src/core/CommandRegistry.h`, `src/core/CommandRegistry.cpp`, `tests/test_command_registry.cpp` | `ctest -R test_command_registry` PASS (253 commands verified), commits `61196f4`, `4b9e5af` |
| **P2-T2** | Settings Engine & Configuration Persistence | Done | Implementer | `src/core/SettingsManager.h`, `src/core/SettingsManager.cpp`, `tests/test_settings_manager.cpp` | `ctest -R test_settings_manager` PASS (INI persistence, XDG path, typed getters), commits `4645159`, `98bfec4` |
| **P2-T3** | InputManager (Keymap Router & Mouse Gestures) | Done | Implementer | `src/input/InputManager.h`, `src/input/InputManager.cpp`, `tests/test_input_manager.cpp` | `xvfb-run -a ctest -R test_input_manager` PASS (QShortcuts, rebinding, mouse filter), commits `2963ada`, `6276dfe` |
| **P2-T4** | Playback Engine Track & Property Expansion | Done | Implementer | `src/engine/IPlaybackEngine.h`, `src/engine/MpvEngine.h`, `src/engine/MpvEngine.cpp`, `tests/test_mpv_engine.cpp` | `xvfb-run -a ctest -R test_mpv_engine` PASS (tracks, chapters, volume, speed, ar), commits `1dc23b8`, `d3b680c` |
| **P2-T5** | Architecture Documentation & Phase 2 Gate Review | Done | Implementer | `docs/architecture.md`, `docs/decisions.md` (ADR 005), `TASKS.md` | 100% tests pass (6/6 ctest, python specs, scaffold verification) |

### Phase 2 Gate Clearance Evidence
- **Build Cleanliness:**
  - `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`: 0 errors, 0 warnings.
- **Unit & Integration Test Suite (`wsl xvfb-run -a ctest --test-dir build --output-on-failure`):**
  - `test_dummy`: PASSED (0.01s)
  - `test_command_registry`: PASSED (0.49s) — verifies 253 canonical command registrations, string-to-enum resolution, info lookups, and callback dispatch.
  - `test_settings_manager`: PASSED (0.29s) — verifies INI persistence, XDG directory resolution, typed accessors, reactive `settingChanged` signals, and file export/import.
  - `test_mpv_engine`: PASSED (0.69s) — verifies media lifecycle, track enumeration (audio/video/sub), chapters, volume/mute/speed/aspect ratio controls, and non-blocking property caching.
  - `test_video_widget`: PASSED (1.34s) — verifies `QOpenGLWidget` surface, FBO format allocation, and `mpv_render_context` frame presentation.
  - `test_input_manager`: PASSED (0.64s) — verifies default `QShortcut` mapping, dynamic shortcut rebinding, and mouse event filtering (`MouseButtonPress`, `MouseButtonDblClick`, `Wheel`).
  - Result: 6/6 passed (100%), 0 failures (Total Test time = 3.68s).
- **Specification & Scaffold Verification Suite:**
  - `python3 tests/test_phase1_specs.py`: PASSED (all Phase 1 spec assertions satisfied, 0 clean-room branding violations).
  - `python3 tests/test_project_scaffold.py`: PASSED (directory structure, ADRs, clean-room logs, and governance verified).
- **Architectural Integrity:**
  - Complete architectural documentation authored in `docs/architecture.md` (component graph, command flow sequence, dependency boundary rules, XDG persistence model, and threading/concurrency model).
  - ADR 005 formally approved and recorded in `docs/decisions.md`.
  - Zero UI dependencies in `src/core/` and `src/engine/` strictly enforced by CMake targets.

---

## Phase 3: Vertical Slices Implementation

**Gate Requirement:** Each vertical slice implemented using strict TDD, verified with automated unit and integration tests.  
**Phase 3A Status (Interactive Player UI & Core Controls):** **CLEARED / COMPLETE** (ADR 006)  
**Phase 3B Status (Navigation & Media Management: Slices 3, 4, 5):** **CLEARED / COMPLETE** (ADR 007)

### Phase 3A Tasks: Interactive Player UI & Core Controls (Slices 1 & 2)

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P3A-T1** | TimelineSlider with Scrubbing & Chapter Markers | Done | Implementer | `src/ui/TimelineSlider.h`, `src/ui/TimelineSlider.cpp`, `tests/test_timeline_slider.cpp` | `ctest -R test_timeline_slider` PASS (10k scale, hover preview, chapter ticks), commits `b69621b`, `451b274` |
| **P3A-T2** | ControlBar with Navigation & Volume Controls | Done | Implementer | `src/ui/ControlBar.h`, `src/ui/ControlBar.cpp`, `tests/test_control_bar.cpp` | `ctest -R test_control_bar` PASS (button states, volume slider, time labels), commits `2d7c6a3`, `9f29b24` |
| **P3A-T3** | OsdOverlay with High-Contrast Typography | Done | Implementer | `src/ui/OsdOverlay.h`, `src/ui/OsdOverlay.cpp`, `tests/test_osd_overlay.cpp` | `ctest -R test_osd_overlay` PASS (click-through overlay, text outline, fadeout), commits `7b2157b`, `fecf0e6` |
| **P3A-T4** | MainWindow Assembly & 2.5s Chrome Autohide | Done | Implementer | `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp`, `tests/test_main_window_ui.cpp` | `xvfb-run -a ctest -R test_main_window_ui` PASS (autohide timer, blank cursor, motion restore), commits `d32cbdc`, `ef31ce1` |
| **P3A-T5** | Phase 3A Gate Review & Integration Verification | Done | Implementer | `docs/decisions.md` (ADR 006), `TASKS.md`, verification test runs | 100% test pass rate (10/10 ctest, specs, synthetic playback test-run) |

### Phase 3A Gate Clearance Evidence
- **Build Cleanliness:**
  - `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`: 0 errors, 0 warnings.
- **Unit & Integration Test Suite (`wsl xvfb-run -a ctest --test-dir build --output-on-failure`):**
  - `test_dummy`: PASSED (0.01s)
  - `test_command_registry`: PASSED (0.21s)
  - `test_settings_manager`: PASSED (0.25s)
  - `test_mpv_engine`: PASSED (0.56s)
  - `test_video_widget`: PASSED (0.61s)
  - `test_input_manager`: PASSED (0.40s)
  - `test_timeline_slider`: PASSED (0.33s)
  - `test_control_bar`: PASSED (0.30s)
  - `test_osd_overlay`: PASSED (0.48s)
  - `test_main_window_ui`: PASSED (13.86s) — verifies composite layout, play/pause synchronization, volume slider/mute sync, timeline seeking, 2.5s chrome autohide, blank cursor suppression, and mouse motion chrome restoration.
  - Result: 10/10 passed (100%), 0 failures (Total Test time = 17.27s).
- **End-to-End Synthetic Media Playback (`wsl xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1`):**
  - Exit code: `0`
  - Render Surface: `QOpenGLWidget` + `mpv_render_context` using OpenGL FBO (`rgba16f`, Mesa llvmpipe desktop OpenGL 4.6).
  - Stream lifecycle: Demuxed `lavfi` source (640x360 @ 25fps), presented frames through `mpv_render_context_render()`, reached container EOF, and exited cleanly.
- **Specification & Scaffold Verification Suite:**
  - `python3 tests/test_phase1_specs.py`: PASSED (all Phase 1 spec assertions satisfied, 0 clean-room branding violations).
  - `python3 tests/test_project_scaffold.py`: PASSED (scaffolding and governance verified).
- **Architectural Integrity:**
  - ADR 006 formally approved and recorded in `docs/decisions.md`.
  - Zero UI dependencies in `src/core/` and `src/engine/` maintained.

### Phase 3B Tasks: Navigation & Media Management (Slices 3, 4, 5)

| Task ID | Description | Status | Assignee / Role | Target Deliverables | Verification Evidence |
|---|---|---|---|---|---|
| **P3B-T1** | Context Menu Architecture & Action Builder (`ContextMenuManager`) | Done | Implementer | `src/ui/ContextMenuManager.h`, `src/ui/ContextMenuManager.cpp`, `tests/test_context_menu.cpp` | `ctest -R test_context_menu` PASS (1:1 menu structure, action counts, shortcuts, commands), commit `4b33e5f` |
| **P3B-T2** | Media Open Dialogs & Streaming URL Controller | Done | Implementer | `src/ui/OpenUrlDialog.h`, `src/ui/OpenUrlDialog.cpp`, `src/ui/MediaOpenController.h`, `src/ui/MediaOpenController.cpp`, `tests/test_media_open_dialogs.cpp` | `ctest -R test_media_open_dialogs` PASS (format checks, recursive scanner, URL history), commit `a383270` |
| **P3B-T3** | Playlist Model & Queue Management (`PlaylistModel`) | Done | Implementer | `src/playlist/PlaylistItem.h`, `src/playlist/PlaylistModel.h`, `src/playlist/PlaylistModel.cpp`, `tests/test_playlist_model.cpp` | `ctest -R test_playlist_model` PASS (queue navigation, repeat/shuffle, 10k items benchmark), commit `cddc446` |
| **P3B-T4** | Playlist Dock Widget & View (`PlaylistDock`) | Done | Implementer | `src/ui/PlaylistDock.h`, `src/ui/PlaylistDock.cpp`, `tests/test_playlist_dock.cpp` | `ctest -R test_playlist_dock` PASS (dark view, real-time search filter proxy, dark delegate, action toolbar), commit `e47aede` |
| **P3B-T5** | MainWindow Navigation Integration (ContextMenu, Playlist Dock, Drag-and-Drop, Auto-Advance) | Done | Implementer | `src/ui/MainWindow.h`, `src/ui/MainWindow.cpp`, `tests/test_main_window_navigation.cpp` | `ctest -R test_main_window_navigation` PASS (F6/PL dock toggle, right-click menu, drag & drop ingestion, EOF track auto-advance), commit `d75dd4f` |
| **P3B-T6** | Phase 3B Gate Review, Integration Verification & ADR 007 | Done | Implementer | `docs/decisions.md` (ADR 007), `TASKS.md`, verification test runs | 100% test pass rate (15/15 ctest, python specs, synthetic playback test-run) |

### Phase 3B Gate Clearance Evidence
- **Build Cleanliness:**
  - `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`: 0 errors, 0 warnings.
- **Unit & Integration Test Suite (`wsl xvfb-run -a ctest --test-dir build --output-on-failure`):**
  - `test_dummy`: PASSED (0.01s)
  - `test_command_registry`: PASSED (0.20s)
  - `test_settings_manager`: PASSED (0.23s)
  - `test_mpv_engine`: PASSED (0.53s)
  - `test_video_widget`: PASSED (0.49s)
  - `test_input_manager`: PASSED (0.27s)
  - `test_timeline_slider`: PASSED (0.25s)
  - `test_control_bar`: PASSED (0.26s)
  - `test_osd_overlay`: PASSED (0.42s)
  - `test_main_window_ui`: PASSED (7.16s)
  - `test_context_menu`: PASSED (0.20s) — verifies 1:1 menu structure, action counts, shortcut bindings, and command dispatch.
  - `test_media_open_dialogs`: PASSED (0.24s) — verifies format filters, recursive directory scanner, URL validation, and history persistence.
  - `test_playlist_model`: PASSED (0.12s) — verifies queue operations, repeat modes, sorting, shuffle, and 10k item load performance.
  - `test_playlist_dock`: PASSED (0.23s) — verifies dark dock view, proxy search filter, delegate rendering, and toolbar actions.
  - `test_main_window_navigation`: PASSED (0.96s) — verifies F6 dock toggle, right-click menu, drag & drop ingestion, and EOF auto-advance.
  - Result: 15/15 passed (100%), 0 failures (Total Test time = 11.88s).
- **End-to-End Synthetic Media Playback (`wsl xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1`):**
  - Exit code: `0`
  - Render Surface: `QOpenGLWidget` + `mpv_render_context` using OpenGL FBO (`rgba16f`, Mesa llvmpipe desktop OpenGL 4.6).
  - Stream lifecycle: Demuxed `lavfi` source (640x360 @ 25fps), presented frames through `mpv_render_context_render()`, reached container EOF, and exited cleanly.
- **Specification & Scaffold Verification Suite:**
  - `python3 tests/test_phase1_specs.py`: PASSED (all Phase 1 spec assertions satisfied, 0 clean-room branding violations).
  - `python3 tests/test_project_scaffold.py`: PASSED (scaffolding and governance verified).
- **Architectural Integrity:**
  - ADR 007 formally approved and recorded in `docs/decisions.md`.
  - Zero UI dependencies in `src/core/`, `src/engine/`, and `src/playlist/PlaylistItem.h` maintained.

### Vertical Slices Progress Table

| Slice | Description | Status | Target Deliverables | Verification Evidence |
|---|---|---|---|---|
| **Slice 1** | Playback Core & Precise Seeking | Done (Phase 3A) | Exact frame step, relative/absolute seek, speed controls (0.2x - 12x), timeline slider with chapter markers | `test_mpv_engine`, `test_timeline_slider`, `test_main_window_ui` PASS |
| **Slice 2** | Control Bar & OSD Engine | Done (Phase 3A) | Autohiding bottom control bar, timeline slider, high-res OSD overlay, 2.5s blank cursor | `test_control_bar`, `test_osd_overlay`, `test_main_window_ui` PASS |
| **Slice 3** | Hotkey Router & Mouse Gestures | Done (Phase 3B) | Global/local shortcut listener, configurable mouse clicks/wheel actions, 1:1 context menu action routing | `test_input_manager`, `test_context_menu` PASS |
| **Slice 4** | Playlist Model/View (F6) | Done (Phase 3B) | High-performance QAbstractItemModel, repeat modes, sorting, search filter dock widget, dark delegate | `test_playlist_model`, `test_playlist_dock`, 10k item benchmark PASS |
| **Slice 5** | File & URL Open Dialogs / Media Ingestion | Done (Phase 3B) | MediaOpenController recursive scanner, OpenUrlDialog with history, drag-and-drop loading, EOF track auto-advance | `test_media_open_dialogs`, `test_main_window_navigation` PASS |
| **Slice 6** | Audio Subsystem | Not Started | 10-band equalizer, volume boost, normalization, delay, track selection | Audio filter pipe tests |
| **Slice 7** | Video Renderer Controls | Not Started | Aspect ratio, rotation, zoom/pan, deinterlacing, color adjustments | OpenGL shader parameter tests |
| **Slice 8** | Subtitle Engine | Not Started | ASS/SSA, SRT, WebVTT rendering, subtitle sync adjustments, styling | libass subtitle placement tests |
| **Slice 9** | Bookmarks & Screen Capture | Not Started | Bookmark manager, instant frame capture (PNG/JPG/GIF), burst capture | File generation verification tests |
| **Slice 10** | Preferences Dialog (F5) | Not Started | Full categorical preferences tree dialog, search filter, reset defaults | Preference dialog test harness |
| **Slice 11** | Streams, Optical & V4L2 Devices | Not Started | HLS, DASH, RTSP, YouTube/yt-dlp integration, DVD/Blu-ray, webcam input | URL opening & streaming tests |
| **Slice 12** | Theming, Skins & SVG Vector Icons | Not Started | Custom dark skin, frameless window styling, original vector iconography | Visual rendering test under xvfb |
| **Slice 13** | Linux Desktop Integration | Not Started | MPRIS2 D-Bus interface, XDG inhibit portal, StatusNotifierItem tray | D-Bus compliance test suite |

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
