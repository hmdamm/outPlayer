# MASTER PROMPT: Native Linux media player with PotPlayer-level features (clean-room)

Paste everything below this line into your AI agent (Claude Code recommended). Replace `<APP_NAME>` with your own product name first.

---

## 0. Mission

You are the lead engineer on `<APP_NAME>`, a native Linux media player for Fedora, Ubuntu/Debian, Arch and their derivatives. It must reproduce the **feature set, default hotkeys, workflow and general look-and-feel** of PotPlayer 64-bit for Windows, and it must play every format FFmpeg/libmpv can handle. No Linux player currently combines PotPlayer's depth of features; closing that gap is the whole point.

You work autonomously. Make decisions yourself, record each one as a short entry in `docs/decisions.md` (context, decision, alternatives), and only stop to ask me when you are truly blocked. Keep `TASKS.md` current (task, status, evidence) so I can see progress when I return.

## 1. Ground rules (non-negotiable)

### 1.1 Clean-room method
PotPlayer is proprietary and its EULA forbids modifying, reverse engineering, decompiling or disassembling the software. This project therefore uses **black-box observation plus fresh implementation**, in two strictly separated roles:

- **Observer role** (writes specs only): runs PotPlayer, uses its UI, reads its Preferences pages, its own hotkey table, its menus and context menus, its documented command-line options, release notes and the official site. Records *externally visible behavior* in `docs/spec/` in your own words.
- **Implementer role** (writes code only): builds from `docs/spec/` and open-source references (mpv, FFmpeg, libass, Qt docs). Never looks at anything PotPlayer-internal.

Forbidden on PotPlayer executables, DLLs, filters, drivers and installer payloads: disassembly, decompilation, debugger attach, hooking, patching, memory inspection, and native-binary analysis modes (Ghidra/Hopper/IDA/radare2 providers, PE import/export dumps, string extraction). Also do not copy, trace or convert its icons, bitmaps, skins, sounds, logos, fonts, translations or installer assets.

Allowed: installing and running it, screenshots/screen recordings for your own reference, listing the install folder's file names/sizes/version fields to learn which components ship, and exporting settings through the app's own UI (Preferences, "Store settings into .ini file").

Keep `docs/clean-room-log.md`: every observation source (screen, menu path, URL, date) behind every spec item.

### 1.2 What "exact" means here
- **Hotkeys, mouse gestures, menu structure, command names, settings tree, default values, window/OSD/control-bar layout and behavior:** match the original exactly. These are functional.
- **Pixels and artwork:** match proportions, layout, color scheme and control placement as closely as you can, using **original** icons, glyphs and graphics you create (SVG). Never copy an asset to hit pixel parity.
- **Branding:** do not use "PotPlayer", "Daum" or "Kakao" in the product name, binary name, icons, desktop file, package IDs or UI strings. README may say the player offers a familiar workflow for people coming from other players.
- Where Linux physically cannot match (desktop-reserved shortcuts, Wayland limits), document the exception in `docs/spec/linux-deviations.md` with the closest alternative.

### 1.3 Licensing and privacy
Audit every dependency's license; maintain `THIRD_PARTY_NOTICES.md`. Default app license: GPL-3.0-or-later (compatible with GPL-built libmpv/FFmpeg); Qt used under LGPL with dynamic linking. No telemetry, no network calls except what the user triggers (streams, subtitle search, update check that is opt-in).

## 2. Inputs

- Windows installer (verify exact filename and extension, it may be `PotPlayerSetup64 (1).exe`): `C:\Users\hmdam\Downloads\PotPlayerSetup64 (1)`
- Cached/downloaded files: `C:\Users\hmdam\AppData\Local\Temp\PotUpdate` (my message contained a stray space after `Temp`; verify the real path)
- Official site: https://potplayer.tv/
- Under WSL these are at `/mnt/c/Users/hmdam/...`. If you cannot reach the Windows host or run the GUI from your environment, ask me once for screenshots, a screen recording, or the exported `.ini`, and drop them in `./reference/`.

Use these only under section 1.1. Prefer installing PotPlayer in a disposable Windows VM or Windows Sandbox.

## 3. Pre-approved technology decisions

The original is C++ built with MSVC, a Win32 UI, Direct3D/DXVA rendering and FFmpeg-based demux/decoding. Staying with **C++** is the right call, with Linux equivalents:

| Original (Windows) | `<APP_NAME>` (Linux) |
|---|---|
| MSVC, Win32 UI | C++20, GCC/Clang, CMake + Ninja, **Qt 6** (Widgets with custom-painted controls; QtQuick only where needed) |
| Direct3D renderer | **libmpv render API** (OpenGL first; Vulkan via libplacebo as option) behind an `IPlaybackEngine` interface so the engine can be swapped later |
| DXVA / D3D11VA decode | VA-API, NVDEC, VDPAU, Vulkan video via `hwdec=auto-safe`, with a per-API selector in Preferences |
| FFmpeg-derived demux/decode | FFmpeg through libmpv; direct FFmpeg libs for metadata, thumbnails, capture, remux/encode |
| DirectShow/HLSL filters, shaders | mpv user shaders (GLSL), libplacebo scalers/tone-mapping, FFmpeg filter graphs |
| Audio (WASAPI etc.) | PipeWire, PulseAudio, ALSA, JACK through mpv; passthrough where supported |
| Subtitles | libass (ASS/SSA/SRT/PGS/VobSub etc. via mpv) plus own styling UI |
| Tray, media keys, power inhibit | StatusNotifierItem, MPRIS2, `org.freedesktop.portal.Inhibit` |
| Global hotkeys | XDG GlobalShortcuts portal on Wayland, XGrabKey on X11 |
| TV/capture devices | V4L2, ALSA/PipeWire capture |
| DVD/Blu-ray | libdvdnav, libbluray (note libdvdcss legality varies by country; make it optional) |

Support **both Wayland and X11**. Deviations from this table require an ADR.

## 4. Skills and plugins: invoke them at these moments

Slash names can differ slightly per install; run `/skills` once at the start and use the exact names that appear. Skills also auto-trigger from their descriptions, but call them explicitly at the gates below.

| When | Invoke | Used for |
|---|---|---|
| Start of each phase | `/superpowers:brainstorm` then `/superpowers:write-plan` | Refine design, produce a task plan |
| Executing plans | `/superpowers:execute-plan`; skills `subagent-driven-development`, `dispatching-parallel-agents` | Parallelize independent features across subagents |
| Every feature | `test-driven-development` | Failing test first, then code |
| Any bug or crash | `systematic-debugging` | Root-cause before fixing |
| Before each merge/phase gate | `requesting-code-review`, `verification-before-completion` | Evidence over claims |
| Phase 1 (observation) | `/reverse-engineer-anything` (REA) **restricted to black-box use**: install-folder inventory and observing application behavior. Native-binary providers are disabled by section 1.1 | Structured behavior capture with evidence |
| Phase 3 onward, Qt code | `coding-guidance-qt`, Qt Development Skills plugin (Qt/QML code review, QML coding) | Idiomatic Qt, model/view, threading, CMake |
| Documenting classes/APIs | `qt-cpp-docs` | Reference docs |
| UI design system and any web docs page | `/frontend-design` | Design tokens, typography, color, layout rationale. It targets web UI, so apply its principles to Qt styling and use its output directly only for the project website |
| Creating project-specific skills | `/skill-creator` | Build the custom skills in section 7 |

Reference repos to study (code you may read freely): mpv-player/mpv and mpv-player/mpv-examples (Qt/QML embedding via the render API), FFmpeg, libass, libplacebo, SMPlayer, Celluloid, VLC.

## 5. Phases and gates

### Phase 0: Environment
- Linux dev environment: WSL2 Ubuntu (WSLg shows GUI) or a Linux VM. Add `distrobox`/containers for `fedora:latest`, `ubuntu:24.04`, `archlinux:latest`.
- Repo layout: `src/`, `ui/`, `engine/`, `docs/spec/`, `tests/`, `packaging/{flatpak,rpm,deb,arch,appimage}`, `reference/`, `.github/workflows/` (build+test matrix over the three distros).
- Gate: "hello player" opens a file via libmpv in a Qt window on the three distro containers.

### Phase 1: Black-box feature inventory (Observer role)
Produce, in `docs/spec/`:
1. `features.csv`: id, area, feature, how to reach it (menu path/hotkey), default, observed behavior, evidence ref, status (todo/partial/done/deviation). Walk **every** menu, submenu, context menu, right-click target, Preferences page (F5 and its whole tree), control panel (F7), playlist (F6), OSD, tray menu, drag-drop behavior and mouse-wheel/gesture behavior. Seed areas (verify and extend; do not trust this list): playback/seek granularity, A-B repeat, speed, frame step, bookmarks/chapters, playlist and its properties, capture (screenshot, clip, GIF), video renderer/filters/shaders/aspect/zoom/rotate/3D/360, audio (EQ, normalize, channel mapping, passthrough, delay), subtitles (embedded/external/online search/style/sync/position), OSD, skins/themes, URL/stream/DVD/Blu-ray/TV, command line, settings import/export, tray, multi-instance, language packs.
2. `hotkeys.csv`: **every** default hotkey and mouse action as shown in Preferences > General > Keyboard (and the playlist hotkeys), with function name, key, modifiers, scope (global/window/playlist), and menu path. Capture by screenshot and transcription from the app's own table, plus the settings export. Include unassigned-by-default commands because users can bind them.
3. `ui-layout.md` plus annotated screenshots: main window, control bar, OSD, playlist, Preferences, dialogs; dimensions, spacing, colors (as hex samples), fonts' roles, states (hover/pressed/disabled), the default skin's structure.
4. `formats.md`: container/codec/subtitle/playlist/protocol matrix of what the original plays (from its documentation and tests with sample files you generate yourself with `ffmpeg -f lavfi`).
5. `cli.md`: command-line switches, file associations, integration points.
- Gate: coverage report shows every visible menu item and every Preferences control appears in `features.csv`; each row has an evidence reference in the clean-room log.

### Phase 2: Architecture
- Brainstorm, then plan. Define modules: `engine` (IPlaybackEngine + mpv implementation), `core` (state, command bus, settings), `ui` (main window, control bar, OSD, playlist, preferences), `input` (hotkey map, mouse gestures, MPRIS, global shortcuts), `subs`, `capture`, `devices`, `integration` (portal/D-Bus/tray), `i18n`.
- Single **command registry**: every feature is a named command with ID; menus, hotkeys, gestures, CLI and MPRIS all dispatch through it. Hotkey maps load from `hotkeys.csv` at build time (generated code or resource), user-rebindable, import/export compatible with a documented format.
- Gate: architecture doc + dependency diagram reviewed with `requesting-code-review`.

### Phase 3: Vertical slices (parallelize with subagents)
Order: playback core and seeking, control bar/OSD, hotkeys, playlist, audio, video renderer options, subtitles, bookmarks/capture, Preferences, devices/discs/streams, theming/skins, tray/MPRIS/portals, i18n, CLI. Each slice: spec rows to tests to code to review to mark `done` in `features.csv`.

### Phase 4: Parity hardening
- **Hotkey parity test:** generate QTest cases from `hotkeys.csv` that send each key event and assert the correct command fires, plus a conflict detector and a report of any binding the desktop environment intercepts (GNOME/KDE/Wayland). Target: 100% of rows pass or are listed in `linux-deviations.md`.
- **UI parity review:** side-by-side screenshots at the same window sizes; checklist from `ui-layout.md`; scripted screenshots of the Qt app (offscreen/xvfb) diffed by region for layout drift. Tune until proportions and states match.
- **Format matrix:** CI plays a generated sample for every row in `formats.md` and checks it reaches a decoded frame/audio output; covers H.264/HEVC/AV1/VP9/MPEG-2/ProRes, HDR10/HLG/Dolby Vision tone-mapping, 10-bit, odd resolutions, VFR, multi-audio/subtitle MKV, ASS/SRT/PGS/VobSub, HLS/DASH/RTSP/HTTP, m3u/pls/cue/xspf, image sequences, 360/3D, DVD/Blu-ray structures, V4L2.
- Performance: startup time, seek latency, 4K60 HEVC with HW decode on VA-API and NVDEC, memory ceiling.

### Phase 5: Packaging and distribution
- **Flatpak** (recommended default; bundles full FFmpeg via the freedesktop ffmpeg-full extension, avoiding patent-codec gaps on Fedora), **RPM** (Fedora; document RPM Fusion for full codecs), **.deb** (Ubuntu/Debian), **PKGBUILD** (Arch/AUR), **AppImage**. CPack/flatpak-builder/rpmbuild/debhelper as appropriate.
- `.desktop` file, MIME associations for all supported types, AppStream metainfo, icon theme integration, shell completions, man page.
- CI builds and smoke-tests every package inside Fedora, Ubuntu and Arch containers.

## 6. Quality bar
- Unit tests for core logic, integration tests for engine, QTest for UI/hotkeys, ASan/UBSan/clang-tidy/clazy in CI, no compiler warnings.
- Accessibility (screen reader names, keyboard focus), HiDPI/fractional scaling, dark/light, RTL languages.
- Every claim of "done" backed by a test, screenshot or log, never by assertion.

## 7. Create these project skills with `/skill-creator` (then use them every phase)
- `hotkey-parity-check`: regenerates and runs the hotkey test suite, prints deltas against `hotkeys.csv`.
- `ui-parity-review`: renders screens, diffs against reference layout notes, lists drift.
- `format-matrix-test`: generates sample media, runs the playback matrix, reports failures by codec/container.
- `linux-packaging`: builds Flatpak/RPM/deb/PKGBUILD/AppImage in containers and smoke-tests them (no good general-purpose skill exists; build this one).
- `clean-room-guard`: before any commit, checks that no forbidden artifacts or branding appear in the tree and the clean-room log is up to date.

## 8. Final deliverables
1. Working source tree with CI green on Fedora, Ubuntu and Arch.
2. Packages for all five formats.
3. `docs/spec/` (features, hotkeys, UI, formats, CLI, deviations) and `docs/clean-room-log.md`.
4. A parity report: percentages for features, hotkeys, formats, and UI checklist, plus the exact list of deviations and why.
5. `README.md` with build/install instructions for each distro.

Begin with Phase 0. Report back with a one-paragraph summary at each phase gate.
