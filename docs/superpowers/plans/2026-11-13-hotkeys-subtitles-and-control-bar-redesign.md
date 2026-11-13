# Hotkeys, Subtitles & Control Bar Redesign Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Fix playback speed shortcuts (`C`, `X`, `Z`) and comprehensive hotkey handling, fix subtitle detection and on-video rendering, implement complete custom subtitle font and appearance customization (family, size, color, weight, outline, shadow), and redesign the bottom control bar into PotPlayer's 2-tier layout (Seek Bar + Mute & Volume on top; Play/Stop/Prev/Next/Open/Time/Badges on bottom).

**Architecture:**
- `src/ui/MainWindow`: Comprehensive command routing for playback speed adjustments (`CMD_SPEED_UP`, `CMD_SPEED_DOWN`, `CMD_SPEED_NORMAL`), frame stepping, subtitle visibility toggling (`CMD_SUB_TOGGLE_VISIBILITY`), file/subtitle loading (`CMD_SUB_LOAD`), and video info badge propagation.
- `src/engine/MpvEngine`: Configure `sub-auto=fuzzy` and `sub-visibility=yes` in `libmpv`; implement subtitle font styling properties (`sub-font`, `sub-font-size`, `sub-color`, `sub-bold`, `sub-italic`, `sub-border-color`, `sub-border-size`, `sub-shadow-color`, `sub-shadow-offset`) and track selection.
- `src/ui/MediaOpenController`: Add subtitle extensions (`srt`, `ass`, `ssa`, `vtt`, `sub`, `idx`, `sup`) and enable drag-and-drop subtitle file attachment.
- `src/ui/PreferencesDialog` & `src/ui/ControlPanel`: Rich subtitle appearance editor with `QFontComboBox`, font size spinbox, bold/italic checkboxes, and color pickers with live preview.
- `src/ui/ControlBar`: Redesign into a 2-tier widget:
  - Top Row (~18px): Full-width `TimelineSlider` + Mute button (`VOL`) + compact horizontal volume slider.
  - Bottom Row (~38px): Play/Pause, Stop, Prev, Next, Open File (`⏏`), Time Label (`00:00:00 / 00:00:00`), Video Info Badges (`H/W`, Video Codec, Audio Codec, Channels), and Right utility buttons (Subtitles, Control Panel, Settings, Playlist).

**Tech Stack:** C++20, Qt 6 Widgets, `libmpv` (0.41.0), CMake, Ninja, Xvfb.

**Spec:**
- User requirements & uploaded reference screenshots (`media_1794554182340_07b724c6.png`, `media_1794554154741_0e02e87a.jpg`)
- `docs/spec/hotkeys.csv` (Speed shortcuts: `C`, `X`, `Z`; Subtitle shortcuts: `Alt+H`, `Alt+O`, `Alt+PageUp`, `Alt+PageDown`)
- `docs/spec/formats.md` (Subtitle formats: `srt`, `ass`, `ssa`, `vtt`, `sub`, `idx`, `sup`)

## Global Constraints

- Strict Clean-Room rules: zero occurrences of forbidden proprietary trademarks (`PotPlayer`, `Daum`, `Kakao`) in `src/`, binary strings, or UI labels.
- 100% automated test pass rate under WSL2 Kali Linux with headless X11 (`QT_QPA_PLATFORM=xcb xvfb-run -a ctest ...`).
- Thread safety: `libmpv` commands and properties called from UI thread or synchronized event loop; zero UI blocks.

## Review Focus

1. **Playback Speed Hotkeys & Clamping:** `C` increments by +0.1x, `X` decrements by -0.1x, `Z` resets to 1.0x; clamped to valid ranges (0.2x to 12.0x) with OSD notifications.
2. **Subtitle Auto-Loading & Ingestion:** External subtitle files matching video filename (`movie.en.srt`, etc.) auto-load upon video open, and dragging a `.srt`/`.ass` onto the player window immediately attaches and displays it.
3. **Custom Subtitle Font Styling:** Setting font family, size, text color, bold/italic, border, and shadow applies immediately to `libmpv` and renders on screen.
4. **2-Tier Control Bar Geometry & Hierarchy:** Top row has seek bar + mute + volume; bottom row has playback controls, open file, time label, and video info badges.
5. **No Regressions:** All existing 29 CTest suites and packaging validation tests continue to pass.

---

### Task 1: Comprehensive Hotkey & Playback Speed Routing (`C`, `X`, `Z` & Command Handlers)

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Create: `tests/test_main_window_hotkeys.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `core/CommandRegistry.h`, `core/CommandId.h`, `engine/IPlaybackEngine.h`, `input/InputManager.h`
- Produces:
  1. Handlers for `CMD_SPEED_UP` (`C`), `CMD_SPEED_DOWN` (`X`), `CMD_SPEED_NORMAL` (`Z`) adjusting playback speed by ±0.1x / 1.0x and showing OSD `Speed: X.Xx`.
  2. Handlers for speed presets (`CMD_SPEED_FAST_2X`, `CMD_SPEED_SLOW_HALF`).
  3. Handlers for frame step (`CMD_FRAME_STEP`, `CMD_FRAME_BACK`).
  4. Handlers for aspect ratio cycles, zoom, deinterlacing, and window management.
  5. Test `test_main_window_hotkeys` verifying execution and OSD feedback.

- [x] **Step 1: Write failing test in `tests/test_main_window_hotkeys.cpp`**
Test that executing `CMD_SPEED_UP`, `CMD_SPEED_DOWN`, and `CMD_SPEED_NORMAL` via `CommandRegistry::instance().execute()` updates `m_engine->speed()` and updates OSD overlay text.

- [x] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_main_window_hotkeys --output-on-failure'`
Expected: FAIL (missing test / unhandled commands).

- [x] **Step 3: Implement playback speed & expanded command handlers in `MainWindow.cpp`**
Wire `CMD_SPEED_UP`, `CMD_SPEED_DOWN`, `CMD_SPEED_NORMAL`, frame stepping, and general command routing.

- [x] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_main_window_hotkeys --output-on-failure'`
Expected: PASS.

- [x] **Step 5: Commit**
```bash
git add src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_main_window_hotkeys.cpp tests/CMakeLists.txt
git commit -m "feat(hotkeys): implement playback speed shortcuts C/X/Z and expanded command handlers"
```

---

### Task 2: Subtitle Engine & Auto-Detection (`MpvEngine`, Drag-and-Drop, Visibility & Track Selection)

**Files:**
- Modify: `src/engine/IPlaybackEngine.h`
- Modify: `src/engine/MpvEngine.h`
- Modify: `src/engine/MpvEngine.cpp`
- Modify: `src/ui/MediaOpenController.h`
- Modify: `src/ui/MediaOpenController.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Create: `tests/test_subtitle_pipeline.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `engine/IPlaybackEngine.h`, `libmpv`
- Produces:
  1. `MpvEngine` configured with `sub-auto=fuzzy` and `sub-visibility=yes`.
  2. `isSubtitleVisible()` and `setSubtitleVisible(bool)` in `IPlaybackEngine`.
  3. `loadSubtitle(path)` with `"select"` flag.
  4. `MediaOpenController::isSubtitleFile(path)` and subtitle extension recognition (`srt`, `ass`, `ssa`, `vtt`, `sub`, `idx`, `sup`).
  5. `MainWindow::dropEvent` subtitle file handling (loads subtitle on active playback, shows OSD).
  6. Command handlers for `CMD_SUB_TOGGLE_VISIBILITY` (`Alt+H`), `CMD_SUB_LOAD` (`Alt+O`), `CMD_SUB_TRACK_NEXT`, `CMD_SUB_TRACK_PREV`, `CMD_SUB_SIZE_UP`, `CMD_SUB_SIZE_DOWN`, `CMD_SUB_SIZE_RESET`.
  7. Automated test in `tests/test_subtitle_pipeline.cpp`.

- [ ] **Step 1: Write failing test in `tests/test_subtitle_pipeline.cpp`**
Verify subtitle file loading, visibility toggling, track enumeration, and drop event subtitle handling.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_subtitle_pipeline --output-on-failure'`
Expected: FAIL.

- [ ] **Step 3: Implement subtitle engine and UI integration**
Update `MpvEngine`, `MediaOpenController`, and `MainWindow`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_subtitle_pipeline --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/engine/IPlaybackEngine.h src/engine/MpvEngine.h src/engine/MpvEngine.cpp src/ui/MediaOpenController.h src/ui/MediaOpenController.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_subtitle_pipeline.cpp tests/CMakeLists.txt
git commit -m "feat(subtitles): enable auto-detection, drag-drop subtitle loading, and visibility toggling"
```

---

### Task 3: Custom Subtitle Font, Size, Color & Appearance Engine

**Files:**
- Modify: `src/engine/IPlaybackEngine.h`
- Modify: `src/engine/MpvEngine.h`
- Modify: `src/engine/MpvEngine.cpp`
- Modify: `src/ui/PreferencesDialog.h`
- Modify: `src/ui/PreferencesDialog.cpp`
- Modify: `src/ui/ControlPanel.h`
- Modify: `src/ui/ControlPanel.cpp`
- Create: `tests/test_subtitle_font_customization.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `libmpv` subtitle styling properties (`sub-font`, `sub-font-size`, `sub-color`, `sub-bold`, `sub-italic`, `sub-border-color`, `sub-border-size`, `sub-shadow-color`, `sub-shadow-offset`)
- Produces:
  1. Font family, font size, text color, bold, italic, border color/size, shadow color/offset in `IPlaybackEngine` and `MpvEngine`.
  2. Full subtitle appearance editor in `PreferencesDialog` (F5 - Subtitles page) with `QFontComboBox`, font size spinbox, bold/italic, color picker swatches, and live preview.
  3. Quick font and size selector in `ControlPanel` (F7 - Subtitles tab).
  4. Automatic persistence in `SettingsManager` under `[Subtitles]`.
  5. Test `tests/test_subtitle_font_customization.cpp` verifying styling application, settings persistence, and dialog sync.

- [ ] **Step 1: Write failing test in `tests/test_subtitle_font_customization.cpp`**
Verify setting subtitle font family, size, color, bold, border, and shadow updates `MpvEngine`, persists to `SettingsManager`, and syncs with Preferences/ControlPanel.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_subtitle_font_customization --output-on-failure'`
Expected: FAIL.

- [ ] **Step 3: Implement subtitle styling in `MpvEngine`, `PreferencesDialog`, and `ControlPanel`**
Implement properties, dialog controls, and settings sync.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_subtitle_font_customization --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/engine/IPlaybackEngine.h src/engine/MpvEngine.h src/engine/MpvEngine.cpp src/ui/PreferencesDialog.h src/ui/PreferencesDialog.cpp src/ui/ControlPanel.h src/ui/ControlPanel.cpp tests/test_subtitle_font_customization.cpp tests/CMakeLists.txt
git commit -m "feat(subtitles): add custom font selection, size, color, weight, and border customization"
```

---

### Task 4: Bottom Control Bar Redesign (2-Tier PotPlayer Layout)

**Files:**
- Modify: `src/ui/ControlBar.h`
- Modify: `src/ui/ControlBar.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Create: `tests/test_control_bar_layout.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Reference images `media_1794554182340_07b724c6.png` and `media_1794554154741_0e02e87a.jpg`
- Produces:
  1. Redesigned 2-tier `ControlBar` (~56px height):
     - **Top row (~18px):** Seek Bar (`TimelineSlider`) spanning width, Mute button (`VOL` / speaker), compact horizontal volume slider (yellow progress with white handle).
     - **Bottom row (~38px):** Play/Pause, Stop, Prev, Next, Open File (`⏏`), Time Label (`00:00:00 / 00:00:00`), Video Info Badges (`H/W`, Video Codec, Audio Codec, Channels), and Right utility buttons (Subtitles, Control Panel, Settings, Playlist).
  2. Open file button emits `openFileClicked()` -> wired to `promptOpenFiles()`.
  3. Settings button emits `preferencesClicked()` -> wired to `showPreferences()`.
  4. Video info badges update dynamically based on loaded stream properties (`H/W` decoding, video codec, audio codec, audio channels).
  5. Test `tests/test_control_bar_layout.cpp` verifying 2-tier layout, button placements, badge updates, and signal emissions.

- [ ] **Step 1: Write failing test in `tests/test_control_bar_layout.cpp`**
Verify two-tier structure, widget presence, top-row seek/volume, bottom-row controls/badges, and click signals.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc) && QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_control_bar_layout --output-on-failure'`
Expected: FAIL.

- [ ] **Step 3: Implement 2-tier ControlBar and MainWindow connections**
Redesign `ControlBar.h` / `ControlBar.cpp`, wire `openFileClicked` and `preferencesClicked` in `MainWindow`, and connect track change events to update video info badges.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_control_bar_layout --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/ControlBar.h src/ui/ControlBar.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_control_bar_layout.cpp tests/CMakeLists.txt
git commit -m "feat(ui): redesign bottom control bar to 2-tier layout matching PotPlayer"
```

---

### Task 5: Parity & Regression Hardening, ADR 012 & Full Verification

**Files:**
- Modify: `tests/test_ui_layout_validation.cpp`
- Modify: `docs/decisions.md` (record ADR 012)
- Modify: `TASKS.md`

**Interfaces:**
- Consumes: Deliverables from Tasks 1-4
- Produces:
  1. Updated `test_ui_layout_validation.cpp` asserting the new ~56px 2-tier control bar metrics.
  2. Recorded `ADR 012: Two-Tier Control Bar, Playback Speed Hotkeys & Advanced Subtitle Styling Engine`.
  3. Updated `TASKS.md` with verification logs.
  4. 100% test pass rate across all 33 CTest suites and all packaging verification suites.

- [ ] **Step 1: Update `tests/test_ui_layout_validation.cpp`**
Update expected control bar height assertions to match new 2-tier layout (~56px).

- [ ] **Step 2: Record ADR 012 in `docs/decisions.md`**
Document architectural context, decisions, and consequences.

- [ ] **Step 3: Update `TASKS.md`**
Record task completion and verification evidence.

- [ ] **Step 4: Run complete verification suite**
Run:
- `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
- `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build --output-on-failure'`
- `wsl python3 tests/test_packaging_assets.py`
- `wsl python3 tests/test_flatpak_manifest.py`
- `wsl python3 tests/test_debian_packaging.py`
- `wsl python3 tests/test_rpm_arch_packaging.py`
- `wsl python3 tests/test_appimage_packaging.py`
- `wsl python3 tests/test_phase1_specs.py`
- `wsl python3 tests/test_project_scaffold.py`

- [ ] **Step 5: Commit**
```bash
git add tests/test_ui_layout_validation.cpp docs/decisions.md TASKS.md
git commit -m "docs: record ADR 012 and certify hotkeys, subtitles, and control bar redesign"
```
