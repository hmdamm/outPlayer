# Phase 1: Black-Box Feature Inventory Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Establish an exhaustive, clean-room black-box specification of PotPlayer 64-bit in `docs/spec/` covering features, default hotkeys, UI metrics, media formats, CLI switches, and Linux deviations, backed by verifiable citations in `docs/clean-room-log.md`.

**Architecture:** Observer-role extraction and cataloging from official public documentation, reference language mapping (`Language\English.ini`), command documentation (`CmdLine64.txt`), and live UI observation of `PotPlayerMini64.exe` on Windows host into structured CSV and Markdown specifications.

**Tech Stack:** Python 3 (verification test harness), CSV, Markdown, Clean-Room Auditing.

**Spec:** `docs/superpowers/specs/2026-11-11-outplayer-design.md`

## Global Constraints
- Clean-room rules: Strict Observer role. No disassembly, decompilation, binary patching, hooking, memory analysis, or copying proprietary binary assets (icons/skins/logos/translations).
- Branding neutrality: No occurrence of "PotPlayer", "Daum", or "Kakao" in product name, binary identifiers (`outplayer`), desktop files, or package IDs.
- Coverage: 100% of visible menu hierarchies, default hotkey tables, and preferences dialog pages must be cataloged in `features.csv` or `hotkeys.csv` with evidence links in `docs/clean-room-log.md`.
- Automated spec validation: All spec files must pass `tests/test_phase1_specs.py`.

## Review Focus
1. Incomplete hotkey mapping missing modifiers (Ctrl/Alt/Shift) -> Validated against PotPlayer's default keyboard shortcut table.
2. Missing unassigned-by-default command IDs -> Ensure actions rebindable by users are represented.
3. Vague layout dimensions -> Provide exact pixel metrics, dark theme hex color tokens, and OSD geometry in `ui-layout.md`.
4. Overlooking Wayland/Linux desktop deviations -> Explicitly detail compositor limitations and solutions in `linux-deviations.md`.
5. Missing clean-room log citations -> Ensure every spec entry references an explicit audit ID in `docs/clean-room-log.md`.

---

### Task 1: Media Formats Matrix & CLI Specifications

**Files:**
- Create: `docs/spec/formats.md`
- Create: `docs/spec/cli.md`
- Modify: `docs/clean-room-log.md`
- Create: `tests/test_phase1_specs.py`

**Interfaces:**
- Produces: `docs/spec/formats.md` (containers, video/audio codecs, subtitles, streaming protocols supported by libmpv/FFmpeg aligned with PotPlayer) and `docs/spec/cli.md` (CLI switches derived from `CmdLine64.txt`).

- [ ] **Step 1: Write test verifying formats.md and cli.md exist and conform to schema**

```python
# tests/test_phase1_specs.py
import os
import sys

def test_task1():
    assert os.path.exists("docs/spec/formats.md"), "formats.md missing"
    assert os.path.exists("docs/spec/cli.md"), "cli.md missing"
    with open("docs/spec/cli.md", "r", encoding="utf-8") as f:
        content = f.read()
        assert "--help" in content or "/filedlg" in content
    print("Task 1 spec verification passed.")

if __name__ == "__main__":
    test_task1()
```

- [ ] **Step 2: Run test to verify it fails**

Run: `python tests/test_phase1_specs.py`  
Expected: FAIL (`formats.md missing`).

- [ ] **Step 3: Author `docs/spec/formats.md` and `docs/spec/cli.md`**

- `docs/spec/formats.md`: Matrix covering containers (MKV, MP4, AVI, WebM, TS, FLV, MOV, OGG, FLAC, MP3, etc.), video codecs (H.264, HEVC, AV1, VP9, MPEG-2, ProRes, VC-1), audio codecs (AAC, AC3, E-AC3, TrueHD, DTS, DTS-HD, FLAC, Opus, MP3), subtitles (ASS/SSA, SRT, WebVTT, PGS, VobSub), and network protocols (HTTP/HTTPS, HLS, DASH, RTSP, RTMP).
- `docs/spec/cli.md`: Comprehensive translation of PotPlayer CLI arguments from `CmdLine64.txt` to `outplayer` GNU-style long options and PotPlayer compatibility switches (`--filedlg`, `--urldlg`, `--seek=<time>`, `--sub=<file>`, `--aud=<file>`, `--fullscreen`, `--new`, `--add`, etc.).
- Update `docs/clean-room-log.md` with citations CR-002 (CmdLine64.txt) and CR-003 (Format matrix).

- [ ] **Step 4: Run test to verify it passes**

Run: `python tests/test_phase1_specs.py`  
Expected: `Task 1 spec verification passed.`

- [ ] **Step 5: Commit**

```bash
git add docs/spec/formats.md docs/spec/cli.md docs/clean-room-log.md tests/test_phase1_specs.py
git commit -m "feat(spec): author media formats matrix and CLI specification"
```

---

### Task 2: Context Menu & Main Menu Feature Inventory

**Files:**
- Create: `docs/spec/features.csv`
- Modify: `docs/clean-room-log.md`
- Modify: `tests/test_phase1_specs.py`

**Interfaces:**
- Produces: `docs/spec/features.csv` containing the complete menu tree: Open, Play, Subtitles, Video, Audio, Filters, Bookmarks, Capture, Skins, Aspect Ratio, Window Size, and Help.
- Header: `id,area,feature,path_or_hotkey,default_value,observed_behavior,evidence_ref,status`

- [ ] **Step 1: Add menu coverage checks to `tests/test_phase1_specs.py`**

Assert `docs/spec/features.csv` exists, has required columns, contains at least 150 menu action rows, and has zero empty fields for `id`, `area`, `feature`, and `evidence_ref`.

- [ ] **Step 2: Run test to verify it fails**

Run: `python tests/test_phase1_specs.py`  
Expected: FAIL (`features.csv missing`).

- [ ] **Step 3: Extract and author menu rows in `docs/spec/features.csv`**

- Audit the complete PotPlayer menu tree using clean English definitions from `Language\English.ini` and live PotPlayer UI observation.
- Catalog all submenus: Open (File, Folder, URL, DVD, Device, Clipboard), Play (Play/Pause, Stop, Speed, A-B Repeat, Chapters, Jump), Subtitles (Select, Add, Style, Sync, Alignment), Video (Renderer, Deinterlace, Filters, Pixel Shaders, Aspect Ratio), Audio (Audio Stream, Output, Volume, Equalizer, Normalizer), Capture (Save Image, Record Video, Copy Current Frame).
- Add audit entry CR-004 in `docs/clean-room-log.md`.

- [ ] **Step 4: Run test to verify it passes**

Run: `python tests/test_phase1_specs.py`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add docs/spec/features.csv docs/clean-room-log.md tests/test_phase1_specs.py
git commit -m "feat(spec): inventory context menu and main menu features in features.csv"
```

---

### Task 3: Hotkey & Mouse Action Mapping

**Files:**
- Create: `docs/spec/hotkeys.csv`
- Modify: `docs/clean-room-log.md`
- Modify: `tests/test_phase1_specs.py`

**Interfaces:**
- Produces: `docs/spec/hotkeys.csv` with columns:
  `id,function_name,key,modifiers,scope,menu_path,command_id`

- [ ] **Step 1: Add hotkey coverage checks to `tests/test_phase1_specs.py`**

Assert `docs/spec/hotkeys.csv` exists, contains standard default keybindings (Space, Enter, Z, X, C, Left, Right, Up, Down, PageUp, PageDown, [, ], Ctrl+O, F5, F6, F7, Tab, etc.), and contains at least 100 shortcut mappings.

- [ ] **Step 2: Run test to verify it fails**

Run: `python tests/test_phase1_specs.py`  
Expected: FAIL (`hotkeys.csv missing`).

- [ ] **Step 3: Author `docs/spec/hotkeys.csv`**

- Document all default playback shortcuts, navigation seeks, volume controls, frame steps, aspect ratio toggles, subtitle sync/offset adjustments, and window controls as defined in PotPlayer Preferences > General > Keyboard.
- Map each to a clean command identifier (`CMD_PLAY_PAUSE`, `CMD_SEEK_FORWARD_SHORT`, `CMD_VOLUME_UP`, `CMD_PREFERENCES`, `CMD_PLAYLIST`, `CMD_OSD_TOGGLE`, etc.).
- Add audit citation CR-005 in `docs/clean-room-log.md`.

- [ ] **Step 4: Run test to verify it passes**

Run: `python tests/test_phase1_specs.py`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add docs/spec/hotkeys.csv docs/clean-room-log.md tests/test_phase1_specs.py
git commit -m "feat(spec): author complete hotkey and input mapping in hotkeys.csv"
```

---

### Task 4: Preferences (F5) Tree & Linux Deviations

**Files:**
- Modify: `docs/spec/features.csv`
- Create: `docs/spec/linux-deviations.md`
- Modify: `docs/clean-room-log.md`
- Modify: `tests/test_phase1_specs.py`

**Interfaces:**
- Produces: Preferences hierarchy in `docs/spec/features.csv` (area = `preferences`) and comprehensive Linux compatibility ledger in `docs/spec/linux-deviations.md`.

- [ ] **Step 1: Add Preferences and deviations checks to `tests/test_phase1_specs.py`**

Assert `features.csv` contains preferences rows across all primary tabs (General, Playback, Video, Audio, Subtitles, Filter, Association), and `linux-deviations.md` details Wayland/Linux platform adaptations (DirectShow -> libmpv/VAAPI, DirectSound -> PipeWire/PulseAudio, Windows Registry -> XDG .ini / QSettings).

- [ ] **Step 2: Run test to verify it fails**

Run: `python tests/test_phase1_specs.py`  
Expected: FAIL (`linux-deviations.md missing` or preferences missing).

- [ ] **Step 3: Implement Preferences rows and `linux-deviations.md`**

- Catalog the Preferences tree: General (Startup, Skin, Keyboard, Mouse, OSD), Playback (Aspect Ratio, Jump, Speed, Time), Video (Renderers, Colors, Deinterlacing, HW Acceleration), Audio (Renderers, Equalizer, Normalizer, Volume), Subtitles (Font, Location, Sync, Processing), Association.
- In `linux-deviations.md`, document technical deviations where Linux cannot or should not match Windows DirectShow/D3D (e.g. exclusive full-screen vs Wayland borderless fullscreen, GlobalShortcuts portal vs XGrabKey, V4L2 vs DirectShow capture).
- Add audit citation CR-006 in `docs/clean-room-log.md`.

- [ ] **Step 4: Run test to verify it passes**

Run: `python tests/test_phase1_specs.py`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add docs/spec/features.csv docs/spec/linux-deviations.md docs/clean-room-log.md tests/test_phase1_specs.py
git commit -m "feat(spec): inventory preferences hierarchy and document linux deviations"
```

---

### Task 5: UI Layout, Themes, Playlist & Control Panel Specifications

**Files:**
- Create: `docs/spec/ui-layout.md`
- Modify: `docs/clean-room-log.md`
- Modify: `tests/test_phase1_specs.py`

**Interfaces:**
- Produces: `docs/spec/ui-layout.md` specifying exact geometry, spacing, colors, font hierarchy, OSD styling, Control Panel (F7) tabs, and Playlist (F6) layout.

- [ ] **Step 1: Add layout spec checks to `tests/test_phase1_specs.py`**

Assert `docs/spec/ui-layout.md` exists and contains sections for Main Window, Titlebar, Control Bar, Time Slider, OSD, Playlist Dock (F6), and Control Panel (F7) with hex color palette.

- [ ] **Step 2: Run test to verify it fails**

Run: `python tests/test_phase1_specs.py`  
Expected: FAIL (`ui-layout.md missing`).

- [ ] **Step 3: Author `docs/spec/ui-layout.md`**

- Specify exact dimensions: window minimum size (320x180), titlebar height (30px), control bar height (40px), time slider height (14px with 4px groove), button sizes (24x24px / 28x28px).
- Define dark color tokens: Background `#1b1b1b` / `#1e1e1e`, Control bar `#252525`, Slider groove `#383838`, Slider progress `#e09b1f` / `#ffaa00`, Text primary `#ffffff`, Text secondary `#aaaaaa`, Accent highlight `#d48800`.
- Document OSD typography (semi-bold sans-serif, drop shadow or border outline `#000000`, top-left and center placements).
- Detail Playlist (F6) multi-tab docking and Control Panel (F7) audio equalizer/video color sliders.
- Add audit citation CR-007 in `docs/clean-room-log.md`.

- [ ] **Step 4: Run test to verify it passes**

Run: `python tests/test_phase1_specs.py`  
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add docs/spec/ui-layout.md docs/clean-room-log.md tests/test_phase1_specs.py
git commit -m "feat(spec): specify ui layout metrics, dark theme palette, and aux windows"
```

---

### Task 6: Clean-Room Audit Verification & Phase 1 Gate Clearance

**Files:**
- Modify: `TASKS.md`
- Modify: `docs/decisions.md`
- Modify: `docs/clean-room-log.md`
- Modify: `tests/test_phase1_specs.py`

**Interfaces:**
- Produces: Verified Phase 1 gate status in `TASKS.md`, ADR 004 in `docs/decisions.md`, complete clean-room audit trail.

- [ ] **Step 1: Run comprehensive Python test suite validating all specs**

Run: `python tests/test_phase1_specs.py`  
Expected: All tests pass, validating 100% of required spec files, row counts, and clean-room cross-references.

- [ ] **Step 2: Verify zero clean-room violations in codebase**

Verify via automated checks that no binary artifacts or copyright infringements are present.

- [ ] **Step 3: Record ADR 004 and update `TASKS.md`**

Add ADR 004 in `docs/decisions.md` detailing Phase 1 Specification Baseline and mark Phase 1 tasks P1-T1 through P1-T6 as `Done` in `TASKS.md`.

- [ ] **Step 4: Commit**

```bash
git add TASKS.md docs/decisions.md docs/clean-room-log.md tests/test_phase1_specs.py
git commit -m "docs: complete Phase 1 gate verification and record ADR 004"
```
