# Clean-Room Engineering & Observation Log

This log documents all clean-room procedures, reference environment configurations, observer-implementer separation boundaries, and observation sessions for the **outPlayer** project.

---

## 1. Clean-Room Methodology & Governance Rules

To ensure that **outPlayer** is an entirely original, legally independent work, all development strictly adheres to an audited clean-room methodology:

1. **Role Separation**:
   - **Observer Role (Black-Box Analyst)**:
     - Operates solely within a user-facing external environment (Windows host or virtual machine).
     - Interacts with reference software strictly as an ordinary end-user.
     - Documents visible user interfaces, keybindings, menu layouts, command naming, and behavioral responses into public technical specifications in `docs/spec/`.
     - Logs all observation sessions in this file.
   - **Implementer Role (Software Engineer)**:
     - Works solely within the Linux/WSL2 development workspace.
     - Authors original source code based exclusively on specifications in `docs/spec/`, public Qt 6 documentation, and open-source `libmpv`/FFmpeg APIs.
     - Never views or requests disassembly or decompilation of the reference software.

2. **Prohibited Activities (Strictly Enforced)**:
   - **NO** binary disassembly or decompilation (e.g., IDA Pro, Ghidra, radare2, x64dbg).
   - **NO** runtime memory inspection, debugger attachment, API hooking, or DLL injection.
   - **NO** extraction, copying, or embedding of proprietary assets (icons, bitmaps, skins, sound effects, strings, or compiled dialog resources).
   - **NO** reuse of closed-source Windows filter binaries or proprietary codec modules.

3. **Permitted Information Sources**:
   - Visual inspection of the running reference application UI and menus.
   - Published user manuals, public hotkey cheatsheets, and official public release notes.
   - Open standards specifications (FreeDesktop MPRIS2, XDG Portals, ASS/SSA subtitle specs, FFmpeg/libmpv documentation, Qt 6 API documentation).

---

## 2. Reference Environment Details

- **Host Operating System:** Windows 10/11 64-bit
- **Reference Application:** Daum PotPlayer (64-bit edition)
- **Reference Installation Directory:** `C:\Program Files\DAUM\PotPlayer`
- **Reference Executable:** `PotPlayer64.exe`
- **Execution Mode:** Unmodified standard installation with default settings

---

## 3. Observation Plan & Phase 1 Scope

The observer role is scheduled to execute black-box behavioral captures according to the following plan:

| Session Target | Deliverable Specification | Target Content |
|---|---|---|
| **Main & Context Menus** | `docs/spec/features.csv` | Full menu tree, accelerator keys, separator placements, sub-menus. |
| **Default Hotkey Map** | `docs/spec/hotkeys.csv` | Default keyboard shortcuts, mouse click/wheel actions, chord combinations. |
| **Preferences Hierarchy** | `docs/spec/features.csv`, `docs/spec/linux-deviations.md` | Complete F5 settings tree, parameter types, Linux-equivalent mappings, Windows-specific deviations. |
| **Auxiliary Windows** | `docs/spec/ui-layout.md` | Playlist window (F6), Control Panel (F7), Equalizer layout, dimensions, docking behavior. |
| **OSD & Feedback Engine** | `docs/spec/ui-layout.md` | On-screen display text formatting, positioning, colors, duration. |
| **CLI & File Associations** | `docs/spec/cli.md`, `docs/spec/formats.md` | Command-line arguments, playback switches, supported container/codec list. |

---

## 4. Observation Log Entries

### Entry CR-001: Project Inception & Reference Verification
- **Date:** 2026-11-11
- **Role:** Observer
- **Reference Target:** `C:\Program Files\DAUM\PotPlayer\PotPlayer64.exe`
- **Action:** Verified presence and integrity of clean reference installation at target path. Confirmed default settings baseline.
- **Artifacts Produced:** Established clean-room governance charter and observation protocol in `docs/clean-room-log.md`.
- **Integrity Statement:** No reverse engineering, decompilation, or binary analysis tools were executed against the reference binary. All specifications will be generated through visual observation of running application behavior.

### Entry CR-002: Command-Line Interface Documentation Audit
- **Date:** 2026-11-11
- **Role:** Observer
- **Reference Target:** `C:\Program Files\DAUM\PotPlayer\CmdLine64.txt`
- **Action:** Audited and cataloged the complete command-line interface documentation shipped with the reference installation across all language code-pages (CP0 English, CP949 Korean, etc.). Mapped all 28 CLI switches, parameters, argument formats, and stdin pipe behaviors to Linux-standard GNU long options and compatibility aliases.
- **Artifacts Produced:** Authored `docs/spec/cli.md` (Specification SPEC-CLI-001).
- **Integrity Statement:** Derived entirely from shipped plain-text documentation (`CmdLine64.txt`). No disassembly, decompilation, debugging, or memory tracing of executable binaries was performed.

### Entry CR-003: Media Formats & Streaming Protocols Matrix
- **Date:** 2026-11-11
- **Role:** Observer
- **Reference Target:** PotPlayer UI File Association Preferences (`F5 > Association`) and public `libmpv`/FFmpeg/libass specifications.
- **Action:** Cataloged visible container types, video codecs, audio decoders, subtitle renderers, and network streaming protocols supported by the reference player. Formulated a 1:1 Linux compatibility matrix powered by `libmpv` and modern FFmpeg decoders with VA-API / NVDEC hardware acceleration.
- **Artifacts Produced:** Authored `docs/spec/formats.md` (Specification SPEC-FORMATS-001).
- **Integrity Statement:** Specifications established through non-invasive black-box UI inspection of public file association dialogues and published open-source codec specifications. No binary reverse engineering was performed.

### Entry CR-004: Context Menu & Main Menu Feature Inventory
- **Date:** 2026-11-11
- **Role:** Observer
- **Reference Target:** PotPlayer Context / Main Menu UI and plain-text language definitions (`C:\Program Files\DAUM\PotPlayer\Language\English.ini`).
- **Action:** Audited and cataloged the comprehensive menu hierarchy of the reference player across all core operational areas: Open, Play, Subtitles, Video, Audio, Filters, Bookmarks, Capture, Skins, Aspect Ratio, Window Size, and Help. Extracted menu paths, default keyboard shortcuts, default states, and observed functional behaviors into a structured, machine-verifiable CSV dataset. Formulated modern Linux-native implementation mappings using Qt 6 actions, `libmpv` properties/commands, and FreeDesktop/XDG standards.
- **Artifacts Produced:** Authored `docs/spec/features.csv` (258 feature entries across 12 functional areas).
- **Integrity Statement:** Derived strictly from non-invasive black-box UI observation and human-readable plain-text localization strings (`English.ini`). No binary decompilation, disassembly, API hooking, or memory inspection was performed.

### Entry CR-005: Default Keyboard Shortcuts & Mouse Action Mapping
- **Date:** 2026-11-12
- **Role:** Observer
- **Reference Target:** PotPlayer Preferences (`F5 > General > Keyboard`), Mouse Settings (`F5 > General > Mouse`), and plain-text configuration definitions (`C:\Program Files\DAUM\PotPlayer\Language\English.ini`).
- **Action:** Audited and cataloged the complete default keyboard shortcuts and mouse input mapping of the reference player. Documented all default playback controls (Space, Left/Right seeks, frame steps, speed adjustments), A-B repeat ranges, audio volume and equalizer adjustments, subtitle timing/sync/position controls, video effects/shaders, window sizing/positioning presets, capture actions, dialog invocations, playlist-internal shortcuts, mouse wheel/click behaviors, and global multimedia hotkeys into a structured, machine-verifiable CSV specification (`docs/spec/hotkeys.csv`). Mapped each shortcut to clean, idiomatic command identifiers (`CMD_*`) suitable for input dispatching in Qt 6 and `libmpv`.
- **Artifacts Produced:** Authored `docs/spec/hotkeys.csv` (253 shortcut, gesture, and mouse action mappings).
- **Integrity Statement:** Derived exclusively through non-invasive black-box UI observation of preferences dialogs, published keyboard cheatsheets, and human-readable plain-text localization strings (`English.ini`). No binary disassembly, decompilation, debugging, memory tracing, or reverse-engineering tools were executed against executable binaries.

### Entry CR-006: Preferences (F5) Hierarchy & Linux Architecture Deviations Audit
- **Date:** 2026-11-12
- **Role:** Observer
- **Reference Target:** PotPlayer Preferences Dialog (`F5`), configuration sections across all primary categories (General, Playback, Video, Audio, Subtitles, Filter, Association), and Linux platform subsystem standards (Wayland, X11, PipeWire, PulseAudio, ALSA, VA-API, NVDEC, XDG Desktop Portals, XDG Base Directory specification).
- **Action:** Audited and cataloged the comprehensive Preferences configuration hierarchy (F5) of the reference player across all primary categories and subcategories (General: Startup, Skin, Keyboard, Mouse, OSD; Playback: Aspect Ratio, Jump, Speed, Time; Video: Renderers, Colors, Deinterlacing, HW Acceleration; Audio: Renderers, Equalizer, Normalizer, Volume; Subtitles: Font, Location, Sync, Processing; Filter: Priority; Association: File Types). Extracted parameter types, default values, and operational behaviors into `docs/spec/features.csv` (87 new preference settings rows, bringing total feature specifications to 345). Conducted an exhaustive technical audit documenting architectural divergences and native Linux adaptations where direct Windows subsystem replication (DirectShow filter graphs, Direct3D exclusive fullscreen, WASAPI exclusive bitstreaming, Windows Registry, Win32 `RegisterHotKey`) is technically invalid, insecure, or obsolete. Mapped each subsystem to modern Linux native primitives: `libmpv` `mpv_render_context` with OpenGL/Vulkan, VA-API/NVDEC hardware decoding, PipeWire/PulseAudio/ALSA audio dispatching, Wayland borderless fullscreen (`xdg_toplevel.set_fullscreen()`), XDG Base Directory `QSettings(QSettings::IniFormat)` configuration, and XDG Desktop Portals (`org.freedesktop.portal.GlobalShortcuts`) / `XGrabKey`.
- **Artifacts Produced:** Authored `docs/spec/linux-deviations.md` (Specification SPEC-LINUX-001) and appended 87 preference feature specifications to `docs/spec/features.csv`.
- **Integrity Statement:** Specifications derived exclusively from non-invasive black-box UI observation of running preferences dialogs, published FreeDesktop/Wayland specifications, and open-source `libmpv`/FFmpeg architectural documentation. No binary reverse engineering, decompilation, disassembly, or proprietary filter extraction was performed.

### Entry CR-007: UI Layout, Themes, Playlist & Control Panel Audit
- **Date:** 2026-11-12
- **Role:** Observer
- **Reference Target:** PotPlayer Main Window, Titlebar, Control Bar, Time Slider, On-Screen Display (OSD), Playlist Dock (`F6`), Control Panel (`F7`), and Graphic Equalizer.
- **Action:** Audited and cataloged visual layouts, geometry metrics, color palettes, typography, interactive states, and auxiliary window docking structures of the reference player. Documented exact dimensions (window minimum size 320x180, titlebar height 30px, control bar height 40px, time slider height 14px with 4px groove, button dimensions 24x24px / 28x28px), dark theme color tokens (`#1b1b1b`, `#1e1e1e`, `#252525`, `#383838`, `#ffaa00`, `#e09b1f`, `#ffffff`, `#aaaaaa`, `#d48800`), font hierarchy, OSD typography and placement rules, multi-tab playlist structure (`Default`, `Favorites`, `Albums`, `History`), and 4-tab control panel functionality (Video adjustments, 10-band audio graphic equalizer, Subtitle sync, Playback speed & A-B repeat). Formulated clean-room implementation guidelines ensuring 100% original QSS/CSS styling, SVG iconography, and Qt 6 architecture without extracting or embedding proprietary binary assets.
- **Artifacts Produced:** Authored `docs/spec/ui-layout.md` (Specification SPEC-UI-001).
- **Integrity Statement:** Specifications established exclusively through non-invasive black-box UI observation of running application windows, measurement of screen geometry, and standard colorimetric sampling. No binary disassembly, decompilation, DLL injection, resource extraction, or proprietary skin/bitmap conversion was performed.


