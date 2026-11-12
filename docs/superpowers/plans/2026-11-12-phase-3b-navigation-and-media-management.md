# Phase 3B: Navigation & Media Management Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Implement the media navigation, context menu system, open media dialogs, high-performance playlist model/view dock, drag-and-drop pipeline, and mouse gesture integration for outPlayer.

**Architecture:** 
- A unified `ContextMenuManager` dynamically constructs the 1:1 PotPlayer hierarchical context menu linked to `CommandRegistry`.
- An asynchronous media open controller (`MediaOpenController` & `OpenUrlDialog`) handles native file selection, recursive folder scanning, and streaming URL intake.
- A high-performance `PlaylistModel` (`QAbstractListModel`) manages item queues, repeat/shuffle logic, and search filtering.
- A collapsible dark `PlaylistDock` widget snaps to the right of `MainWindow`, toggled via `F6` or the `PL` button.
- Clean integration with `MainWindow`, `VideoWidget`, and `InputManager` connects right-click menus, drag-and-drop media ingestion, and automatic playlist track advance on EOF.

**Tech Stack:** C++20, Qt 6 (Widgets, Core, Gui), libmpv (`IPlaybackEngine`), CMake, Ninja, Xvfb.

**Spec:**
- `docs/spec/features.csv` (Open, Playback, and Playlist features)
- `docs/spec/hotkeys.csv` (Navigation shortcuts: F2, F3, F4, F6, Ctrl+O, Ctrl+U, etc.)
- `docs/spec/ui-layout.md` (Section 9: Playlist Dock Specification)
- `docs/architecture.md` (Component Graph and UI boundaries)

## Global Constraints

- Modern C++20 standard; clean RAII memory management and parent-child `QObject` hierarchies.
- Strict Clean-Room rules: zero occurrences of forbidden proprietary trademarks (`PotPlayer`, `Daum`, `Kakao`) in `src/` or binary outputs.
- Zero UI dependencies (`QWidget`, `QPainter`) in `src/core/` and `src/playlist/PlaylistItem.h`.
- Dark theme palette: base background `#1b1b1b` / `#1e1e1e`, panel background `#252525`, border `#2e2e2e`, accent/amber `#ffaa00` / `#e09b1f`.
- 100% automated test pass rate under WSL2 Kali Linux with headless X11 (`QT_QPA_PLATFORM=xcb xvfb-run -a ctest ...`).

## Review Focus

1. **Massive Playlist Ingestion (10,000+ items):** Queueing large directories must not freeze the GUI thread; `PlaylistModel` must emit accurate `beginInsertRows` / `endInsertRows` and support sub-millisecond lookups.
2. **Context Menu Keyboard Traversal & Action Sync:** All 1:1 menu actions must display registered shortcuts from `CommandRegistry` and execute cleanly without memory leaks or stale action pointers.
3. **URL Validation & Stream History Persistence:** Invalid or malformed streaming URLs in `OpenUrlDialog` must be safely rejected without crashing; valid URLs must be persisted to `SettingsManager`.
4. **Drag-and-Drop Ingestion:** Dropping mixed media files, non-media files, and directories onto `MainWindow` or `PlaylistDock` must gracefully filter unsupported types and enqueue supported formats.
5. **Seamless Track Advance & Teardown Order:** Reaching EOF on an active track must smoothly advance to the next playlist item according to repeat mode; destroying `MainWindow` or `PlaylistDock` must cleanly disconnect engine listeners.

---

### Task 1: Context Menu Architecture & Action Builder (`ContextMenuManager`)

**Files:**
- Create: `src/ui/ContextMenuManager.h`
- Create: `src/ui/ContextMenuManager.cpp`
- Test: `tests/test_context_menu.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `core/CommandRegistry.h`, `core/CommandId.h`, `engine/IPlaybackEngine.h`
- Produces: `outplayer::ui::ContextMenuManager` with:
  ```cpp
  class ContextMenuManager : public QObject {
      Q_OBJECT
  public:
      explicit ContextMenuManager(QObject* parent = nullptr);
      QMenu* buildMainMenu(QWidget* parentWidget, core::IPlaybackEngine* engine = nullptr);
      void showContextMenu(const QPoint& globalPos, QWidget* parentWidget, core::IPlaybackEngine* engine = nullptr);
  signals:
      void openFilesRequested();
      void openFolderRequested();
      void openUrlRequested();
      void togglePlaylistRequested();
      void toggleControlPanelRequested();
      void preferencesRequested();
      void aboutRequested();
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_context_menu.cpp`**
Test that `buildMainMenu` constructs a top-level menu containing the 1:1 sections: "Open", "Playback", "Subtitles", "Video", "Audio", "Window Size", "Playlist", "Control Panel", "Preferences", "About...", "Exit". Verify that actions bound to `CommandRegistry` execute their corresponding command IDs.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `ContextMenuManager.h`).

- [ ] **Step 3: Implement `ContextMenuManager` in `src/ui/ContextMenuManager.h` and `src/ui/ContextMenuManager.cpp`**
Implement the context menu hierarchy with dark styling (`#252525` background, `#383838` hover, `#ffaa00` highlight), wiring actions directly to `core::CommandRegistry::instance().execute(cmdId)` or custom signals for dialog triggers. Add `ui/ContextMenuManager.h` and `ui/ContextMenuManager.cpp` to `outplayer_ui` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_context_menu --output-on-failure'`
Expected: PASS (all menu categories, shortcut bindings, and command execution pass).

- [ ] **Step 5: Commit**
```bash
git add src/ui/ContextMenuManager.h src/ui/ContextMenuManager.cpp tests/test_context_menu.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement ContextMenuManager with 1:1 command registry actions"
```

---

### Task 2: Media Open Dialogs & Streaming URL Controller

**Files:**
- Create: `src/ui/OpenUrlDialog.h`
- Create: `src/ui/OpenUrlDialog.cpp`
- Create: `src/ui/MediaOpenController.h`
- Create: `src/ui/MediaOpenController.cpp`
- Test: `tests/test_media_open_dialogs.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `core/SettingsManager.h`, `engine/IPlaybackEngine.h`
- Produces:
  ```cpp
  class OpenUrlDialog : public QDialog {
      Q_OBJECT
  public:
      explicit OpenUrlDialog(QWidget* parent = nullptr);
      QString url() const;
      void setRecentUrls(const QStringList& urls);
      QStringList recentUrls() const;
  };

  class MediaOpenController : public QObject {
      Q_OBJECT
  public:
      explicit MediaOpenController(QObject* parent = nullptr);
      static QStringList supportedMediaFilter();
      static bool isMediaFile(const QString& filePath);
      static QStringList scanDirectory(const QString& dirPath, bool recursive = true);
      QStringList promptOpenFiles(QWidget* parent);
      QString promptOpenFolder(QWidget* parent);
      QString promptOpenUrl(QWidget* parent);
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_media_open_dialogs.cpp`**
Test `MediaOpenController::isMediaFile()` on common audio/video extensions (`.mp4`, `.mkv`, `.avi`, `.flac`, `.mp3`) and rejection of unsupported files (`.txt`, `.exe`, `.cpp`). Test `scanDirectory()` on temporary test trees. Test `OpenUrlDialog` URL entry, validation, and recent URL list storage.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing dialog classes).

- [ ] **Step 3: Implement `OpenUrlDialog` and `MediaOpenController`**
Implement the styled `OpenUrlDialog` with dark theme inputs and `MediaOpenController` with robust format detection based on Phase 1 format specifications (`docs/spec/formats.csv`). Save recent stream URLs to `core::SettingsManager`. Add sources to `outplayer_ui` in `src/CMakeLists.txt`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_media_open_dialogs --output-on-failure'`
Expected: PASS (format checks, recursive scanner, and dialog properties verified).

- [ ] **Step 5: Commit**
```bash
git add src/ui/OpenUrlDialog.h src/ui/OpenUrlDialog.cpp src/ui/MediaOpenController.h src/ui/MediaOpenController.cpp tests/test_media_open_dialogs.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement OpenUrlDialog and MediaOpenController recursive scanner"
```

---

### Task 3: Playlist Model & Queue Management (`PlaylistModel`)

**Files:**
- Create: `src/playlist/PlaylistItem.h`
- Create: `src/playlist/PlaylistModel.h`
- Create: `src/playlist/PlaylistModel.cpp`
- Test: `tests/test_playlist_model.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Qt6 `QAbstractListModel`, `QFileInfo`
- Produces:
  ```cpp
  namespace outplayer::playlist {

  struct PlaylistItem {
      QString filePath;
      QString title;
      double duration = 0.0;
  };

  enum class RepeatMode { None, RepeatTrack, RepeatAll, Shuffle };

  class PlaylistModel : public QAbstractListModel {
      Q_OBJECT
  public:
      enum Roles {
          FilePathRole = Qt::UserRole + 1,
          TitleRole,
          DurationRole,
          IsActiveRole
      };

      explicit PlaylistModel(QObject* parent = nullptr);
      int rowCount(const QModelIndex& parent = QModelIndex()) const override;
      QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

      void addItem(const PlaylistItem& item);
      void addItems(const QList<PlaylistItem>& items);
      void removeItem(int index);
      void clear();
      void moveItem(int fromIndex, int toIndex);

      int currentIndex() const;
      void setCurrentIndex(int index);
      std::optional<PlaylistItem> currentItem() const;

      int nextIndex() const;
      int previousIndex() const;

      RepeatMode repeatMode() const;
      void setRepeatMode(RepeatMode mode);

      void sortByName(Qt::SortOrder order = Qt::AscendingOrder);
      void sortByDuration(Qt::SortOrder order = Qt::AscendingOrder);
      void shuffle();

      double totalDuration() const;

  signals:
      void currentItemChanged(int index, const PlaylistItem& item);
  };

  } // namespace outplayer::playlist
  ```

- [ ] **Step 1: Write the failing test in `tests/test_playlist_model.cpp`**
Test adding items, removing items, moving items, repeat mode navigation (`None`, `RepeatTrack`, `RepeatAll`, `Shuffle`), sorting by name/duration, total duration calculation, and a stress benchmark inserting 10,000 items in < 50ms.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `PlaylistModel`).

- [ ] **Step 3: Implement `PlaylistItem` and `PlaylistModel` in `src/playlist/`**
Implement `PlaylistModel` with high-performance vector storage, safe bounds checking, and correct Qt model reset/insert/remove signal dispatching. Create library `outplayer_playlist` in `src/CMakeLists.txt` and link to `outplayer_ui` and tests.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_playlist_model --output-on-failure'`
Expected: PASS (100% of unit tests and 10k item benchmark pass).

- [ ] **Step 5: Commit**
```bash
git add src/playlist/PlaylistItem.h src/playlist/PlaylistModel.h src/playlist/PlaylistModel.cpp tests/test_playlist_model.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(playlist): implement high-performance PlaylistModel with queue navigation and sorting"
```

---

### Task 4: Playlist Dock Widget & View (`PlaylistDock`)

**Files:**
- Create: `src/ui/PlaylistDock.h`
- Create: `src/ui/PlaylistDock.cpp`
- Test: `tests/test_playlist_dock.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `playlist/PlaylistModel.h`, `ui/MediaOpenController.h`
- Produces:
  ```cpp
  class PlaylistDock : public QWidget {
      Q_OBJECT
  public:
      explicit PlaylistDock(playlist::PlaylistModel* model, QWidget* parent = nullptr);
      void setSearchFilter(const QString& pattern);
      bool isFilterActive() const;
      int selectedIndex() const;

  signals:
      void playItemRequested(int modelIndex);
      void addFilesRequested();
      void addFolderRequested();
      void addUrlRequested();
      void closeRequested();
  };
  ```

- [ ] **Step 1: Write the failing test in `tests/test_playlist_dock.cpp`**
Test construction with `PlaylistModel`, search filter narrowing visible rows via `QSortFilterProxyModel`, tab switching between Default/Favorites/Albums/History, double-click item emitting `playItemRequested`, and bottom action toolbar buttons.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL with compilation error (missing `PlaylistDock`).

- [ ] **Step 3: Implement `PlaylistDock` in `src/ui/PlaylistDock.h` and `src/ui/PlaylistDock.cpp`**
Assemble the dark styled collapsible panel matching `docs/spec/ui-layout.md` section 9:
- Top tab bar: `[Default] [Favorites] [Albums] [History]` + Close button.
- Real-time search filter input field (`QLineEdit`).
- `QListView` with custom item delegate displaying 2-digit track number, title, formatted duration, and amber `#ffaa00` active playing row with `▶`.
- Bottom action toolbar: `[+] Add`, `[-] Remove`, `[⇅] Sort`, `[☰] Menu`, and status summary label (`N items [HH:MM:SS]`).
- Native drag-and-drop support accepting file/folder drops.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_playlist_dock --output-on-failure'`
Expected: PASS (view rendering, search filtering, action buttons verified).

- [ ] **Step 5: Commit**
```bash
git add src/ui/PlaylistDock.h src/ui/PlaylistDock.cpp tests/test_playlist_dock.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement PlaylistDock with search filter and custom dark delegate"
```

---

### Task 5: MainWindow Integration (ContextMenu, Playlist Dock, Drag-and-Drop & Auto-Advance)

**Files:**
- Modify: `src/ui/MainWindow.h`
- Modify: `src/ui/MainWindow.cpp`
- Test: `tests/test_main_window_navigation.cpp`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `ui/ContextMenuManager.h`, `ui/PlaylistDock.h`, `playlist/PlaylistModel.h`, `ui/MediaOpenController.h`
- Produces: Integrated `MainWindow` coordinating playback, playlist dock toggle via `F6` / `PL` button, right-click context menu, drag-and-drop media loading, and automatic track advance on EOF.

- [ ] **Step 1: Write the failing test in `tests/test_main_window_navigation.cpp`**
Test:
1. `F6` hotkey and `PL` button toggle `PlaylistDock` visibility.
2. Context menu opening on video surface right-click.
3. Drag-and-drop of media files enqueues and plays item.
4. When `IPlaybackEngine` reaches `Ended`, next playlist item automatically loads and plays.
5. `CMD_PLAYLIST_NEXT` (`PgDn`) and `CMD_PLAYLIST_PREV` (`PgUp`) advance playlist queue.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
Expected: FAIL (missing navigation test assertions in `MainWindow`).

- [ ] **Step 3: Implement MainWindow navigation integrations**
In `src/ui/MainWindow.cpp`:
- Add `PlaylistDock` into horizontal layout beside `m_videoWidget`.
- Connect `ContextMenuManager` to `m_videoWidget` right-click and `CustomTitleBar` icon click.
- Wire `F6` shortcut, `PL` button in `m_controlBar`, and `CMD_PLAYLIST_WINDOW` command to toggle `PlaylistDock`.
- Wire `MediaOpenController` for file, folder, and URL open triggers.
- Implement drag-and-drop event filters (`dragEnterEvent`, `dropEvent`) importing media files.
- Wire `IPlaybackEngine::playbackStateChanged` so that transitioning to `State::Ended` triggers `playNext()`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build -R test_main_window_navigation --output-on-failure'`
Expected: PASS (all integration scenarios verified).

- [ ] **Step 5: Commit**
```bash
git add src/ui/MainWindow.h src/ui/MainWindow.cpp tests/test_main_window_navigation.cpp tests/CMakeLists.txt
git commit -m "feat(ui): integrate PlaylistDock, ContextMenuManager, drag-and-drop, and track auto-advance into MainWindow"
```

---

### Task 6: Phase 3B Gate Review, Integration Verification & ADR 007

**Files:**
- Modify: `docs/decisions.md`
- Modify: `TASKS.md`

- [ ] **Step 1: Execute complete test suite and verification commands**
```bash
wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'
wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build --output-on-failure'
wsl bash -c 'python3 tests/test_phase1_specs.py'
wsl bash -c 'xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1'
```
Verify 100% tests pass.

- [ ] **Step 2: Record ADR 007 in `docs/decisions.md`**
Record "ADR 007: Context Menu Hierarchy, Playlist Model/View Dock & Media Ingestion Pipeline" detailing:
- 1:1 Context menu generation bound to `CommandRegistry`.
- `PlaylistModel` design with high-performance vector storage and repeat modes.
- Frameless `PlaylistDock` integration with custom dark delegate and real-time search filtering.
- Asynchronous media ingestion and recursive directory scanning.

- [ ] **Step 3: Update `TASKS.md`**
Mark Phase 3 Slices 3, 4, 5 as Cleared under Phase 3B milestone.

- [ ] **Step 4: Commit**
```bash
git add docs/decisions.md TASKS.md
git commit -m "docs: complete Phase 3B gate review and record ADR 007"
```
