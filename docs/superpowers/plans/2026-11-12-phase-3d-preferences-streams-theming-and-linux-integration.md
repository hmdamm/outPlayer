# Phase 3D: Preferences Dialog, Streams/Devices, Theming & Linux Desktop Integration Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the Master Preferences dialog (`F5`), Linux desktop integrations (MPRIS2 D-Bus media controls, XDG screensaver inhibit, System Tray), optical and device streams (V4L2 webcam, DVD/Blu-ray, yt-dlp/network streams), and vector SVG theming.

**Architecture:**
- `ThemeManager` manages dark palette variables, accent colors (`#ffaa00`), and SVG vector iconography.
- `PreferencesDialog` (`F5`) provides a modal dialog with categorized settings tree (`QTreeWidget`), search filter, and stacked configuration panels bound to `SettingsManager`.
- `DeviceStreamManager` detects Linux V4L2 devices, optical disc nodes, and formats streaming URLs for `IPlaybackEngine`.
- `DesktopIntegration` implements the standard Linux desktop protocols: MPRIS2 D-Bus (`org.mpris.MediaPlayer2`), XDG screen saver inhibitor during active video playback, and `QSystemTrayIcon`.
- Integration into `MainWindow` coordinates preferences persistence, remote desktop events, and system tray minimization.

**Tech Stack:** C++20, Qt 6 (Widgets, Core, Gui, DBus), libmpv, CMake, Ninja, Xvfb.

**Spec:**
- `docs/spec/features.csv` (Preferences, Skins, Open devices, Help areas)
- `docs/spec/hotkeys.csv` (F5, Ctrl+D, Ctrl+J, etc.)
- `docs/spec/preferences.csv` (87 rows across 7 categories)
- `docs/spec/linux-deviations.md` (MPRIS2 D-Bus, Wayland/X11 Inhibit, System Tray)

## Global Constraints

- Modern C++20 standard; clean RAII memory management and parent-child `QObject` hierarchies.
- Strict Clean-Room rules: zero occurrences of forbidden proprietary trademarks (`PotPlayer`, `Daum`, `Kakao`) in `src/` or binary outputs.
- Zero UI dependencies in `src/core/` (`DeviceStreamManager`).
- Dark theme palette: base background `#1b1b1b` / `#1e1e1e`, panel background `#252525`, border `#2e2e2e`, accent/amber `#ffaa00`.
- 100% automated test pass rate under WSL2 Kali Linux with headless X11 (`QT_QPA_PLATFORM=xcb xvfb-run -a ctest ...`).

## Review Focus

1. **D-Bus Headless Graceful Degradation:** When D-Bus session bus is unavailable (e.g. headless CI, isolated sandboxes), `DesktopIntegration` must gracefully disable MPRIS2 without crashing or throwing unhandled exceptions.
2. **Settings Synchronization & Cancellation:** Changes in `PreferencesDialog` must only commit to `SettingsManager` when clicking `[ OK ]` or `[ Apply ]`; clicking `[ Cancel ]` must restore unmodified settings.
3. **Screen Saver Inhibit Lifecycle:** Screen saver inhibit must only be active during non-paused video playback; pausing, stopping, or closing the player must cleanly release the inhibit lock.
4. **V4L2 & Optical Node Discovery Safety:** Scanning `/dev/video*` and `/dev/cdrom` / `/dev/sr0` must never block the UI thread or crash when devices are absent or permissions are restricted.
5. **Dynamic Theming Contrast:** Switching theme accents must maintain text legibility and high contrast ratios across all custom widgets.

---

### Task 1: Vector Theming & SVG Icon System (`ThemeManager`)

**Files:**
- Create: `src/ui/ThemeManager.h`
- Create: `src/ui/ThemeManager.cpp`
- Test: `tests/test_theme_manager.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `outplayer::ui::ThemeManager` with:
  ```cpp
  class ThemeManager : public QObject {
      Q_OBJECT
  public:
      static ThemeManager& instance();

      enum class AccentColor { Amber, Blue, Green, Crimson };

      void setAccentColor(AccentColor color);
      AccentColor accentColor() const;
      QString accentHex() const;

      QString styleSheet() const;
      QIcon icon(const QString& name, const QColor& color = QColor()) const;
      QPixmap renderSvg(const QString& svgXml, const QSize& size) const;

  signals:
      void themeChanged();
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_theme_manager.cpp`**
Test default accent color (Amber `#ffaa00`), accent color switching, stylesheet generation containing color tokens, icon generation for standard icons ("play", "pause", "stop", "prev", "next", "volume", "mute", "settings", "playlist"), and `themeChanged` signal emission.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `ThemeManager.h`).

- [ ] **Step 3: Implement `ThemeManager` in `src/ui/`**
Implement the singleton `ThemeManager` with vector SVG path rendering, theme stylesheet generation with dark palette variables, and accent color customization. Add to `outplayer_ui` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_theme_manager --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/ThemeManager.h src/ui/ThemeManager.cpp tests/test_theme_manager.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement ThemeManager with SVG vector iconography and accent customization"
```

---

### Task 2: Master Preferences Dialog (`PreferencesDialog` - F5)

**Files:**
- Create: `src/ui/PreferencesDialog.h`
- Create: `src/ui/PreferencesDialog.cpp`
- Test: `tests/test_preferences_dialog.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `core/SettingsManager.h`, `ui/ThemeManager.h`
- Produces: `outplayer::ui::PreferencesDialog` with:
  ```cpp
  class PreferencesDialog : public QDialog {
      Q_OBJECT
  public:
      explicit PreferencesDialog(core::SettingsManager* settings = nullptr, QWidget* parent = nullptr);

      void loadSettings();
      void saveSettings();
      void resetDefaults();

      void selectCategory(const QString& categoryName);
      void setSearchFilter(const QString& query);

  signals:
      void settingsApplied();
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_preferences_dialog.cpp`**
Test creation of `PreferencesDialog`, tree categories (General, Playback, Video, Audio, Subtitles, Associations, Hotkeys), search filter hiding non-matching categories, changing settings, clicking Cancel reverting changes, clicking Apply/OK persisting to `SettingsManager`, and Reset Defaults.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `PreferencesDialog.h`).

- [ ] **Step 3: Implement `PreferencesDialog` in `src/ui/`**
Implement the styled modal dialog with left category tree (`QTreeWidget`), search filter (`QLineEdit`), stacked settings pages (`QStackedWidget`), and action buttons (`OK`, `Cancel`, `Apply`, `Reset Defaults`). Add to `outplayer_ui` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_preferences_dialog --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/PreferencesDialog.h src/ui/PreferencesDialog.cpp tests/test_preferences_dialog.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement Master PreferencesDialog (F5) with categorized settings and search"
```

---

### Task 3: Streams, Optical & V4L2 Device Manager (`DeviceStreamManager`)

**Files:**
- Create: `src/core/DeviceStreamManager.h`
- Create: `src/core/DeviceStreamManager.cpp`
- Test: `tests/test_device_stream_manager.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Produces: `outplayer::core::DeviceStreamManager` with:
  ```cpp
  namespace outplayer::core {

  struct DeviceInfo {
      QString name;
      QString path;
      QString type; // "v4l2", "optical", "network"
  };

  class DeviceStreamManager : public QObject {
      Q_OBJECT
  public:
      explicit DeviceStreamManager(QObject* parent = nullptr);

      QList<DeviceInfo> availableVideoDevices() const;
      QList<DeviceInfo> availableOpticalDrives() const;

      static bool isStreamUrl(const QString& url);
      static QString formatDvdUrl(const QString& deviceOrFolder = QString());
      static QString formatBlurayUrl(const QString& deviceOrFolder = QString());
      static QString formatV4l2Url(const QString& deviceNode);
  };

  } // namespace outplayer::core
  ```

- [ ] **Step 1: Write the failing test in `tests/test_device_stream_manager.cpp`**
Test `isStreamUrl` for protocols (`http://`, `https://`, `rtsp://`, `hls://`, `dash://`, `dvdnav://`, `bd://`, `v4l2://`), formatting DVD/Blu-ray/V4L2 URLs, and mock device scanning.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `DeviceStreamManager.h`).

- [ ] **Step 3: Implement `DeviceStreamManager` in `src/core/`**
Implement device scanning via `/dev/` file inspection (`video*`, `cdrom*`, `sr*`), stream URL validation, and mpv URI formatting. Zero UI dependencies in `src/core/`. Add to `outplayer_core` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_device_stream_manager --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/core/DeviceStreamManager.h src/core/DeviceStreamManager.cpp tests/test_device_stream_manager.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(core): implement DeviceStreamManager for V4L2 webcams, optical media, and stream URLs"
```

---

### Task 4: Linux Desktop Integration (MPRIS2 D-Bus, ScreenSaver Inhibit & System Tray)

**Files:**
- Create: `src/ui/DesktopIntegration.h`
- Create: `src/ui/DesktopIntegration.cpp`
- Test: `tests/test_desktop_integration.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `engine/IPlaybackEngine.h`, `playlist/PlaylistModel.h`
- Produces: `outplayer::ui::DesktopIntegration` with:
  ```cpp
  class DesktopIntegration : public QObject {
      Q_OBJECT
  public:
      explicit DesktopIntegration(engine::IPlaybackEngine* engine, playlist::PlaylistModel* playlist = nullptr, QWidget* mainWindow = nullptr, QObject* parent = nullptr);
      ~DesktopIntegration() override;

      bool isMprisActive() const;
      bool isScreenSaverInhibited() const;
      bool isSystemTrayAvailable() const;

      void updateMetadata(const QString& title, const QString& artist, double duration);
      void setScreenSaverInhibited(bool inhibit);

  signals:
      void playPauseRequested();
      void stopRequested();
      void nextRequested();
      void previousRequested();
      void raiseWindowRequested();
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_desktop_integration.cpp`**
Test construction with engine and playlist, fallback when D-Bus is unavailable, screensaver inhibit state transitions (active when video playing, released when paused/stopped), metadata updates, and system tray menu construction.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `DesktopIntegration.h`).

- [ ] **Step 3: Implement `DesktopIntegration` in `src/ui/`**
Implement MPRIS2 D-Bus service adapter (`org.mpris.MediaPlayer2.outPlayer`), XDG Inhibit ScreenSaver via `org.freedesktop.ScreenSaver` or `org.freedesktop.portal.Inhibit`, and `QSystemTrayIcon` with context menu. Link `Qt6::DBus` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_desktop_integration --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/DesktopIntegration.h src/ui/DesktopIntegration.cpp tests/test_desktop_integration.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement DesktopIntegration with MPRIS2 D-Bus, XDG ScreenSaver Inhibit, and System Tray"
```

---

### Task 5: MainWindow & Command Integration for Phase 3D

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Test: `tests/test_main_window_integration.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ui/PreferencesDialog.h`, `core/DeviceStreamManager.h`, `ui/DesktopIntegration.h`, `ui/ThemeManager.h`
- Produces: Integrated `MainWindow` coordinating preferences dialog (`F5`), device/stream opening (`Ctrl+D`, `Ctrl+J`), MPRIS2 media key response, and automatic screensaver inhibition.

- [ ] **Step 1: Write the failing test in `tests/test_main_window_integration.cpp`**
Test:
1. `F5` / `CMD_PREFERENCES` opens `PreferencesDialog`.
2. `DesktopIntegration` signals (`playPauseRequested`, `nextRequested`, etc.) trigger engine actions.
3. Screensaver inhibit is active when video plays, inactive when paused.
4. `CMD_OPEN_DVD` and `CMD_OPEN_DEVICE` route to `DeviceStreamManager` and `MpvEngine`.
5. Preferences changes apply theme and settings live.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL.

- [ ] **Step 3: Implement MainWindow integrations**
In `src/ui/MainWindow.cpp`:
- Initialize `DesktopIntegration`, `DeviceStreamManager`, and `PreferencesDialog`.
- Connect `CMD_PREFERENCES` (`F5`) and `ContextMenuManager::preferencesRequested`.
- Connect `CMD_OPEN_DVD` and `CMD_OPEN_DEVICE`.
- Connect `DesktopIntegration` signals to player playback/playlist controls.
- Connect `engine->stateChanged()` to update screensaver inhibition.
- Clean destruction in `~MainWindow()`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_main_window_integration --output-on-failure'`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_main_window_integration.cpp tests/CMakeLists.txt
git commit -m "feat(ui): integrate PreferencesDialog F5, DesktopIntegration, DeviceStreamManager, and theming into MainWindow"
```

---

### Task 6: Phase 3D Gate Review, Integration Verification & ADR 009

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

- [ ] **Step 2: Record ADR 009 in `docs/decisions.md`**
Record "ADR 009: Preferences System F5, Desktop Integration, Device Streams & Vector Theming".

- [ ] **Step 3: Update `TASKS.md`**
Mark Phase 3 as 100% COMPLETE across all 12 vertical slices.

- [ ] **Step 4: Commit**
```bash
git add docs/decisions.md TASKS.md
git commit -m "docs: complete Phase 3D gate review, finalize Phase 3, and record ADR 009"
```
