# Phase 2: Core Architecture & Command Engine Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a robust, decoupled architecture for outPlayer consisting of a central Command Registry ingesting all 253 commands from `docs/spec/hotkeys.csv`, an Input Manager supporting rebindable keyboard shortcuts and mouse gestures, a hierarchical Settings Manager persisting 87 preferences to `.ini`, an expanded Playback Engine with track and property controls, and formal architecture documentation.

**Architecture:** Model-View-Controller / Bus architecture. `CommandRegistry` serves as the central application bus where UI controls (menus, toolbars), keyboard shortcuts, mouse gestures, CLI, and MPRIS2 dispatch named actions without tight coupling. `SettingsManager` manages typed settings backed by `QSettings::IniFormat`. `InputManager` routes keyboard events and intercepts mouse actions via event filtering. `MpvEngine` expands to expose track lists, aspect ratios, and asynchronous commands.

**Tech Stack:** C++20, CMake 3.22+, Ninja, Qt 6.5+ (Core, Widgets, Gui, Test), libmpv (0.35+), Python 3 (codegen).

**Spec:** `docs/superpowers/specs/2026-11-11-outplayer-design.md`, `docs/spec/hotkeys.csv`, `docs/spec/features.csv`, `docs/spec/linux-deviations.md`.

## Global Constraints
- Target binary name: `outplayer`.
- Clean-room rules: Strict Implementer role. Implement purely against `docs/spec/*`, Qt 6, and libmpv documentation. Zero proprietary binary assets or symbols.
- Licensing: Code licensed under GPL-3.0-or-later; Qt 6 dynamically linked under LGPL-3.0.
- Decoupling rule: Zero UI dependencies in `src/core/` and `src/engine/` (must link only `Qt6::Core` and `mpv`). UI dependencies remain strictly in `src/ui/` and `src/input/`.
- All build and test execution executed via WSL2 Kali Linux environment (`wsl cmake --build build ...` and `wsl xvfb-run -a ctest ...`).

## Review Focus
1. Command dispatch overhead or memory leaks on custom action callbacks -> Managed via `std::function` and Qt signal/slot connections.
2. Shortcut ambiguity or collision when user rebinds shortcuts -> Handled by conflict detection and scope isolation (`Window`, `Playlist`, `Global`).
3. Mouse gesture race conditions on video surface -> Managed via `QObject::eventFilter` installed on `VideoWidget`.
4. Asynchronous mpv property sync and track list memory corruption -> RAII encapsulation of `mpv_node` track list structures.
5. Thread safety of settings mutations -> Managed on main Qt GUI thread with change notification signals.

---

### Task 1: Command Codegen & `CommandRegistry` Core

**Files:**
- Create: `tools/generate_commands.py`
- Create: `src/core/CommandId.h`
- Create: `src/core/CommandRegistry.h`
- Create: `src/core/CommandRegistry.cpp`
- Create: `tests/test_command_registry.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `docs/spec/hotkeys.csv`.
- Produces:
  ```cpp
  namespace outplayer::core {
  enum class CommandId {
      CMD_PLAY_PAUSE,
      CMD_STOP,
      CMD_SEEK_FORWARD_SHORT,
      // ... all 253 command identifiers
  };

  struct CommandInfo {
      CommandId id;
      QString name;
      QString defaultKey;
      QString defaultModifiers;
      QString scope;
      QString menuPath;
      QString description;
  };

  class CommandRegistry : public QObject {
      Q_OBJECT
  public:
      using Handler = std::function<void(const QVariantMap& params)>;

      static CommandRegistry& instance();
      bool registerHandler(CommandId id, Handler handler);
      bool execute(CommandId id, const QVariantMap& params = {});
      bool execute(const QString& commandIdString, const QVariantMap& params = {});
      const CommandInfo* getInfo(CommandId id) const;
      const CommandInfo* getInfo(const QString& commandIdString) const;
      QList<CommandInfo> allCommands() const;
  signals:
      void commandExecuted(CommandId id, const QVariantMap& params);
  };
  }
  ```

- [ ] **Step 1: Write failing unit test for `CommandRegistry`**

```cpp
// tests/test_command_registry.cpp
#include <QTest>
#include <QSignalSpy>
#include "core/CommandRegistry.h"
#include "core/CommandId.h"

class TestCommandRegistry : public QObject {
    Q_OBJECT
private slots:
    void testCommandCatalog();
    void testExecuteCommand();
    void testExecuteUnknownCommand();
};

void TestCommandRegistry::testCommandCatalog() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    QVERIFY(registry.allCommands().size() >= 250);
    const auto* info = registry.getInfo(outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QVERIFY(info != nullptr);
    QCOMPARE(info->defaultKey, QString("Space"));
}

void TestCommandRegistry::testExecuteCommand() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    bool executed = false;
    registry.registerHandler(outplayer::core::CommandId::CMD_PLAY_PAUSE, [&](const QVariantMap&) {
        executed = true;
    });
    QVERIFY(registry.execute(outplayer::core::CommandId::CMD_PLAY_PAUSE));
    QVERIFY(executed);
}

void TestCommandRegistry::testExecuteUnknownCommand() {
    auto& registry = outplayer::core::CommandRegistry::instance();
    QVERIFY(!registry.execute("CMD_NONEXISTENT_XYZ"));
}

QTEST_MAIN(TestCommandRegistry)
#include "test_command_registry.moc"
```

- [ ] **Step 2: Run test build to verify failure before implementation**

Run: `wsl cmake --build build --target test_command_registry`  
Expected: FAIL with compilation error (missing files).

- [ ] **Step 3: Implement `tools/generate_commands.py`, `CommandId.h`, and `CommandRegistry`**

- `tools/generate_commands.py`: Reads `docs/spec/hotkeys.csv`, generates `src/core/CommandId.h` containing `enum class CommandId` and string conversion tables.
- `src/core/CommandRegistry.h` and `.cpp`: Implements singleton registry, action mapping, string-to-enum resolution, and Qt signal dispatch.
- Update `src/CMakeLists.txt` adding `CommandRegistry.cpp` to `outplayer_core`.
- Update `tests/CMakeLists.txt` adding `test_command_registry`.

- [ ] **Step 4: Run unit tests to verify they pass**

Run: `wsl cmake --build build -j$(nproc) && wsl ctest --test-dir build -R test_command_registry --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add tools/generate_commands.py src/core/CommandId.h src/core/CommandRegistry.h src/core/CommandRegistry.cpp tests/test_command_registry.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(core): implement CommandRegistry and command codegen from hotkeys spec"
```

---

### Task 2: `SettingsManager` & Configuration Persistence

**Files:**
- Create: `src/core/SettingsManager.h`
- Create: `src/core/SettingsManager.cpp`
- Create: `tests/test_settings_manager.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `docs/spec/features.csv` (Preferences category), `docs/spec/linux-deviations.md`.
- Produces:
  ```cpp
  namespace outplayer::core {
  class SettingsManager : public QObject {
      Q_OBJECT
  public:
      static SettingsManager& instance();
      void init(const QString& customConfigPath = {});

      QVariant get(const QString& key, const QVariant& defaultValue = {}) const;
      void set(const QString& key, const QVariant& value);
      bool contains(const QString& key) const;
      void remove(const QString& key);
      void sync();

      bool exportToFile(const QString& filePath) const;
      bool importFromFile(const QString& filePath);
      void resetToDefaults();

      // Strongly-typed accessors for core preferences
      bool allowMultipleInstances() const;
      void setAllowMultipleInstances(bool enable);
      double defaultVolume() const;
      void setDefaultVolume(double vol);
      QString hardwareDecoder() const;
      void setHardwareDecoder(const QString& hwdec);
      bool useSystemWindowDecorations() const;
      void setUseSystemWindowDecorations(bool enable);
  signals:
      void settingChanged(const QString& key, const QVariant& newValue);
  };
  }
  ```

- [ ] **Step 1: Write failing unit test for `SettingsManager`**

```cpp
// tests/test_settings_manager.cpp
#include <QTest>
#include <QTemporaryDir>
#include <QSignalSpy>
#include "core/SettingsManager.h"

class TestSettingsManager : public QObject {
    Q_OBJECT
private slots:
    void testGetAndSet();
    void testSignals();
    void testExportImport();
    void testDefaults();
};

void TestSettingsManager::testGetAndSet() {
    QTemporaryDir tempDir;
    QString iniPath = tempDir.filePath("test_outplayer.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.init(iniPath);
    settings.set("Playback/Volume", 85.0);
    QCOMPARE(settings.get("Playback/Volume").toDouble(), 85.0);
}

void TestSettingsManager::testSignals() {
    auto& settings = outplayer::core::SettingsManager::instance();
    QSignalSpy spy(&settings, &outplayer::core::SettingsManager::settingChanged);
    settings.set("Video/Deinterlace", QString("auto"));
    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.takeFirst().at(0).toString(), QString("Video/Deinterlace"));
}

void TestSettingsManager::testDefaults() {
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.resetToDefaults();
    QCOMPARE(settings.hardwareDecoder(), QString("auto-safe"));
}

void TestSettingsManager::testExportImport() {
    QTemporaryDir tempDir;
    QString exportPath = tempDir.filePath("exported.ini");
    auto& settings = outplayer::core::SettingsManager::instance();
    settings.set("General/CustomSetting", 12345);
    QVERIFY(settings.exportToFile(exportPath));
    settings.set("General/CustomSetting", 0);
    QVERIFY(settings.importFromFile(exportPath));
    QCOMPARE(settings.get("General/CustomSetting").toInt(), 12345);
}

QTEST_MAIN(TestSettingsManager)
#include "test_settings_manager.moc"
```

- [ ] **Step 2: Run test build to verify failure before implementation**

Run: `wsl cmake --build build --target test_settings_manager`  
Expected: FAIL with compilation error (missing files).

- [ ] **Step 3: Implement `SettingsManager`**

- `src/core/SettingsManager.h` & `.cpp`: Implements `QSettings::IniFormat` file backend with fallback to `$XDG_CONFIG_HOME/outplayer/outplayer.ini`. Enforces defaults matching `docs/spec/features.csv` (e.g. `hwdec=auto-safe`, `audio-channels=stereo`, `sub-font=sans-serif`).
- Update `src/CMakeLists.txt` adding `SettingsManager.cpp` to `outplayer_core`.
- Update `tests/CMakeLists.txt` adding `test_settings_manager`.

- [ ] **Step 4: Run unit tests to verify they pass**

Run: `wsl cmake --build build -j$(nproc) && wsl ctest --test-dir build -R test_settings_manager --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/core/SettingsManager.h src/core/SettingsManager.cpp tests/test_settings_manager.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(core): implement SettingsManager with .ini persistence and typed preferences"
```

---

### Task 3: `InputManager` (Keymap Router & Mouse Gestures)

**Files:**
- Create: `src/input/InputManager.h`
- Create: `src/input/InputManager.cpp`
- Create: `tests/test_input_manager.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `CommandRegistry`, `SettingsManager`, `QShortcut`, `QMouseEvent`, `QWheelEvent`.
- Produces:
  ```cpp
  namespace outplayer::input {
  enum class MouseAction {
      LeftClick,
      LeftDblClick,
      MiddleClick,
      RightClick,
      WheelUp,
      WheelDown
  };

  class InputManager : public QObject {
      Q_OBJECT
  public:
      explicit InputManager(QWidget* parentWindow = nullptr);
      void attachToWindow(QWidget* window);
      void attachMouseFilter(QWidget* surface);

      bool setCustomShortcut(core::CommandId id, const QKeySequence& shortcut);
      QKeySequence getShortcut(core::CommandId id) const;
      void resetShortcut(core::CommandId id);

      void setMouseMapping(MouseAction action, core::CommandId commandId);
      core::CommandId getMouseMapping(MouseAction action) const;
  protected:
      bool eventFilter(QObject* watched, QEvent* event) override;
  signals:
      void mouseActionTriggered(MouseAction action, core::CommandId commandId);
  };
  }
  ```

- [ ] **Step 1: Write failing unit test for `InputManager`**

```cpp
// tests/test_input_manager.cpp
#include <QTest>
#include <QWidget>
#include <QSignalSpy>
#include "input/InputManager.h"
#include "core/CommandRegistry.h"

class TestInputManager : public QObject {
    Q_OBJECT
private slots:
    void testDefaultShortcutsLoaded();
    void testRebindShortcut();
    void testMouseFilter();
};

void TestInputManager::testDefaultShortcutsLoaded() {
    QWidget dummyWindow;
    outplayer::input::InputManager inputMgr(&dummyWindow);
    QKeySequence seq = inputMgr.getShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QCOMPARE(seq, QKeySequence(Qt::Key_Space));
}

void TestInputManager::testRebindShortcut() {
    QWidget dummyWindow;
    outplayer::input::InputManager inputMgr(&dummyWindow);
    QVERIFY(inputMgr.setCustomShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE, QKeySequence(Qt::Key_P)));
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE), QKeySequence(Qt::Key_P));
    inputMgr.resetShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE);
    QCOMPARE(inputMgr.getShortcut(outplayer::core::CommandId::CMD_PLAY_PAUSE), QKeySequence(Qt::Key_Space));
}

void TestInputManager::testMouseFilter() {
    QWidget dummySurface;
    outplayer::input::InputManager inputMgr;
    inputMgr.attachMouseFilter(&dummySurface);
    inputMgr.setMouseMapping(outplayer::input::MouseAction::WheelUp, outplayer::core::CommandId::CMD_VOLUME_UP);
    QSignalSpy spy(&inputMgr, &outplayer::input::InputManager::mouseActionTriggered);
    // Simulate mouse wheel up event
    QWheelEvent wheelEv(QPointF(10, 10), QPointF(10, 10), QPoint(0, 120), QPoint(0, 120),
                        Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
    QApplication::sendEvent(&dummySurface, &wheelEv);
    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(TestInputManager)
#include "test_input_manager.moc"
```

- [ ] **Step 2: Run test build to verify failure before implementation**

Run: `wsl cmake --build build --target test_input_manager`  
Expected: FAIL with compilation error (missing files).

- [ ] **Step 3: Implement `InputManager`**

- `src/input/InputManager.h` & `.cpp`:
  - Dynamically builds `QShortcut` instances attached to `parentWindow` based on default and customized keybindings from `hotkeys.csv` / `SettingsManager`.
  - Implements `eventFilter()` to capture `QEvent::MouseButtonPress`, `QEvent::MouseButtonDblClick`, and `QEvent::Wheel`, translating them to registered `CommandId` actions.
- Update `src/CMakeLists.txt` creating static library `outplayer_input` linking `outplayer_core` and `Qt6::Widgets`.
- Update `tests/CMakeLists.txt` adding `test_input_manager`.

- [ ] **Step 4: Run unit tests under xvfb to verify they pass**

Run: `wsl cmake --build build -j$(nproc) && wsl xvfb-run -a ctest --test-dir build -R test_input_manager --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/input/InputManager.h src/input/InputManager.cpp tests/test_input_manager.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(input): implement InputManager with rebindable QShortcuts and mouse gesture filtering"
```

---

### Task 4: Playback Engine Track & Property Expansion

**Files:**
- Modify: `src/engine/IPlaybackEngine.h`
- Modify: `src/engine/MpvEngine.h`
- Modify: `src/engine/MpvEngine.cpp`
- Modify: `tests/test_mpv_engine.cpp`

**Interfaces:**
- Produces:
  ```cpp
  namespace outplayer::engine {
  struct MediaTrack {
      int id{-1};
      QString type; // "video", "audio", "sub"
      QString title;
      QString language;
      QString codec;
      bool selected{false};
      bool isDefault{false};
  };

  struct ChapterInfo {
      int index{0};
      QString title;
      double time{0.0};
  };
  }

  // In IPlaybackEngine:
  virtual QList<MediaTrack> tracks(const QString& type = {}) const = 0;
  virtual bool setTrack(const QString& type, int trackId) = 0;
  virtual double volume() const = 0;
  virtual void setVolume(double vol) = 0;
  virtual bool isMuted() const = 0;
  virtual void setMuted(bool mute) = 0;
  virtual double speed() const = 0;
  virtual void setSpeed(double spd) = 0;
  virtual QString aspectRatio() const = 0;
  virtual void setAspectRatio(const QString& ar) = 0;
  virtual QList<ChapterInfo> chapters() const = 0;
  virtual bool setChapter(int chapterIndex) = 0;

  signals:
  void tracksChanged();
  void volumeChanged(double vol);
  void muteChanged(bool muted);
  void speedChanged(double spd);
  void chaptersChanged();
  ```

- [ ] **Step 1: Extend `test_mpv_engine.cpp` with track and property assertions**

Add tests for volume adjustment, mute toggle, speed adjustment (0.5x, 1.0x, 2.0x), aspect ratio property setting, and track enumeration.

- [ ] **Step 2: Run test build to verify it fails before implementation**

Run: `wsl cmake --build build --target test_mpv_engine`  
Expected: FAIL with compilation error on missing methods.

- [ ] **Step 3: Implement track and property extensions in `IPlaybackEngine` and `MpvEngine`**

- `MpvEngine`: Observe properties `volume`, `mute`, `speed`, `track-list`, `chapter-list`. Parse `mpv_node` for tracks into `QList<MediaTrack>`. Return cached values for property queries to avoid synchronous thread blocks.
- Implement command execution via `mpv_command_async`.

- [ ] **Step 4: Run unit tests under xvfb to verify they pass**

Run: `wsl cmake --build build -j$(nproc) && wsl xvfb-run -a ctest --test-dir build -R test_mpv_engine --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/engine/IPlaybackEngine.h src/engine/MpvEngine.h src/engine/MpvEngine.cpp tests/test_mpv_engine.cpp
git commit -m "feat(engine): expand IPlaybackEngine with tracks, chapters, volume, and async commands"
```

---

### Task 5: Architecture Documentation & Phase 2 Gate Review

**Files:**
- Create: `docs/architecture.md`
- Modify: `TASKS.md`
- Modify: `docs/decisions.md`

- [ ] **Step 1: Author `docs/architecture.md`**

Document the overall application architecture with Mermaid diagrams:
- System component graph (`Engine`, `Core`, `Input`, `UI`, `Integration`).
- Data & Command Flow diagram showing how user input dispatches via `CommandRegistry` to `MpvEngine` and `VideoWidget`.
- Dependency boundaries ensuring `src/core` and `src/engine` remain zero-UI dependencies.

- [ ] **Step 2: Run full build and test suite**

Run: `wsl cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && wsl cmake --build build -j$(nproc)`  
Run: `wsl xvfb-run -a ctest --test-dir build --output-on-failure`  
Run: `python tests/test_phase1_specs.py`  
Expected: 100% tests passing across all test suites.

- [ ] **Step 3: Record ADR 005 in `docs/decisions.md` and update `TASKS.md`**

Add ADR 005: Core Architecture & Command Engine Foundation. Mark Phase 2 tasks P2-T1 through P2-T5 as `Done` in `TASKS.md`.

- [ ] **Step 4: Commit**

```bash
git add docs/architecture.md TASKS.md docs/decisions.md
git commit -m "docs: author architecture documentation and record ADR 005"
```
