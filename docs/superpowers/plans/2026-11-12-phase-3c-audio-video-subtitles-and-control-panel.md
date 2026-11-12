# Phase 3C: Audio & Video Subsystems, Subtitles, Frame Capture & Control Panel Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement real-time video equalization (brightness, contrast, saturation, hue, rotation, deinterlace), 10-band audio equalization and sync, subtitle engine controls and styling, media bookmarks, instant frame capture, and the multi-tab Control Panel (`F7`).

**Architecture:**
- Extend `IPlaybackEngine` and `MpvEngine` with reactive properties for video equalization, 10-band audio equalization, subtitle delay/scaling, and frame capture.
- `BookmarkManager` provides persistent timestamped bookmarks per file path with hotkey traversal (`H`, `Shift+H`, `Ctrl+H`).
- `ScreenCaptureController` provides instant frame capture (`Ctrl+E`) to disk and clipboard copying (`Ctrl+C`).
- `ControlPanel` (`F7`) provides a floating non-modal dark tool window (`Qt::Tool`) featuring 4 tabs: Video, Audio / EQ, Subtitles, and Playback.
- Integration in `MainWindow` routes hotkeys and commands (`CommandRegistry`) to the engine, capture controller, and control panel.

**Tech Stack:** C++20, Qt 6 (Widgets, Core, Gui), libmpv (`mpv_command`, properties), CMake, Ninja, Xvfb.

**Spec:**
- `docs/spec/features.csv` (Video, Audio, Subtitles, Bookmarks, Capture areas)
- `docs/spec/hotkeys.csv` (F7, H, Ctrl+E, Ctrl+C, audio/video EQ hotkeys)
- `docs/spec/ui-layout.md` (Section 10: Control Panel F7 Specification)

## Global Constraints

- Modern C++20 standard; clean RAII memory management and parent-child `QObject` hierarchies.
- Strict Clean-Room rules: zero occurrences of forbidden proprietary trademarks (`PotPlayer`, `Daum`, `Kakao`) in `src/` or binary outputs.
- Zero UI dependencies (`QWidget`, `QPainter`) in `src/core/` and `src/engine/`.
- Dark theme palette: base background `#1b1b1b` / `#1e1e1e`, panel background `#252525`, border `#2e2e2e`, accent/amber `#ffaa00` / `#e09b1f`.
- 100% automated test pass rate under WSL2 Kali Linux with headless X11 (`QT_QPA_PLATFORM=xcb xvfb-run -a ctest ...`).

## Review Focus

1. **Equalizer Bounds & Hardware Reset:** Video adjustments (brightness, contrast, saturation, hue in [-100, 100]) and audio 10-band EQ ([-12dB, +12dB]) must cleanly reset to neutral 0 without audio distortion or video pipeline resets.
2. **Audio/Subtitle Sync Drift Handling:** Relative sync adjustments (+/- 0.05s, +/- 0.5s) must correctly update `IPlaybackEngine` and display millisecond accuracy in OSD / Control Panel.
3. **Screen Capture Format & Directory Safety:** Capturing frames (`Ctrl+E`) must write valid PNG/JPG image files, safely create output directories if missing, and gracefully handle read-only file systems.
4. **Bookmark Timestamp Stability:** Bookmarks must be stored with accurate float seconds and persisted across sessions in `SettingsManager`.
5. **Control Panel Non-Modal Concurrency:** The `ControlPanel` (`Qt::Tool`) must float alongside `MainWindow` without stealing playback focus or locking video scrubbing.

---

### Task 1: Engine AV Equalizer, Subtitle Sync & Frame Capture Extension

**Files:**
- Modify: `src/engine/IPlaybackEngine.h`
- Modify: `src/engine/MpvEngine.h`
- Modify: `src/engine/MpvEngine.cpp`
- Test: `tests/test_mpv_engine_av.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `libmpv` property APIs (`mpv_set_property`, `mpv_command_async`)
- Produces in `IPlaybackEngine`:
  ```cpp
  // Video Equalizer
  virtual int brightness() const = 0;
  virtual void setBrightness(int val) = 0;
  virtual int contrast() const = 0;
  virtual void setContrast(int val) = 0;
  virtual int saturation() const = 0;
  virtual void setSaturation(int val) = 0;
  virtual int hue() const = 0;
  virtual void setHue(int val) = 0;
  virtual int rotation() const = 0;
  virtual void setRotation(int degrees) = 0;
  virtual bool deinterlace() const = 0;
  virtual void setDeinterlace(bool enable) = 0;
  virtual void resetVideoEqualizer() = 0;

  // Audio Equalizer & Sync
  virtual double audioDelay() const = 0;
  virtual void setAudioDelay(double seconds) = 0;
  virtual double equalizerBand(int bandIndex) const = 0;
  virtual void setEqualizerBand(int bandIndex, double gainDb) = 0;
  virtual void resetAudioEqualizer() = 0;
  virtual bool isNormalizerEnabled() const = 0;
  virtual void setNormalizerEnabled(bool enable) = 0;

  // Subtitle Controls & Sync
  virtual double subtitleDelay() const = 0;
  virtual void setSubtitleDelay(double seconds) = 0;
  virtual double subtitleScale() const = 0;
  virtual void setSubtitleScale(double scale) = 0;
  virtual bool loadSubtitle(const QString& filePath) = 0;

  // Frame Capture
  virtual bool captureFrame(const QString& destinationPath) = 0;
  ```

- [ ] **Step 1: Write the failing test in `tests/test_mpv_engine_av.cpp`**
Test getting and setting video brightness (-100 to 100), contrast, saturation, hue, rotation (0, 90, 180, 270), deinterlace toggle, audio delay, 10-band equalizer gains, normalizer toggle, subtitle delay and scale, and capturing a frame from a synthetic stream.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing interface methods).

- [ ] **Step 3: Implement AV equalizer and capture methods in `IPlaybackEngine` and `MpvEngine`**
In `MpvEngine.cpp`:
- Map brightness, contrast, saturation, hue to mpv properties `"brightness"`, `"contrast"`, `"saturation"`, `"gamma"`, `"hue"`.
- Map rotation to `"video-rotate"` and deinterlace to `"deinterlace"`.
- Map audio delay to `"audio-delay"`.
- Map 10-band EQ to mpv filter string (using af=lavfi=[firequalizer] or af=equalizer).
- Map subtitle delay to `"sub-delay"`, scale to `"sub-scale"`, and load subtitle via `sub-add` command.
- Implement `captureFrame` using `screenshot-to-file` mpv command.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_mpv_engine_av --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/engine/IPlaybackEngine.h src/engine/MpvEngine.h src/engine/MpvEngine.cpp tests/test_mpv_engine_av.cpp tests/CMakeLists.txt
git commit -m "feat(engine): implement AV equalizer, subtitle sync, and frame capture in MpvEngine"
```

---

### Task 2: Media Bookmark Manager (`BookmarkManager`)

**Files:**
- Create: `src/core/BookmarkManager.h`
- Create: `src/core/BookmarkManager.cpp`
- Test: `tests/test_bookmark_manager.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `core/SettingsManager.h`
- Produces: `outplayer::core::BookmarkManager` with:
  ```cpp
  namespace outplayer::core {

  struct Bookmark {
      QString filePath;
      double timestamp = 0.0;
      QString title;

      bool operator==(const Bookmark&) const = default;
  };

  class BookmarkManager : public QObject {
      Q_OBJECT
  public:
      explicit BookmarkManager(SettingsManager* settings = nullptr, QObject* parent = nullptr);

      void addBookmark(const QString& filePath, double timestamp, const QString& title = QString());
      void removeBookmark(const QString& filePath, double timestamp);
      void clearBookmarks(const QString& filePath = QString());

      QList<Bookmark> bookmarksForFile(const QString& filePath) const;
      QList<Bookmark> allBookmarks() const;

      std::optional<Bookmark> nextBookmark(const QString& filePath, double currentPos) const;
      std::optional<Bookmark> previousBookmark(const QString& filePath, double currentPos) const;

      void save();
      void load();

  signals:
      void bookmarksChanged(const QString& filePath);
  };

  } // namespace outplayer::core
  ```

- [ ] **Step 1: Write the failing test in `tests/test_bookmark_manager.cpp`**
Test adding bookmarks, removing bookmarks, finding next and previous bookmarks given a current position, persistence round-trip through `SettingsManager`, and signal emission.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `BookmarkManager.h`).

- [ ] **Step 3: Implement `BookmarkManager` in `src/core/`**
Implement bookmark storage using sorted collections per file, persistence to INI via `SettingsManager`, and chronological navigation. Add to `outplayer_core` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_bookmark_manager --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/core/BookmarkManager.h src/core/BookmarkManager.cpp tests/test_bookmark_manager.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(core): implement persistent BookmarkManager with timestamp navigation"
```

---

### Task 3: Screen Capture Controller (`ScreenCaptureController`)

**Files:**
- Create: `src/ui/ScreenCaptureController.h`
- Create: `src/ui/ScreenCaptureController.cpp`
- Test: `tests/test_screen_capture.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `engine/IPlaybackEngine.h`, `core/SettingsManager.h`
- Produces: `outplayer::ui::ScreenCaptureController` with:
  ```cpp
  class ScreenCaptureController : public QObject {
      Q_OBJECT
  public:
      explicit ScreenCaptureController(engine::IPlaybackEngine* engine, core::SettingsManager* settings = nullptr, QObject* parent = nullptr);

      QString captureFolder() const;
      void setCaptureFolder(const QString& path);

      QString captureInstantFrame();
      bool copyFrameToClipboard();

  signals:
      void frameCaptured(const QString& filePath);
      void frameCopiedToClipboard();
      void captureError(const QString& message);
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_screen_capture.cpp`**
Test capture folder generation (defaults to `XDG_PICTURES_DIR/outPlayer` or temporary dir), filename generation with timestamp format `YYYY-MM-DD_HH-mm-ss_frame.png`, calling `engine->captureFrame()`, and error handling for invalid directories.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `ScreenCaptureController.h`).

- [ ] **Step 3: Implement `ScreenCaptureController` in `src/ui/`**
Implement the capture controller, integrating `QStandardPaths::PicturesLocation`, safe directory creation (`QDir().mkpath`), timestamped filename generation, and signal dispatch. Add to `outplayer_ui` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_screen_capture --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/ScreenCaptureController.h src/ui/ScreenCaptureController.cpp tests/test_screen_capture.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement ScreenCaptureController for instant snapshot and clipboard export"
```

---

### Task 4: Control Panel Tool Window (`ControlPanel` - F7)

**Files:**
- Create: `src/ui/ControlPanel.h`
- Create: `src/ui/ControlPanel.cpp`
- Test: `tests/test_control_panel.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `engine/IPlaybackEngine.h`
- Produces: `outplayer::ui::ControlPanel` with:
  ```cpp
  class ControlPanel : public QWidget {
      Q_OBJECT
  public:
      explicit ControlPanel(engine::IPlaybackEngine* engine, QWidget* parent = nullptr);

      void updateFromEngine();

  signals:
      void closeRequested();
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_control_panel.cpp`**
Test construction of `ControlPanel`, presence of 4 tabs: "Video", "Audio / EQ", "Subtitles", "Playback", slider ranges and initial values, slider manipulation updating `IPlaybackEngine`, and reset buttons restoring defaults.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `ControlPanel.h`).

- [ ] **Step 3: Implement `ControlPanel` in `src/ui/ControlPanel.h` and `src/ui/ControlPanel.cpp`**
Implement the styled dark tool window matching `docs/spec/ui-layout.md` section 10:
- Tool window window flags (`Qt::Tool | Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint` or custom dark header).
- Tab 1 Video: Brightness, Contrast, Saturation, Hue sliders (-100 to 100) with Reset buttons; Rotation combobox (0, 90, 180, 270); Deinterlace checkbox; Reset All button.
- Tab 2 Audio/EQ: 10 vertical EQ sliders (31Hz, 62Hz, 125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz, 8kHz, 16kHz) from -12dB to +12dB; Normalizer toggle; Audio delay adjuster.
- Tab 3 Subtitles: Delay sync (-0.5s, +0.5s, reset), font scale slider (50% to 200%), subtitle track combobox, Load Subtitle button.
- Tab 4 Playback: Speed slider (0.2x to 12.0x, default 1.0x) with 1.0x reset button; A-B repeat controls.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_control_panel --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/ControlPanel.h src/ui/ControlPanel.cpp tests/test_control_panel.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement multi-tab ControlPanel (F7) for real-time AV and subtitle tuning"
```

---

### Task 5: MainWindow & Command Integration for AV Controls, Bookmarks & F7

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Test: `tests/test_main_window_av.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ui/ControlPanel.h`, `core/BookmarkManager.h`, `ui/ScreenCaptureController.h`
- Produces: Integrated `MainWindow` coordinating `ControlPanel` toggle via `F7` and `CP` button, bookmark management (`H`, `Shift+H`, `Ctrl+H`), and screen capture (`Ctrl+E`, `Ctrl+C`).

- [ ] **Step 1: Write the failing test in `tests/test_main_window_av.cpp`**
Test:
1. `F7` and `CP` button toggle `ControlPanel` visibility.
2. `H` hotkey adds bookmark, verified in `BookmarkManager`.
3. `Shift+H` and `Ctrl+H` navigate bookmarks.
4. `Ctrl+E` triggers screen capture via `ScreenCaptureController`.
5. Hotkeys for video brightness/contrast/saturation adjust engine values and show OSD notification.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL (missing AV bindings in `MainWindow`).

- [ ] **Step 3: Implement MainWindow AV, Bookmark, and Control Panel integrations**
In `src/ui/MainWindow.cpp`:
- Initialize `BookmarkManager`, `ScreenCaptureController`, and `ControlPanel`.
- Connect `F7`, `CP` button in `m_controlBar`, and `CMD_CONTROL_PANEL` to toggle `ControlPanel`.
- Connect bookmark commands (`CMD_BOOKMARK_ADD`, `CMD_BOOKMARK_PREV`, `CMD_BOOKMARK_NEXT`).
- Connect frame capture commands (`CMD_CAPTURE_SAVE`, `CMD_CAPTURE_COPY`).
- Connect video equalizer commands (`CMD_VIDEO_BRIGHTNESS_UP`, `CMD_VIDEO_BRIGHTNESS_DOWN`, `CMD_VIDEO_BRIGHTNESS_RESET`, etc.) and subtitle sync commands with OSD notifications.
- Clean teardown in `~MainWindow()`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_main_window_av --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_main_window_av.cpp tests/CMakeLists.txt
git commit -m "feat(ui): integrate ControlPanel F7, BookmarkManager, ScreenCapture, and AV equalizer hotkeys into MainWindow"
```

---

### Task 6: Phase 3C Gate Review, Integration Verification & ADR 008

**Files:**
- Modify: `docs/decisions.md`
- Modify: `TASKS.md`

- [ ] **Step 1: Execute complete build and test suite**
```bash
wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'
wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build --output-on-failure'
wsl bash -c 'python3 tests/test_phase1_specs.py'
wsl bash -c 'xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1'
```
Verify 100% tests pass.

- [ ] **Step 2: Record ADR 008 in `docs/decisions.md`**
Record "ADR 008: Audio/Video Processing, Subtitle Timing Engine, Bookmarks & Control Panel F7".

- [ ] **Step 3: Update `TASKS.md`**
Mark Phase 3 Slices 6, 7, 8 as Cleared under Phase 3C milestone.

- [ ] **Step 4: Commit**
```bash
git add docs/decisions.md TASKS.md
git commit -m "docs: complete Phase 3C gate review and record ADR 008"
```
