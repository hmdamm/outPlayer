# Phase 3A Implementation Plan: Interactive Player & Core Controls

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Transform outPlayer into a fully interactive desktop media player with a custom frameless dark window, a timeline scrubber with hover preview, a 40px playback control bar, an On-Screen Display (OSD) overlay, and autohide behavior.

**Architecture:** Layered QWidget presentation above the hardware-accelerated `VideoWidget` (`mpv_render_context`), driven by `CommandRegistry` for action dispatch, `InputManager` for hotkeys and mouse gestures, `IPlaybackEngine` for asynchronous playback state, and `SettingsManager` for persistence.

**Tech Stack:** C++20, Qt 6.8.2 (`QtWidgets`, `QtOpenGLWidgets`, `QtCore`, `QtGui`), `libmpv`.

**Spec:** `docs/spec/ui-layout.md`, `docs/spec/features.csv`, `docs/spec/hotkeys.csv`, `docs/architecture.md`.

## Global Constraints

- Modern C++20 standard; Qt 6.8.2 with `clang` / `g++ 15`.
- Strict clean-room implementation: 0 proprietary assets, skin bitmaps, or brand names.
- Zero UI dependencies in `src/core/` and `src/engine/`. All UI classes reside strictly in `src/ui/` (`outplayer_ui` target).
- All source files must include `// SPDX-License-Identifier: GPL-3.0-or-later`.
- Deterministic geometry and dark theme tokens defined in `docs/spec/ui-layout.md` (`#1b1b1b` base, `#252525` control bar, `#e09b1f` progress, `#ffaa00` accent).
- Test-Driven Development (TDD): Red-Green-Refactor with failure evidence before implementation.

## Review Focus

1. **Window Resize & Aspect Ratio Glitches:** Resizing the frameless window or toggling fullscreen must smoothly adjust the FBO and avoid black-frame flicker.
2. **Seek Slider Drag Jitter:** Scrubbing the timeline must not stutter audio or queue dozens of synchronous mpv seeks; seeks during drag should use absolute seeks with non-blocking dispatch.
3. **Autohide Mouse Strobe:** Mouse moving across the control bar must keep controls visible; autohide timer (2.5s) must only count down when mouse is stationary or outside interactive areas.
4. **Frameless Window Draggability:** Dragging the custom titlebar must move the window across multi-monitor Linux setups (X11 & Wayland).
5. **OSD Overlay Legibility:** OSD text must remain readable over pure white or noisy video backgrounds via a 2px dark outline / drop-shadow.

---

### Task 1: Timeline Slider & Precise Time Formatting

**Files:**
- Create: `src/ui/TimelineSlider.h`
- Create: `src/ui/TimelineSlider.cpp`
- Create: `src/ui/TimeFormatter.h`
- Create: `tests/test_timeline_slider.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
```cpp
namespace outplayer::ui {

class TimeFormatter {
public:
    static QString formatTime(double seconds, bool showHours = false);
    static QString formatTimePair(double currentSeconds, double totalSeconds);
};

class TimelineSlider : public QSlider {
    Q_OBJECT
public:
    explicit TimelineSlider(QWidget* parent = nullptr);

    void setDuration(double seconds);
    void setPosition(double seconds);
    void setBufferProgress(double fraction);
    void setChapters(const QList<engine::ChapterInfo>& chapters);

    [[nodiscard]] double currentSeconds() const noexcept;
    [[nodiscard]] double durationSeconds() const noexcept;
    [[nodiscard]] bool isScrubbing() const noexcept;

signals:
    void seekRequested(double targetSeconds);
    void scrubStarted();
    void scrubMoved(double targetSeconds);
    void scrubEnded(double targetSeconds);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;
};

} // namespace outplayer::ui
```

- [ ] **Step 1: Write failing unit test for `TimeFormatter` and `TimelineSlider`**
Create `tests/test_timeline_slider.cpp` asserting:
- `TimeFormatter::formatTime(0)` == `"00:00"`, `formatTime(3661)` == `"01:01:01"`.
- `TimeFormatter::formatTimePair(65, 3600)` == `"00:01:05 / 01:00:00"`.
- Slider click-to-seek calculation: clicking at 50% width on a 100s duration yields `seekRequested(50.0)`.
- Scrubbing state: `isScrubbing()` true between press and release.
- Chapter markers and buffer range properties.

- [ ] **Step 2: Run test build to verify failure before implementation**
Run: `wsl cmake --build build --target test_timeline_slider`  
Expected: FAIL with missing headers.

- [ ] **Step 3: Implement `TimeFormatter` and `TimelineSlider`**
- In `src/ui/TimeFormatter.h`: header-only or utility helper for time formatting.
- In `src/ui/TimelineSlider.h` & `TimelineSlider.cpp`:
  - 14px total height with 4px groove height per `ui-layout.md`.
  - Custom `paintEvent` rendering buffer bar (`#555555`), playback progress (`#e09b1f`), hover highlight (`#ffaa00`), groove (`#383838`), and chapter ticks.
  - Hover tooltip showing formatted timestamp at mouse position.
  - Immediate click-to-seek on mouse click (no thumb drag delay).
- Update `src/CMakeLists.txt` and `tests/CMakeLists.txt`.

- [ ] **Step 4: Run unit tests under xvfb to verify they pass**
Run: `wsl xvfb-run -a ctest --test-dir build -R test_timeline_slider --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/TimelineSlider.h src/ui/TimelineSlider.cpp src/ui/TimeFormatter.h tests/test_timeline_slider.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement TimelineSlider with hover preview and chapter ticks"
```

---

### Task 2: Playback Control Bar Widget

**Files:**
- Create: `src/ui/ControlBar.h`
- Create: `src/ui/ControlBar.cpp`
- Create: `tests/test_control_bar.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
```cpp
namespace outplayer::ui {

class ControlBar : public QWidget {
    Q_OBJECT
public:
    explicit ControlBar(QWidget* parent = nullptr);

    void setPlaybackEngine(engine::IPlaybackEngine* engine);
    void setPlaying(bool isPlaying);
    void setTime(double currentSeconds, double totalSeconds);
    void setVolume(double volumePercent);
    void setMuted(bool isMuted);

    [[nodiscard]] TimelineSlider* timelineSlider() const noexcept;

signals:
    void playPauseClicked();
    void stopClicked();
    void prevClicked();
    void nextClicked();
    void volumeChanged(double volumePercent);
    void muteToggled();
    void playlistToggled();
    void controlPanelToggled();
    void subtitleToggled();
};

} // namespace outplayer::ui
```

- [ ] **Step 1: Write failing unit test for `ControlBar`**
Create `tests/test_control_bar.cpp` asserting:
- `ControlBar` height is 40px fixed.
- Time label text updates when `setTime()` is called.
- Play/Pause icon toggles state between play and pause.
- Clicking buttons emits corresponding signals and dispatches to `CommandRegistry`.
- Volume slider updates and signals reflect changes.

- [ ] **Step 2: Run test build to verify failure before implementation**
Run: `wsl cmake --build build --target test_control_bar`  
Expected: FAIL with compilation error.

- [ ] **Step 3: Implement `ControlBar`**
- In `src/ui/ControlBar.h` & `ControlBar.cpp`:
  - 40px height with dark background `#252525` and subtle top border `#2e2e2e`.
  - Layout: `QHBoxLayout` with Play/Pause button, Stop button, Prev/Next buttons, Time display label (`#ffffff`), `TimelineSlider` integrated or stacked above, Volume icon/mute button, 70px Volume slider, Subtitle button, Playlist button (F6), Control Panel button (F7).
  - Connect buttons to `CommandRegistry::instance().execute()`.
  - Connect `IPlaybackEngine` signals (`stateChanged`, `positionChanged`, `durationChanged`, `volumeChanged`, `muteChanged`) to update UI controls.
- Update `src/CMakeLists.txt` and `tests/CMakeLists.txt`.

- [ ] **Step 4: Run unit tests under xvfb to verify they pass**
Run: `wsl xvfb-run -a ctest --test-dir build -R test_control_bar --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/ControlBar.h src/ui/ControlBar.cpp tests/test_control_bar.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement 40px ControlBar with playback buttons and volume scrubber"
```

---

### Task 3: On-Screen Display (OSD) Engine

**Files:**
- Create: `src/ui/OsdOverlay.h`
- Create: `src/ui/OsdOverlay.cpp`
- Create: `tests/test_osd_overlay.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
```cpp
namespace outplayer::ui {

enum class OsdPosition {
    TopLeft,
    Center,
    BottomLeft
};

class OsdOverlay : public QWidget {
    Q_OBJECT
public:
    explicit OsdOverlay(QWidget* parent = nullptr);

    void showMessage(const QString& text, int durationMs = 1500, OsdPosition pos = OsdPosition::TopLeft);
    void clear();

    [[nodiscard]] QString currentText() const;
    [[nodiscard]] bool isVisibleOsd() const;

protected:
    void paintEvent(QPaintEvent* event) override;

private slots:
    void onFadeStep();
};

} // namespace outplayer::ui
```

- [ ] **Step 1: Write failing unit test for `OsdOverlay`**
Create `tests/test_osd_overlay.cpp` asserting:
- `showMessage("Volume: 85%")` sets text and activates display.
- After timer timeout, overlay fades and clears.
- Multi-message queuing: new message immediately replaces previous.

- [ ] **Step 2: Run test build to verify failure before implementation**
Run: `wsl cmake --build build --target test_osd_overlay`  
Expected: FAIL with compilation error.

- [ ] **Step 3: Implement `OsdOverlay`**
- In `src/ui/OsdOverlay.h` & `OsdOverlay.cpp`:
  - Transparent overlay widget (`Qt::WA_TransparentForMouseEvents`, `Qt::WA_NoSystemBackground`).
  - Paint text using `QPainterPath` to render 18pt semi-bold text with `#ffffff` fill and 2px `#000000` outline/shadow for high contrast over any video background.
  - Timer-based alpha decay for smooth 1.5s fade-out.
  - Auto-resizes to track parent `VideoWidget` geometry.
- Connect `CommandRegistry` or playback engine events (seek, volume, pause/play, speed) to display OSD notifications.
- Update `src/CMakeLists.txt` and `tests/CMakeLists.txt`.

- [ ] **Step 4: Run unit tests under xvfb to verify they pass**
Run: `wsl xvfb-run -a ctest --test-dir build -R test_osd_overlay --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/OsdOverlay.h src/ui/OsdOverlay.cpp tests/test_osd_overlay.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement high-contrast OsdOverlay with outlined typography and fadeout"
```

---

### Task 4: Frameless Window, Custom Titlebar & Autohide Chrome

**Files:**
- Create: `src/ui/CustomTitleBar.h`
- Create: `src/ui/CustomTitleBar.cpp`
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Create: `tests/test_main_window_ui.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
```cpp
namespace outplayer::ui {

class CustomTitleBar : public QWidget {
    Q_OBJECT
public:
    explicit CustomTitleBar(QWidget* parent = nullptr);

    void setTitle(const QString& title);
    void setMaximizedState(bool isMaximized);

signals:
    void minimizeClicked();
    void maximizeRestoreClicked();
    void closeClicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
};

} // namespace outplayer::ui
```

- [ ] **Step 1: Write failing unit test for `CustomTitleBar` and `MainWindow` UI composition**
Create `tests/test_main_window_ui.cpp` asserting:
- Custom titlebar is 30px height.
- Minimize, Maximize, and Close buttons trigger appropriate window states.
- Window starts with frameless client decorations when `General/UseSystemWindowDecorations` is false.
- Fullscreen toggle hides `CustomTitleBar` and `ControlBar`.
- Mouse inactivity timer (2.5s) toggles chrome visibility.

- [ ] **Step 2: Run test build to verify failure before implementation**
Run: `wsl cmake --build build --target test_main_window_ui`  
Expected: FAIL with compilation error.

- [ ] **Step 3: Implement `CustomTitleBar` and assemble `MainWindow`**
- In `src/ui/CustomTitleBar.h` & `CustomTitleBar.cpp`:
  - 30px dark titlebar `#1b1b1b` with icon, elided title, minimize, maximize/restore, close buttons (`#c42b1c` on hover).
  - Native window dragging via `window()->windowHandle()->startSystemMove()`.
  - Double-click to maximize/restore.
- In `src/ui/MainWindow.h` & `MainWindow.cpp`:
  - Assemble layered layout: `CustomTitleBar` (top), `VideoWidget` + `OsdOverlay` (center), `TimelineSlider` + `ControlBar` (bottom).
  - Setup border resize handles for frameless mode.
  - Setup mouse inactivity tracking (2.5s timer): hide titlebar and control bar when mouse is idle during playback, show on mouse motion.
  - Wire `InputManager` to `MainWindow`.
  - Wire `CMD_WINDOW_FULLSCREEN` to toggle fullscreen mode cleanly.
- Update `src/CMakeLists.txt` and `tests/CMakeLists.txt`.

- [ ] **Step 4: Run unit tests under xvfb to verify they pass**
Run: `wsl xvfb-run -a ctest --test-dir build -R test_main_window_ui --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/CustomTitleBar.h src/ui/CustomTitleBar.cpp src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_main_window_ui.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): assemble frameless MainWindow with CustomTitleBar and 2.5s chrome autohide"
```

---

### Task 5: Phase 3A Gate Review & Integration Verification

**Files:**
- Modify: `TASKS.md`
- Modify: `docs/decisions.md`

- [ ] **Step 1: Execute complete build and test suite**
- `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
- `wsl bash -c 'xvfb-run -a ctest --test-dir build --output-on-failure'`
- `wsl bash -c 'python3 tests/test_phase1_specs.py'`
- Verify 100% tests pass.

- [ ] **Step 2: Record ADR 006 in `docs/decisions.md`**
Record "ADR 006: Interactive Player UI Layer, Timeline Scrubber & Chrome Autohide".

- [ ] **Step 3: Update `TASKS.md`**
Mark Phase 3 Slices 1, 2, and UI framework tasks as Cleared under Phase 3A milestone.

- [ ] **Step 4: Commit**
```bash
git add docs/decisions.md TASKS.md
git commit -m "docs: complete Phase 3A gate review and record ADR 006"
```
