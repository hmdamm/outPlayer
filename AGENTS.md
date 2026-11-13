# outPlayer (Linux Media Player)

A modern, high-performance Linux media player offering functional, workflow, and hotkey parity with desktop media players, powered by Qt 6 and `libmpv`.

---

## 1. Development & Build System in AntiGravity

AntiGravity operates seamlessly with the underlying Linux toolchain (via WSL2 Kali Linux). All compilation, test execution, and packaging run directly through the integrated terminal:

*   **CMake Configuration**:
    ```bash
    wsl cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug
    ```
*   **Compile / Build**:
    ```bash
    wsl cmake --build build -j$(nproc)
    ```
*   **Run Unit & Integration Tests**:
    ```bash
    wsl ctest --test-dir build --output-on-failure
    ```
*   **Headless GUI Tests (xvfb)**:
    ```bash
    wsl xvfb-run -a ctest --test-dir build --output-on-failure
    ```

---

## 2. Core Architecture

1.  **Core Player Engine (`src/core/`)**:
    *   Thin, idiomatic C++20 RAII wrapper around `libmpv` (`mpv/client.h` and `mpv/render.h`).
    *   Observes mpv properties asynchronously, dispatches Qt signals to the UI thread.
    *   Zero UI dependencies in `core/` to ensure full testability.
2.  **Renderer (`src/render/`)**:
    *   Hardware-accelerated video surface using `mpv_render_context` with Qt Quick OpenGL / Vulkan or QOpenGLWidget.
3.  **UI & Controls (`src/ui/`)**:
    *   Custom frameless player window, minimal OSD, customizable control bar, context menu, and preferences.
    *   Follows Qt6 desktop guidelines and modern styling (`frontend-design` & `coding-guidance-qt`).
4.  **Hotkey & Command Engine (`src/hotkeys/`)**:
    *   Default keybindings mapped to 1:1 PotPlayer defaults (Space: toggle pause, Arrow keys: seek/volume, [ / ]: playback speed, etc.).
    *   Configurable JSON hotkey bindings with action triggers.
5.  **Playlist & History (`src/playlist/`)**:
    *   Qt Model/View architecture (`QAbstractListModel`) for high-performance handling of tens of thousands of media items.

---

## 3. Engineering Protocols & Superpowers

AntiGravity agents working on this project must strictly uphold:

*   **Test-Driven Development (TDD)**: Write reproducible unit/regression tests before writing implementation code (`test-driven-development`).
*   **Planning First**: Produce concrete, structured implementation plans for non-trivial features before modifying code (`writing-plans`).
*   **Reverse Engineering (REA)**: Use REA tools (`reverse-engineer-anything` skill and `rea` MCP server) strictly for black-box analysis of PotPlayer behavior, hotkey mapping, and UI states.
*   **Verification Before Completion**: Run real build and test commands to verify code correctness before claiming completion (`verification-before-completion`).
*   **Qt Best Practices**: Validate C++ and QML using `qt-cpp-review` and `qt-qml-review`.
