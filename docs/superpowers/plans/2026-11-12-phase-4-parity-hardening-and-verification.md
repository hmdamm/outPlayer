# Phase 4: Parity Hardening & Verification Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Execute comprehensive parity hardening, automated hotkey execution coverage against all 253 commands, offscreen layout metric validation, format matrix verification, latency/startup performance benchmarks, and memory safety sanitizer verification.

**Architecture:**
- `test_hotkey_parity`: Automated test runner loading `docs/spec/hotkeys.csv`, verifying that every command ID is registered in `CommandRegistry`, has valid keybindings, and dispatches properly through the input pipeline.
- `test_ui_layout_validation`: Offscreen QPainter and widget geometry validation verifying exact heights (TitleBar: 30px, ControlBar: 40px, TimelineSlider: 14px), color tokens (`#1b1b1b`, `#252525`, `#383838`, `#ffaa00`), and dark theme consistency.
- `test_format_matrix`: Synthetic media stream matrix validating playback of video containers (`mp4`, `mkv`, `avi`, `webm`, `flv`, `mov`), audio streams (`mp3`, `flac`, `wav`, `ogg`, `aac`, `opus`), and external subtitles (`srt`, `ass`, `vtt`).
- `test_benchmarks`: Automated latency benchmarks validating player startup time (< 500ms), seek latency (< 50ms), and 10k playlist memory efficiency.
- Sanitizer audit: Clean compilation and test execution under GCC AddressSanitizer (ASan) and UndefinedBehaviorSanitizer (UBSan).

**Tech Stack:** C++20, Qt 6, libmpv, ASan/UBSan, CMake, Ninja, Xvfb.

**Spec:**
- `docs/spec/hotkeys.csv` (253 command mappings)
- `docs/spec/ui-layout.md` (UI layout specifications, color tokens, geometry)
- `docs/spec/formats.md` (Container, codec, and subtitle format matrices)
- `docs/spec/features.csv` (345 features across 13 areas)

## Global Constraints

- Modern C++20 standard; clean RAII memory management and parent-child `QObject` hierarchies.
- Strict Clean-Room rules: zero occurrences of forbidden proprietary trademarks (`PotPlayer`, `Daum`, `Kakao`) in `src/` or binary outputs.
- 100% automated test pass rate under WSL2 Kali Linux with headless X11 (`QT_QPA_PLATFORM=xcb xvfb-run -a ctest ...`).

## Review Focus

1. **Hotkey Parity Coverage Completeness:** All 253 hotkeys from `hotkeys.csv` must map to valid `CommandId` enums in `CommandRegistry` with zero missing handlers or unregistered actions.
2. **Offscreen Layout Discrepancy Tolerance (< 1%):** Layout measurements must conform exactly to `ui-layout.md` specifications.
3. **Format Matrix Demuxer Resilience:** Corrupted or truncated media headers must be handled gracefully without crashing the player engine.
4. **Benchmark Threshold Strictness:** Latency benchmarks must consistently pass below target thresholds (< 500ms startup, < 50ms seek) without jitter.
5. **ASan / UBSan Cleanliness:** Zero buffer overflows, use-after-free, leaks, or undefined behavior during full suite execution under sanitizers.

---

### Task 1: Automated Hotkey Parity Test Suite (`test_hotkey_parity`)

**Files:**
- Create: `tests/test_hotkey_parity.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `docs/spec/hotkeys.csv`, `core/CommandRegistry.h`, `core/CommandId.h`, `input/InputManager.h`
- Produces: Executable `test_hotkey_parity` validating:
  1. 100% of rows in `docs/spec/hotkeys.csv` (253 rows) resolve to valid `CommandId` enum values.
  2. Every command has a non-empty human-readable label and menu category.
  3. Every assigned key sequence is a valid `QKeySequence`.
  4. Executing every command through `CommandRegistry::instance().execute(cmdId)` succeeds without crashes.

- [ ] **Step 1: Write the failing test in `tests/test_hotkey_parity.cpp`**
Parse `docs/spec/hotkeys.csv` and assert that all 253 commands exist in `CommandRegistry`, matching shortcuts and execution dispatch.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL (missing test file).

- [ ] **Step 3: Implement `tests/test_hotkey_parity.cpp`**
Implement the parity test harness. Register in `tests/CMakeLists.txt`. Fix any missing command IDs or alias mappings if discovered.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_hotkey_parity --output-on-failure'`
Expected: PASS (253/253 commands validated).

- [ ] **Step 5: Commit**
```bash
git add tests/test_hotkey_parity.cpp tests/CMakeLists.txt
git commit -m "test(parity): implement automated 253-command hotkey parity test suite"
```

---

### Task 2: UI Layout & Visual Diff Metric Validation (`test_ui_layout_validation`)

**Files:**
- Create: `tests/test_ui_layout_validation.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ui/MainWindow.h`, `ui/CustomTitleBar.h`, `ui/ControlBar.h`, `ui/TimelineSlider.h`, `ui/PlaylistDock.h`, `ui/ControlPanel.h`, `ui/OsdOverlay.h`
- Produces: Executable `test_ui_layout_validation` validating:
  1. `CustomTitleBar` height is exactly 30px with `#1b1b1b` base styling.
  2. `ControlBar` height is exactly 40px with `#252525` base styling and `#2e2e2e` border.
  3. `TimelineSlider` height is exactly 14px with 4px groove height, `#383838` background, and `#ffaa00` progress.
  4. `PlaylistDock` width constraints (default 300px, min 200px, max 600px).
  5. `ControlPanel` dimensions (~420x360px) and tool window flag.
  6. `OsdOverlay` pass-through mouse flag and transparent background.

- [ ] **Step 1: Write the failing test in `tests/test_ui_layout_validation.cpp`**
Assert layout metrics, dimensions, constraints, and color palette tokens across all UI widgets.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL (missing test file).

- [ ] **Step 3: Implement `tests/test_ui_layout_validation.cpp`**
Implement layout assertion suite. Register in `tests/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_ui_layout_validation --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add tests/test_ui_layout_validation.cpp tests/CMakeLists.txt
git commit -m "test(layout): implement UI layout and visual metric validation test suite"
```

---

### Task 3: Comprehensive Format Matrix Test Suite (`test_format_matrix`)

**Files:**
- Create: `tests/test_format_matrix.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `engine/MpvEngine.h`, `docs/spec/formats.md`, `ui/MediaOpenController.h`
- Produces: Executable `test_format_matrix` testing:
  1. Extension recognition and mime type mapping across 40+ container and codec combinations (MP4, MKV, AVI, WebM, FLV, MOV, TS, MP3, FLAC, WAV, OGG, Opus, AAC, M3U, M3U8, PLS, SRT, ASS, VTT).
  2. Synthetic stream generation via ffmpeg/lavfi and playback validation through `MpvEngine`.
  3. Graceful error handling on corrupt or zero-byte files (clean EOF / error signal, no crash).

- [ ] **Step 1: Write the failing test in `tests/test_format_matrix.cpp`**
Implement matrix of format tests and error handling checks.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL.

- [ ] **Step 3: Implement `tests/test_format_matrix.cpp`**
Register in `tests/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_format_matrix --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add tests/test_format_matrix.cpp tests/CMakeLists.txt
git commit -m "test(formats): implement comprehensive media format matrix test suite"
```

---

### Task 4: Performance & Latency Benchmarks (`test_benchmarks`)

**Files:**
- Create: `tests/test_benchmarks.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ui/MainWindow.h`, `engine/MpvEngine.h`, `playlist/PlaylistModel.h`
- Produces: Executable `test_benchmarks` verifying:
  1. Startup Time: `MainWindow` instantiation, widget layout assembly, and engine initialization in < 500ms.
  2. Seek Latency: `engine->seek()` processing time in < 50ms.
  3. Playlist Scalability: 10,000 item insertion in < 50ms, memory overhead < 10MB for 10k items.
  4. Autohide responsiveness: Inactivity timer precision within 50ms tolerance.

- [ ] **Step 1: Write the failing test in `tests/test_benchmarks.cpp`**
Assert performance timing thresholds using `QElapsedTimer`.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL.

- [ ] **Step 3: Implement `tests/test_benchmarks.cpp`**
Register in `tests/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_benchmarks --output-on-failure'`
Expected: PASS (all latency and memory thresholds met).

- [ ] **Step 5: Commit**
```bash
git add tests/test_benchmarks.cpp tests/CMakeLists.txt
git commit -m "perf(benchmarks): implement latency and performance benchmark test suite"
```

---

### Task 5: Memory Safety & Sanitizer Audit (ASan / UBSan)

**Files:**
- Modify: `CMakeLists.txt` (enable optional `-DENABLE_SANITIZERS=ON`)
- Verify all tests pass with zero memory leaks, buffer overflows, or undefined behavior.

- [ ] **Step 1: Configure build with AddressSanitizer and UndefinedBehaviorSanitizer**
Run:
```bash
wsl bash -c 'cmake -B build-asan -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON && cmake --build build-asan -j$(nproc)'
```

- [ ] **Step 2: Execute full test suite under ASan/UBSan**
Run:
```bash
wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build-asan --output-on-failure'
```
Expected: 100% tests pass, zero sanitizer warnings.

- [ ] **Step 3: Commit sanitizer build support in CMakeLists.txt**
```bash
git add CMakeLists.txt
git commit -m "build(sanitizers): add ENABLE_SANITIZERS option and verify ASan/UBSan cleanliness"
```

---

### Task 6: Phase 4 Gate Review, Parity Certification & ADR 010

**Files:**
- Modify: `docs/decisions.md`
- Modify: `TASKS.md`

- [ ] **Step 1: Execute complete verification run**
```bash
wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'
wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build --output-on-failure'
wsl bash -c 'python3 tests/test_phase1_specs.py'
wsl bash -c 'xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1'
```
Verify 100% tests pass.

- [ ] **Step 2: Record ADR 010 in `docs/decisions.md`**
Record "ADR 010: Parity Hardening, Latency Thresholds & Sanitizer Certification".

- [ ] **Step 3: Update `TASKS.md`**
Mark Phase 4 tasks (P4-T1 through P4-T6) as Cleared.

- [ ] **Step 4: Commit**
```bash
git add docs/decisions.md TASKS.md
git commit -m "docs: complete Phase 4 gate review, record ADR 010, and certify parity"
```
