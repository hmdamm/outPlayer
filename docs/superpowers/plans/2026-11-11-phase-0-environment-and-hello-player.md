# Phase 0: Environment Setup & Hello Player Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish the `outPlayer` repository scaffolding, build system (CMake + Ninja + C++20 + Qt 6 + libmpv), governance documentation, and a working "Hello Player" proof-of-concept that initializes `libmpv` with `mpv_render_context` and renders video inside a Qt 6 window.

**Architecture:** A decoupled architecture separating `src/engine/` (`IPlaybackEngine` and `MpvEngine` C++20 RAII wrappers around `libmpv`) from `src/ui/` (`VideoWidget` using `QOpenGLWidget` and `mpv_render_context`). A minimal `MainWindow` shell hosts the video widget to fulfill the Phase 0 gate.

**Tech Stack:** C++20, CMake 3.22+, Ninja, Qt 6 Widgets & OpenGLWidgets, `libmpv` (v0.35+), WSL2 Kali Linux (g++ 14+, ctest, xvfb).

**Spec:** `docs/superpowers/specs/2026-11-11-outplayer-design.md`

## Global Constraints
- Target binary name: `outplayer`.
- Package ID / desktop ID: `org.outplayer.outPlayer` / `outplayer.desktop`.
- Clean-room rules: No decompilation or binary analysis of PotPlayer; no proprietary assets in tree.
- Licensing: Project code licensed under GPL-3.0-or-later; dynamic linking to Qt 6 under LGPL-3.0.
- All build and test execution executed via WSL2 Kali Linux environment (`wsl cmake ...`, `wsl ctest ...`).

## Review Focus
1. `libmpv` OpenGL render context fails to initialize when running headlessly without an active display -> Handled by software OpenGL fallback / xvfb support.
2. Property observation race conditions on `mpv_wait_event` -> Handled by event pump thread and Qt signal marshalling.
3. Memory leaks on player shutdown -> RAII cleanup for `mpv_render_context_free` before `mpv_destroy`.
4. Graceful handling of invalid or non-existent media file paths -> Handled by error signaling without crashing.
5. High-DPI scaling coordinate mismatches in OpenGL viewport -> Handled by devicePixelRatio scaling in paintGL.

---

### Task 1: Scaffolding, Governance Documents & Clean-Room Log

**Files:**
- Create: `TASKS.md`
- Create: `docs/decisions.md`
- Create: `docs/clean-room-log.md`
- Create: `THIRD_PARTY_NOTICES.md`
- Create: `.gitignore`
- Create: `README.md`
- Test: `tests/test_project_scaffold.py`

**Interfaces:**
- Produces: Project governance documents and tracking infrastructure for all subsequent phases.

- [ ] **Step 1: Write test verifying required files and clean-room structure exist**

```python
# tests/test_project_scaffold.py
import os
import sys

required_files = [
    "TASKS.md",
    "docs/decisions.md",
    "docs/clean-room-log.md",
    "THIRD_PARTY_NOTICES.md",
    ".gitignore",
    "README.md",
]

for f in required_files:
    if not os.path.exists(f):
        print(f"Missing required file: {f}")
        sys.exit(1)
print("Scaffold verification passed.")
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python tests/test_project_scaffold.py`  
Expected: FAIL with missing file error.

- [ ] **Step 3: Create the governance files**

- Create `TASKS.md` with Phase 0 through Phase 5 roadmap and evidence tracking columns.
- Create `docs/decisions.md` with initial Architecture Decision Records (ADR 001: Project Name `outPlayer`, ADR 002: Qt 6 Widgets + libmpv render API).
- Create `docs/clean-room-log.md` logging the reference installation path `C:\Program Files\DAUM\PotPlayer` and initial observation plan.
- Create `THIRD_PARTY_NOTICES.md` declaring GPL-3.0-or-later, Qt 6 LGPL-3.0, and libmpv licenses.
- Create `.gitignore` ignoring `build/`, `.cache/`, compile artifacts, and temporary test media.
- Create `README.md` introducing outPlayer, build requirements, and quick-start instructions.

- [ ] **Step 4: Run test to verify it passes**

Run: `python tests/test_project_scaffold.py`  
Expected: `Scaffold verification passed.`

- [ ] **Step 5: Commit**

```bash
git add TASKS.md docs/ THIRD_PARTY_NOTICES.md .gitignore README.md tests/test_project_scaffold.py
git commit -m "chore: scaffold project structure and governance documents"
```

---

### Task 2: CMake Build System with Qt 6 & libmpv

**Files:**
- Create: `CMakeLists.txt`
- Create: `src/CMakeLists.txt`
- Create: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: Qt 6 (`Widgets`, `OpenGLWidgets`), `mpv` (via pkg-config).
- Produces: `outplayer_core` static/object library, `outplayer` executable target, and `ctest` test suite.

- [ ] **Step 1: Write CMakeLists.txt configuration**

Configure `CMakeLists.txt`:
- Require C++20 (`set(CMAKE_CXX_STANDARD 20)`).
- Require `Qt6 COMPONENTS Widgets OpenGLWidgets REQUIRED`.
- Require `PkgConfig` and `pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)`.
- Enable testing with `enable_testing()`.
- Add subdirectories `src` and `tests`.

- [ ] **Step 2: Run CMake configuration in WSL to verify toolchain links**

Run: `wsl cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug`  
Expected: Configuration succeeds with found `Qt6Widgets`, `Qt6OpenGLWidgets`, and `mpv`.

- [ ] **Step 3: Commit**

```bash
git add CMakeLists.txt src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "build: setup root CMake build configuration with Qt 6 and libmpv"
```

---

### Task 3: Core Playback Engine Interface & libmpv Wrapper

**Files:**
- Create: `src/engine/IPlaybackEngine.h`
- Create: `src/engine/MpvEngine.h`
- Create: `src/engine/MpvEngine.cpp`
- Create: `tests/test_mpv_engine.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `mpv/client.h` and `mpv/render.h`.
- Produces:
  ```cpp
  class IPlaybackEngine : public QObject {
      Q_OBJECT
  public:
      virtual bool loadFile(const QString& url) = 0;
      virtual void play() = 0;
      virtual void pause() = 0;
      virtual void togglePause() = 0;
      virtual void stop() = 0;
      virtual void seek(double seconds, bool exact = false) = 0;
      virtual double position() const = 0;
      virtual double duration() const = 0;
      virtual bool isPaused() const = 0;
  signals:
      void positionChanged(double seconds);
      void durationChanged(double seconds);
      void stateChanged(bool paused);
      void fileLoaded();
      void playbackFinished();
  };
  ```

- [ ] **Step 1: Write failing unit test for MpvEngine**

```cpp
// tests/test_mpv_engine.cpp
#include <gtest/gtest.h> // or QTest
#include <QCoreApplication>
#include <QSignalSpy>
#include "engine/MpvEngine.h"

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);
    outplayer::engine::MpvEngine engine;
    if (!engine.initialize()) {
        return 1;
    }
    // Load synthesized lavfi video
    engine.loadFile("lavfi://nullsrc=s=1280x720:d=1");
    return 0;
}
```

- [ ] **Step 2: Run build to verify compilation fails before implementation**

Run: `wsl cmake --build build --target test_mpv_engine`  
Expected: FAIL with missing `MpvEngine.h` or compilation errors.

- [ ] **Step 3: Implement `IPlaybackEngine` and `MpvEngine`**

- `IPlaybackEngine.h`: Pure virtual QObject interface.
- `MpvEngine.h`: Inherits `IPlaybackEngine`, holds `mpv_handle* m_mpv`, event wake thread/loop, property observers for `time-pos`, `duration`, `pause`.
- `MpvEngine.cpp`: Initializes `mpv_create()`, sets `terminal=yes`, `msg-level=all=v`, `vo=libmpv`, calls `mpv_initialize()`. Implements `loadFile()`, `pause()`, `seek()`. Dispatches Qt signals when properties change.

- [ ] **Step 4: Run build and unit test to verify it passes**

Run: `wsl cmake --build build --target test_mpv_engine && wsl ctest --test-dir build -R test_mpv_engine --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/engine/ tests/test_mpv_engine.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(engine): implement IPlaybackEngine and MpvEngine C++20 wrapper"
```

---

### Task 4: VideoWidget (mpv_render_context) & MainWindow Shell

**Files:**
- Create: `src/ui/VideoWidget.h`
- Create: `src/ui/VideoWidget.cpp`
- Create: `src/ui/MainWindow.h`
- Create: `src/ui/MainWindow.cpp`
- Create: `src/main.cpp`
- Create: `tests/test_video_widget.cpp`
- Modify: `src/CMakeLists.txt`
- Modify: `tests/CMakeLists.txt`

**Interfaces:**
- Consumes: `MpvEngine`, `QOpenGLWidget`, `mpv_render_context`.
- Produces: `VideoWidget` rendering frames to screen, `MainWindow` desktop shell, `outplayer` executable.

- [ ] **Step 1: Write test for VideoWidget initialization and rendering**

```cpp
// tests/test_video_widget.cpp
#include <QApplication>
#include <QTest>
#include "ui/VideoWidget.h"
#include "engine/MpvEngine.h"

class TestVideoWidget : public QObject {
    Q_OBJECT
private slots:
    void testInitAndRender();
};

void TestVideoWidget::testInitAndRender() {
    outplayer::engine::MpvEngine engine;
    QVERIFY(engine.initialize());
    outplayer::ui::VideoWidget widget(&engine);
    widget.resize(640, 480);
    widget.show();
    QVERIFY(widget.isVisible());
}
QTEST_MAIN(TestVideoWidget)
#include "test_video_widget.moc"
```

- [ ] **Step 2: Run test build to verify it fails before implementation**

Run: `wsl cmake --build build --target test_video_widget`  
Expected: FAIL with missing `VideoWidget.h`.

- [ ] **Step 3: Implement `VideoWidget`, `MainWindow`, and `main.cpp`**

- `VideoWidget`: Inherits `QOpenGLWidget`. In `initializeGL()`, creates `mpv_render_param` with `MPV_RENDER_PARAM_API_TYPE = MPV_RENDER_API_TYPE_OPENGL`, connects `mpv_render_context_set_update_callback` to Qt's `update()` on the GUI thread. In `paintGL()`, invokes `mpv_render_context_render`.
- `MainWindow`: Inherits `QMainWindow`. Instantiates `MpvEngine` and `VideoWidget` as central widget. Handles command line arguments (e.g. video file to open).
- `main.cpp`: Standard Qt application startup (`QApplication app(argc, argv)`), sets application metadata (`outPlayer`, `org.outplayer.outPlayer`), opens file if passed as argument.

- [ ] **Step 4: Run test under xvfb to verify it passes**

Run: `wsl xvfb-run -a ctest --test-dir build -R test_video_widget --output-on-failure`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add src/ui/ src/main.cpp tests/test_video_widget.cpp src/CMakeLists.txt tests/CMakeLists.txt
git commit -m "feat(ui): implement VideoWidget with mpv_render_context and MainWindow"
```

---

### Task 5: Phase 0 Verification & Gate Clearance

**Files:**
- Modify: `TASKS.md`
- Modify: `docs/decisions.md`

- [ ] **Step 1: Build entire project cleanly in WSL**

Run: `wsl cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && wsl cmake --build build -j$(nproc)`  
Expected: Zero compilation errors and zero warnings.

- [ ] **Step 2: Run full automated test suite under xvfb**

Run: `wsl xvfb-run -a ctest --test-dir build --output-on-failure`  
Expected: 100% of tests passing.

- [ ] **Step 3: Test outplayer executable running with synthetic media**

Run: `wsl xvfb-run -a ./build/src/outplayer --test-run lavfi://nullsrc=s=640x360:d=1`  
Expected: Executable runs, renders frame, and exits cleanly with returncode 0.

- [ ] **Step 4: Update TASKS.md and docs/decisions.md with evidence**

Mark Phase 0 deliverables complete in `TASKS.md` and log test outputs.

- [ ] **Step 5: Commit**

```bash
git add TASKS.md docs/decisions.md
git commit -m "docs: mark Phase 0 complete with verification evidence"
```
