# UI Layout, Themes, Playlist & Control Panel Specifications

- **Specification ID:** SPEC-UI-001
- **Clean-Room Evidence Reference:** `CR-007` (docs/clean-room-log.md)
- **Target Application:** `outplayer` (Linux Qt 6 + libmpv)
- **Reference Application:** Daum PotPlayer 64-bit (Windows)
- **Design Baseline:** Frameless dark ergonomic media interface with hardware-accelerated presentation surface

---

## 1. Executive Summary & Aesthetic Architecture

The user interface of **outPlayer** is engineered to provide a faithful, 1:1 behavioral and ergonomic reproduction of Daum PotPlayer's iconic dark media-player aesthetic, while functioning natively on modern Linux desktop environments (Wayland and X11 compositors).

### 1.1 Core Design Pillars

1. **Media-First Minimalism:** In video playback mode, chrome elements (custom titlebar, playback control bar, and auxiliary docks) remain unobtrusive, fading into pure dark surfaces (`#1b1b1b` / `#1e1e1e`) or auto-hiding entirely so the media content takes visual precedence.
2. **Deterministic Geometry & Muscle Memory:** All interactive touchpoints—including button dimensions, slider groove height, hit target paddings, titlebar dimensions, and time scrubbers—match PotPlayer defaults to preserve established user muscle memory.
3. **Clean-Room Visual Implementation:** In accordance with clean-room engineering rules (governed under `CR-001` through `CR-007`), no proprietary bitmaps, binary skins, icon resources, or compiled dialog resources from the reference software are extracted, converted, or embedded. All UI elements are rendered using original SVG vector iconography, Qt Style Sheets (QSS), and modern Qt 6 / QWidget drawing primitives.
4. **Adaptive Presentation Layer:** Built on a layered rendering architecture where the hardware-accelerated video surface (`mpv_render_context` with OpenGL 3.3+ / Vulkan) is seamlessly composited beneath client-side window decorations, subtitle layers, and On-Screen Display (OSD) overlays.

---

## 2. Dark Theme Palette & Design Tokens

The visual appearance of **outPlayer** is defined by a cohesive dark theme palette optimized for low visual fatigue in dark viewing environments and high contrast against video content.

### 2.1 Hex Color Tokens

| Token Name | Hex Value | QSS / Component Usage |
|---|---|---|
| `color-bg-base` | `#1b1b1b` | Main window background, titlebar background, frame surface |
| `color-bg-surface` | `#1e1e1e` | Video canvas idle background, playlist background, dialog surface |
| `color-bg-control` | `#252525` | Bottom control bar background, tab bar background, aux window headers |
| `color-bg-hover` | `#323232` | Control button hover, menu item hover, list row hover (`#2a2a2a` to `#323232`) |
| `color-bg-active` | `#404040` | Control button pressed/active, active drag target |
| `color-selection` | `#3a3a3a` | Selected playlist item, selected menu entry, selected tab |
| `color-border-subtle` | `#2e2e2e` | Subtle division borders, frame edge dividers |
| `color-border-standard`| `#383838` | Slider background groove, dialog panel borders, groupbox outlines |
| `color-slider-groove` | `#383838` | Time slider background groove, volume slider groove, EQ groove |
| `color-slider-buffer` | `#555555` | Demuxer cache / network buffer progress bar fill |
| `color-slider-progress`| `#e09b1f` | Primary playback seek progress fill (amber gold) |
| `color-accent-primary`| `#ffaa00` | Bright amber accent, hover progress fill, active playback track highlight |
| `color-accent-hover`  | `#d48800` | Deep amber accent for pressed sliders and active tab indicators |
| `color-text-primary`  | `#ffffff` | Primary text: titlebar, time display, active labels, OSD text |
| `color-text-secondary`| `#aaaaaa` | Secondary text: duration totals, inactive tab headers, status labels |
| `color-text-disabled` | `#666666` | Disabled action items, grayed out menu options |
| `color-osd-outline`   | `#000000` | 2px text outline and drop shadow for OSD readability over video |
| `color-danger-hover`  | `#c42b1c` | Window close button hover background |

---

## 3. Typography & Font Hierarchy

To ensure crisp legibility on Linux desktops with diverse font configurations and High-DPI displays (X11 and Wayland fractional scaling), **outPlayer** uses system-native sans-serif font families with monospace fallbacks for numerical indicators.

### 3.1 Font Stacks

- **Primary UI Font Stack:** `Inter, -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, "Noto Sans", Ubuntu, sans-serif`
- **Monospace Numerical Stack:** `JetBrains Mono, "Roboto Mono", "Fira Code", Consolas, "Liberation Mono", monospace`

### 3.2 Typography Scale

| UI Area | Font Family | Size (pt / px) | Weight | Text Color | Alignment / Style |
|---|---|---|---|---|---|
| **Titlebar Title** | Primary UI | 11pt / 12px | Regular (400) | `#ffffff` | Left-aligned, single line, elided |
| **Control Time Display** | Monospace | 10pt / 11px | Medium (500) | `#ffffff` | Right-aligned, fixed-width glyphs |
| **Control Buttons Tooltips** | Primary UI | 9pt / 11px | Regular (400) | `#ffffff` | Tooltip popup with `#252525` background |
| **Playlist Item Title** | Primary UI | 11pt / 13px | Regular (400) | `#ffffff` / `#aaaaaa` | Left-aligned, elided at right |
| **Playlist Duration** | Monospace | 10pt / 12px | Regular (400) | `#aaaaaa` | Right-aligned |
| **Control Panel Labels** | Primary UI | 10pt / 12px | Medium (500) | `#ffffff` | Left-aligned form labels |
| **Control Panel Values** | Monospace | 9pt / 11px | Regular (400) | `#aaaaaa` | Center/Right aligned slider values |
| **OSD Notifications** | Primary UI | 18pt - 22pt | Semi-Bold (600) | `#ffffff` | 2px solid `#000000` outline, left/center |

---

## 4. Main Window Architecture & Metrics

The **Main Window** serves as the central orchestration canvas. It provides a frameless Client-Side Decoration (CSD) layout with seamless video rendering, custom border resize handles, and modular overlay docking.

```
+-------------------------------------------------------------------------------+
| [Icon] outplayer - [Filename.mkv] [1080p HEVC]                    [_] [O] [X] |  <- Titlebar (30px)
+-------------------------------------------------------------------------------+
|                                                                               |
|                                                                               |
|                                                                               |
|                             Video Surface Layer                               |
|                         (mpv_render_context OpenGL)                           |
|                                                                               |
|                                                                               |
+-------------------------------------------------------------------------------+
| ==============================[===O===================]====================== |  <- Time Slider (14px, 4px groove)
+-------------------------------------------------------------------------------+
| [ > ] [ ■ ] [ |< ] [ >| ]  00:14:32 / 01:45:20    [Vol --O--] [SUB] [F6] [F7] |  <- Control Bar (40px)
+-------------------------------------------------------------------------------+
```

### 4.1 Window Geometry Specifications

- **Minimum Window Dimensions:** `320x180` px (standard 16:9 minimum footprint).
- **Default Launch Dimensions:** `960x540` px (or dynamically sized to native video geometry, clamped to 80% of current screen resolution).
- **Aspect Ratio Locking:** When `Keep Aspect Ratio on Resize` is enabled, interactive window resizing maintains the video's display aspect ratio (e.g., 16:9, 4:3, 2.39:1 anamorphic).
- **Frameless Resize Borders:** 4px invisible active resize perimeter spanning 8 distinct hit zones (`N`, `S`, `E`, `W`, `NW`, `NE`, `SW`, `SE`).
- **Compositing Layers (Z-Index Order):**
  1. **Z-0 (Bottom):** Hardware-accelerated video surface (`mpv_render_context` using `QOpenGLWidget` / `QQuickWindow`).
  2. **Z-10:** Subtitle rendering plane (libass vector rasterization).
  3. **Z-20:** On-Screen Display (OSD) overlay canvas (`QPainter` transparent surface).
  4. **Z-30:** Custom Titlebar and Bottom Control Bar.
  5. **Z-40 (Top):** Context menus, volume slider popups, and hover tooltip thumbnails.

---

## 5. Titlebar Specification

The custom **Titlebar** integrates window manipulation and media identification within a compact, dark header.

### 5.1 Geometry & Dimensions

- **Height:** Exactly `30px`.
- **Width:** Spans 100% of the main player window width.
- **Background Color:** `#1b1b1b`.
- **Border Bottom:** 1px solid `#2e2e2e`.

### 5.2 Component Hierarchy & Controls

1. **Application Icon / Main Menu Trigger (Left):**
   - Size: `24x24px` button with centered 16x16px `outplayer` brand logo.
   - Action: Left-click opens main context menu at position `(0, 30)`. Double-click closes window.
2. **Title Label (Center-Left):**
   - Height: `30px`, vertically centered.
   - Text Template: `outplayer - [Media Title / Filename] [Resolution / Codec]` (e.g., `outplayer - Sintel.2010.1080p.mkv [1920x1080 HEVC]`).
   - Elision: Right-edge elision (`Qt::ElideRight`) when title exceeds available width.
3. **Draggable Area:**
   - Unoccupied titlebar space acts as an interactive drag handle for window repositioning.
   - Double-click toggles between Maximized and Restored window states.
4. **Window Control Buttons (Right Cluster):**
   - Each button has dimension `28x24px` (or `24x24px` compact) with centered vector glyphs:
     - **Minimize (`_`):** Minimizes player to taskbar / dock.
     - **Maximize / Restore (`□` / `❐`):** Toggles maximized state.
     - **Close (`✕`):** Terminates playback and exits application (hover background: `#c42b1c`).

### 5.3 Presentation Behavior

- **Windowed Mode:** Permanently visible by default (configurable to auto-hide).
- **Fullscreen Mode:** Automatically hides with the control bar. Hovering the cursor within 0-10px of the top display edge smoothly animates the titlebar into view (200ms slide transition).

---

## 6. Control Bar Specification

The **Control Bar** occupies the bottom of the player window, hosting playback controls, timing indicators, audio adjustments, and auxiliary window triggers.

### 6.1 Geometry & Dimensions

- **Height:** Exactly `40px`.
- **Width:** Spans 100% of the player window width.
- **Placement:** Immediately below the Time Slider at the bottom of the window.
- **Background Color:** `#252525`.
- **Border Top:** 1px solid `#303030`.

### 6.2 Button Dimensions & Styling

- **Standard Control Buttons:** `24x24px` square footprint with 4px inner padding.
- **Primary Play / Pause Button:** `28x28px` prominent button.
- **Button Hover State:** Background `#323232`, border-radius 3px.
- **Button Pressed State:** Background `#404040`.

### 6.3 Control Clusters

#### Left Cluster: Playback Transport
- **Play / Pause (`28x28px`):** Toggles playback state (Hotkey: `Space`). Displays Play (`▶`) or Pause (`❚❚`) vector icon.
- **Stop (`24x24px`):** Halts playback and unloads current frame (Hotkey: `Ctrl+Space`).
- **Previous (`24x24px`):** Skips to previous playlist file or chapter (Hotkey: `PageUp`).
- **Next (`24x24px`):** Skips to next playlist file or chapter (Hotkey: `PageDown`).
- **Step Backward (`24x24px`):** Frame-step backward (Hotkey: `D`).
- **Step Forward (`24x24px`):** Frame-step forward (Hotkey: `F`).

#### Middle Cluster: Playback Time Indicator
- **Layout:** Monospace numerical text field formatted as `00:00:00 / 01:45:22` (Elapsed / Total duration).
- **Color:** `#ffffff` for elapsed time, `#aaaaaa` for total duration.
- **Interactive Click Toggle:** Clicking the time display toggles elapsed time mode vs remaining time mode (`-01:45:22`).

#### Right Cluster: Volume, Streams & Auxiliary Windows
- **Mute / Volume Trigger (`24x24px`):** Click toggles mute state (Hotkey: `M`). Hovering or clicking reveals a horizontal volume slider (`width: 80px`, groove: `#383838`, progress: `#e09b1f`, thumb: `#ffffff`).
- **Audio Stream Selector (`24x24px`):** Cycles audio tracks (Hotkey: `A`) or opens stream popup.
- **Subtitle Selector (`24x24px`):** Cycles subtitle tracks (Hotkey: `Alt+H`) or toggles visibility (`V`).
- **Playlist Toggle (`24x24px`):** Toggles Playlist Dock (Hotkey: `F6`). Highlighted with `#ffaa00` when open.
- **Control Panel Toggle (`24x24px`):** Toggles Control Panel window (Hotkey: `F7`). Highlighted with `#ffaa00` when open.
- **Preferences (`24x24px`):** Opens full Preferences dialog (Hotkey: `F5`).
- **Fullscreen Toggle (`24x24px`):** Toggles fullscreen presentation (Hotkey: `Enter`).

### 6.4 Auto-Hide Presentation Logic

- **Inactivity Timer:** In fullscreen mode (and optionally windowed mode), the control bar and time slider smoothly fade out after 3000ms of user inactivity (mouse stationary).
- **Wake Trigger:** Any mouse movement over the video canvas immediately restores visibility within 150ms.

---

## 7. Time Slider Specification

The **Time Slider** provides high-precision media timeline navigation, buffer status feedback, and visual scrubbing.

### 7.1 Geometry & Dimensions

- **Total Height:** Exactly `14px` interactive hit area.
- **Groove Height:** Exactly `4px` horizontal bar centered vertically within the 14px hit region.
- **Width:** Spans 100% of the player window width (pinned between left and right window borders).
- **Scrubber Thumb Size:** `10x10px` circle (`#ffffff`), expanding to `14x14px` with `#ffaa00` outer glow on hover or active scrub.

### 7.2 Multi-Layer Visual Track

```
+-------------------------------------------------------------------------------+
|                      [============= Scrubber Thumb =============]             |
| [==================== Progress Bar (#e09b1f) ======]                          |
| [=========================== Buffer Bar (#555555) ===============]             |
| [---------------------------- Background Groove (#383838) -------------------] |
+-------------------------------------------------------------------------------+
```

1. **Background Groove:** `#383838` with 2px border radius.
2. **Buffer Bar:** `#555555` showing cached demuxer / network stream range.
3. **Progress Bar:** `#e09b1f` (default) transitioning to `#ffaa00` on hover, tracking current playback timestamp.
4. **Chapter / Bookmark Ticks:** 2px wide vertical tick marks placed along the groove:
   - Chapters: `#ffffff` semi-transparent tick mark.
   - Bookmarks: `#ff5555` vivid red tick mark.

### 7.3 Interactive Scrubbing & Hover Preview

- **Click-to-Seek:** Direct left-click anywhere along the 14px hit area instantly seeks to the proportional timestamp.
- **Continuous Drag Seek:** Smooth scrubbing updates the playback timestamp continuously (or emits throttled `seek` commands with keyframe snapping for low-latency scrubbing).
- **Hover Thumbnail & Time Tooltip:**
  - Moving the cursor across the time slider displays a floating tooltip positioned directly above the cursor.
  - Displays formatted timestamp `hh:mm:ss`.
  - Displays cached video thumbnail preview (`160x90px` popup) corresponding to the hovered timeline coordinate.

---

## 8. On-Screen Display (OSD) Specification

The **OSD Engine** renders non-intrusive textual and graphical feedback directly over the video presentation layer to confirm user commands, volume modifications, speed shifts, and playback diagnostics.

### 8.1 Visual Styling & Typography

- **Font Family:** Primary UI font stack (`Inter`, `Noto Sans`, `Segoe UI`).
- **Font Weight:** Semi-Bold (`600`).
- **Font Size:** Responsive sizing between `18pt` and `22pt` (scaled dynamically based on window resolution: `clamp(14pt, 2vw, 24pt)`).
- **Text Color:** `#ffffff` (pure white).
- **Outline & Contrast Shield:** 2px solid `#000000` text stroke outline combined with an ambient drop shadow (offset `1px 2px`, blur 4px, alpha 0.85). This guarantees crisp legibility against 100% white video backgrounds.

### 8.2 Placements & Message Types

1. **Top-Left Placement (Status Notifications):**
   - Volume adjustment: `Volume: 85%`, `Mute`
   - Playback speed: `Speed: 1.20x`, `Normal Speed (1.0x)`
   - Seek offset: `Seek: +00:00:05 [00:15:30]`
   - Audio delay: `Audio Sync: +50 ms`
   - Subtitle sync: `Subtitle Sync: -100 ms`
   - Aspect ratio: `Aspect Ratio: 16:9`
   - A-B Repeat: `Repeat: Set Point A [00:10:00]`, `Repeat: A-B Range Active`
   - Bookmarks: `Bookmark Added: 00:22:15`
2. **Center Placement (Transient Transport State):**
   - Translucent icon and text indicator for Play (`▶`), Pause (`❚❚`), Stop (`■`), or Frame Step.
3. **Full-Canvas Diagnostic Overlay (Tab Key):**
   - Invocable via `Tab` key (toggles detailed technical playback overlay).
   - Displays real-time metadata:
     - Container format, file size, bitrate
     - Video codec, pixel format, color space, hardware decoder status (`vaapi`, `nvdec`, `sw`)
     - Audio codec, sample rate, channels, output driver (`PipeWire`)
     - Dropped frames counter, presentation jitter, CPU and GPU utilization graph.

### 8.3 Timing & Animation

- **Display Duration:** Standard notifications display for 1500ms to 2000ms.
- **Fade Transition:** Smooth alpha fade-out over 300ms following the hold duration.
- **Message Stacking:** Successive notifications cleanly replace the active notification without layout stutter.

---

## 9. Playlist Dock (F6) Specification

The **Playlist Window** (`F6`) provides robust media queue management with multi-tab categorization, search filtering, and drag-and-drop file organization.

```
+---------------------------------------------------------+
| [Default] [Favorites] [Albums] [History]            [X] |  <- Tab Bar
+---------------------------------------------------------+
| [ Search playlist...                              (🔍) ] |  <- Search Filter
+---------------------------------------------------------+
| #  | Title                                  | Duration  |
|----+----------------------------------------+-----------|
| 01 | Sintel - Teaser Trailer                | 00:52     |
| 02 ▶ Big Buck Bunny (1080p 60fps)           | 09:56     |  <- Active Playing (#ffaa00)
| 03 | Tears of Steel - VFX Open Movie        | 12:14     |
| 04 | Cosmos Laundromat - First Cycle        | 12:04     |
+---------------------------------------------------------+
| [+] Add   [-] Remove   [⇅] Sort   [☰] Menu   4 Items     |  <- Bottom Toolbar
+---------------------------------------------------------+
```

### 9.1 Docking & State Architecture

- **Docked Mode:** Snaps seamlessly to the right (or left) edge of the main player window. Resizes synchronously when the main window is moved or resized. Default docked width: `300px` (resizable from `200px` to `600px`).
- **Floating Mode:** Can be detached into an independent top-level window suitable for multi-monitor setups.
- **Toggle Hotkey:** `F6`. Remembers open/closed state and geometry across sessions via `QSettings`.

### 9.2 Multi-Tab Dock Structure

The playlist window features a top tab bar supporting:
- **Default:** The active playback queue.
- **Favorites:** Starred media paths and network streams.
- **Albums:** Grouped collections and music albums.
- **History:** Chronological log of recently played media.
- **Tab Styling:** Background `#252525`, active tab `#1b1b1b` with an amber highlight line (`#ffaa00`).

### 9.3 Item List View & Data Columns

- **Table Columns:**
  - Track Index (`#`): 2-digit numerical index, right-aligned.
  - Title / Filename: Left-aligned media title (or base filename when title tag is unavailable).
  - Duration: Right-aligned duration string formatted as `mm:ss` or `hh:mm:ss`.
- **Row Styling:**
  - Base Row Background: `#1e1e1e` (alternating rows `#222222`).
  - Hover Row Background: `#2a2a2a`.
  - Selected Row Background: `#383838`.
  - Active Playing Row: High-contrast amber text `#ffaa00` with a playing indicator glyph (`▶`).
- **Interactions:**
  - Double-click item: Immediately starts playback.
  - Drag-and-drop: Reorder items within the playlist or drag external files/folders from desktop file managers (Nautilus, Dolphin, Thunar).

### 9.4 Search Filter & Bottom Toolbar

- **Live Search Filter:** Text input field above the playlist table providing real-time fuzzy filtering.
- **Bottom Action Toolbar:**
  - **Add (`+`):** Popup menu: Add File(s), Add Directory, Add URL stream.
  - **Remove (`-`):** Remove Selected, Remove Missing / Inaccessible Files, Clear Playlist.
  - **Sort (`⇅`):** Sort by Name, Sort by Path, Sort by File Size, Sort by Date, Randomize / Shuffle.
  - **Playlist Menu (`☰`):** Export Playlist (`.m3u`, `.m3u8`, `.pls`), Load Playlist, Save as Favorite.
  - **Status Summary:** Label showing item count and aggregate queue runtime (e.g., `24 items [02:14:10]`).

---

## 10. Control Panel (F7) Specification

The **Control Panel** (`F7`) is a dedicated quick-adjustment auxiliary window providing fine-grained, real-time tuning of video parameters, audio equalization, subtitle synchronization, and playback speed.

```
+---------------------------------------------------------+
| [ Video ] [ Audio / EQ ] [ Subtitles ] [ Playback ] [X] |  <- Tab Bar
+---------------------------------------------------------+
|  Brightness:   [----O-----------------] 0     [Reset]   |
|  Contrast:     [--------O-------------] 0     [Reset]   |
|  Saturation:   [----O-----------------] 0     [Reset]   |
|  Hue:          [------------O---------] 0     [Reset]   |
|                                                         |
|  [x] Deinterlacing   Rotation: [ 0° ]   [Flip H] [Flip V|
|  [ Reset All Video Adjustments ]                        |
+---------------------------------------------------------+
```

### 10.1 Window Specifications

- **Window Type:** Non-modal tool window (`Qt::Tool`) that floats alongside the player and allows concurrent playback interaction.
- **Default Geometry:** `380x320px` fixed or resizable panel.
- **Toggle Hotkey:** `F7`.

### 10.2 Tab 1: Video Controls

- **Hardware Video Equalizer Sliders:**
  - **Brightness:** Range `-100` to `+100`, default `0`, step `1`.
  - **Contrast:** Range `-100` to `+100`, default `0`, step `1`.
  - **Saturation:** Range `-100` to `+100`, default `0`, step `1`.
  - **Hue:** Range `-100` to `+100`, default `0`, step `1`.
  - Each slider includes a numerical value label and an individual `Reset` button.
- **Transform & Deinterlace Controls:**
  - **Deinterlacing Toggle:** Checkbox with mode selector (`Auto`, `Yadif`, `Bwdif`, `None`).
  - **Screen Rotation:** Dropdown / buttons for `0°`, `90°`, `180°`, `270°`.
  - **Flip:** Horizontal Flip and Vertical Flip toggle buttons.
- **Global Reset:** `Reset All Video Adjustments` button returning all values to `0`.

### 10.3 Tab 2: Audio & Graphic Equalizer

- **Master Equalizer Toggle:** Checkbox enabling/disabling 10-band equalization.
- **10-Band Graphic Equalizer:**
  - Frequency Bands: `31Hz`, `62Hz`, `125Hz`, `250Hz`, `500Hz`, `1kHz`, `2kHz`, `4kHz`, `8kHz`, `16kHz`.
  - Gain Range: `-12dB` to `+12dB` per band, default `0dB`.
  - Sliders: Vertical slider layout with central zero-detent.
- **Preamp Slider:** `-12dB` to `+12dB` overall gain control.
- **Equalizer Presets Dropdown:**
  - Includes: `Flat`, `Rock`, `Pop`, `Jazz`, `Classical`, `Vocal`, `Bass Boost`, `Full Treble`, `Dance`, `Party`, `Club`.
- **Dynamic Normalizer Toggle:**
  - Checkbox enabling real-time audio volume normalization (dynamic range compression for dialogue leveling).

### 10.4 Tab 3: Subtitles

- **Subtitle Stream Selection:** Dropdown listing all embedded and external subtitle tracks.
- **Sync Offset Slider:** Range `-5.0s` to `+5.0s` in precise `0.1s` increments, with a `Reset (0.0s)` button.
- **Font Size Adjustment:** Slider adjusting subtitle scale (`50%` to `200%`, default `100%`).
- **Vertical Position Adjustment:** Slider positioning subtitles along the Y-axis (`0%` top to `100%` bottom, default `90%`).

### 10.5 Tab 4: Playback Controls

- **Speed Control Slider:**
  - Range: `0.2x` to `12.0x` in `0.1x` / `1%` increments. Default `1.0x`.
  - Quick Speed Buttons: `0.5x`, `1.0x` (Reset), `1.5x`, `2.0x`.
- **A-B Repeat Controller:**
  - **Point A:** Timestamp display + `Set Point A` button (Hotkey: `[`).
  - **Point B:** Timestamp display + `Set Point B` button (Hotkey: `]`).
  - **Repeat Toggle:** Checkbox enabling/disabling loop between Point A and Point B (Hotkey: `\`).
  - **Clear:** Button clearing active A-B repeat range.

---

## 11. Clean-Room Adherence & Audit Citation

This specification is derived exclusively through non-invasive black-box visual inspection of Daum PotPlayer running in a standard Windows reference environment, cross-referenced with public Qt 6 documentation and `libmpv` architectural specifications.

- **Clean-Room Audit Entry:** `CR-007` (recorded in `docs/clean-room-log.md`).
- **Target Application:** `outplayer`.
- **Asset Integrity Statement:** Zero proprietary skins, icons, images, dialog binaries, or audio resources are incorporated into `outplayer`. All visual tokens, styling rules, metrics, and vector layouts are original implementations designed to achieve functional and ergonomic equivalence.
