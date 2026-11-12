# Technical Deviations & Linux Architecture Ledger

- **Specification ID:** SPEC-LINUX-001
- **Clean-Room Evidence Reference:** `CR-006` (docs/clean-room-log.md)
- **Target Application:** `outplayer` (Linux Qt 6 + libmpv)
- **Reference Application:** Daum PotPlayer 64-bit (Windows)
- **Engine Baseline:** `libmpv` (v0.38+) / `Qt 6.6+` / `Wayland` / `PipeWire`

---

## 1. Executive Summary & Architectural Philosophy

Daum PotPlayer was architected as a native Win32/Windows application deeply tied to the Windows multimedia stack: DirectShow filter graphs, Media Foundation transforms, Direct3D 9Ex/11/12 presentation engines, Windows Audio Session API (WASAPI), Win32 message queues, and the Windows Registry.

In architecting **outPlayer** for Linux desktop environments (Ubuntu, Fedora, Arch, Debian, etc.), direct emulation of Windows-specific subsystems (e.g., attempting to wrap DirectShow filters via Wine or replicating registry keys) is strictly rejected. Such approaches introduce performance overhead, instability, and poor desktop integration.

Instead, **outPlayer** adopts an **architectural parity through native Linux primitives** philosophy:
1. **Behavioral & Interaction Equivalence:** Preserve 100% of the user experience, hotkey muscle memory, UI layout paradigms, context menus, and configuration options.
2. **Native Under-the-Hood Stack:** Implement playback, rendering, audio dispatching, settings storage, and IPC using modern Linux desktop standards: `libmpv`, Qt 6, Wayland/X11 protocols, PipeWire/ALSA, XDG Desktop Portals, and XDG Base Directory specifications.

This specification documents all architectural divergences between the Windows reference application and the Linux implementation, detailing technical rationales, security boundaries, and the modern Linux alternative adopted.

---

## 2. Core Architectural Deviations & Adaptations

### 2.1 Video Rendering, Hardware Decoding & Presentation Pipeline

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Pipeline Architecture** | Modular DirectShow Filter Graph (Source Filter -> Splitter -> Video Decoder -> Video Processor -> Video Renderer). | Unified, asynchronous `libmpv` pipeline using `mpv_render_context`. | DirectShow filter graph graphs suffer from high latency, thread synchronization stalls, and deadlocks. `libmpv` provides an integrated, battle-tested demux-decode-filter-render loop with thread isolation and hardware zero-copy. |
| **Graphics APIs** | Direct3D 9Ex, Direct3D 11, Direct3D 12, EVR (Enhanced Video Renderer), madVR. | OpenGL 3.3+ Core (`QOpenGLWidget` / `QQuickWindow`), Vulkan (`QVulkanWindow`), Wayland dmabuf. | Direct3D is proprietary to Windows. `outplayer` embeds `mpv_render_context` into Qt 6 graphics surfaces using native OpenGL or Vulkan render targets, supporting GPU shader processing and ICC color management. |
| **Hardware Video Acceleration** | DXVA2 (DirectX Video Acceleration 2), Direct3D 11 Video API, Intel QuickSync, NVIDIA CUVID. | **VA-API** (Intel / AMD via Mesa `radeonsi`/`iris`), **NVDEC** (NVIDIA proprietary), with fallback to **VDPAU** and Vulkan Video. | `hwdec=vaapi` and `hwdec=nvdec` provide hardware decoding on Linux without CPU memory copies, passing GPU surfaces directly to OpenGL/Vulkan rendering contexts via EGL / dmabuf. |
| **Custom Video Shaders** | Direct3D HLSL pixel shaders (`.hlsl`). | User-configurable GLSL shaders (`.glsl`, e.g., mpv/Anime4K shaders). | Direct3D HLSL is incompatible with Linux drivers; `libmpv` includes an extensible GLSL shader pipeline (`--glsl-shaders`) executed directly on the GPU. |
| **Color Management** | Windows Color System (WCS) / Direct3D 11 Advanced Color (HDR10). | `libmpv` LittleCMS2 integration (`icc-profile`), Wayland color management protocol (`xx-color-management-v4`). | Native Linux color grading and ICC display profiles applied automatically through `libmpv`'s render pipeline. |

---

### 2.2 Display Modes & Fullscreen Presentation

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Exclusive Fullscreen Mode** | Exclusive Fullscreen via Direct3D mode switches and Win32 `ChangeDisplaySettings` (forces monitor resolution and refresh rate change). | **Wayland Borderless Fullscreen** via `xdg_toplevel.set_fullscreen()`; window-layer fullscreen on X11 (`_NET_WM_STATE_FULLSCREEN`). | **Wayland Security & Design Model:** Wayland compositors (GNOME Mutter, KDE KWin, Sway/wlroots) intentionally disallow individual client applications from altering global display output modes or taking exclusive control of the display hardware. Mode switching causes desktop flickering, window re-arrangements, and compositor stalls. |
| **Refresh Rate & Frame Pacing** | Direct3D exclusive swap chains with hardware VSync and display mode switching (e.g. switching display to 23.976 Hz or 24 Hz). | Smooth software frame pacing via `libmpv` (`--video-sync=display-resample`), Adaptive Sync / VRR (Variable Refresh Rate), and Wayland tearing protocols. | `video-sync=display-resample` adjusts audio/video clock ratios dynamically to eliminate frame judder without disrupting other desktop applications. For low-latency / uncomposited presentation, the Wayland `wp_tearing_control_v1` protocol is engaged where supported. |
| **Multi-Monitor Fullscreen** | Win32 `EnumDisplayMonitors` + Direct3D multi-adapter fullscreen. | FreeDesktop `wl_output` / `QScreen` output targeting. | Window management is delegated to the Wayland compositor or X11 window manager, honoring user-assigned output targets seamlessly. |

---

### 2.3 Audio Subsystem & Output Architecture

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Audio APIs** | DirectSound, WASAPI (Shared & Exclusive modes), ASIO, WaveOut. | **PipeWire** (primary default via `--ao=pipewire`), **PulseAudio** (`--ao=pulse`), and **ALSA** (`--ao=alsa`). | WASAPI and DirectSound are Windows-only. Modern Linux desktop distributions standardize on PipeWire as a unified, low-latency multimedia routing graph. PipeWire provides sub-millisecond latency, per-stream volume control, and dynamic stream routing. |
| **Bit-Perfect Pass-Through** | WASAPI Exclusive mode / S/PDIF HDMI bitstreaming (AC-3, DTS, TrueHD, DTS-HD MA). | Direct hardware pass-through via ALSA (`--ao=alsa:device=hw:...` with `--audio-spdif=ac3,dts,eac3,truehd,dts-hd`) or PipeWire non-format-converting pass-through. | Bitstreaming uncompressed/compressed multichannel formats requires bypassing software mixers. `outplayer` routes encoded IEC 61937 bitstreams directly to HDMI/optical sound cards via ALSA hardware endpoints or PipeWire passthrough nodes. |
| **Audio Device Hotplugging** | Win32 `IMMDeviceEnumerator` audio endpoint notification callbacks. | PipeWire / PulseAudio native event loops (`pa_context_subscribe` / PipeWire node registries). | Automatically redirects running audio streams when headphones, Bluetooth headsets, or HDMI outputs are connected without interrupting playback. |
| **Equalizer & Sound Effects** | Proprietary 10-band DirectShow audio transform filter. | `libmpv` audio filter graph (`af=lavfi=[firequalizer...]` / `volume` / `dynaudnorm` / `scaletempo2`). | Replaces closed-source Windows filters with high-precision 64-bit float FFmpeg audio filters (`lavfi`), offering superior sound clarity, zero phase distortion, and transparent pitch-preserved speed adjustments. |

---

### 2.4 Settings Persistence & Configuration Storage

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Storage Engine** | Windows Registry (`HKCU\Software\Daum\PotPlayer64`) or `%APPDATA%\PotPlayerMini64.ini`. | **XDG Base Directory Specification** using Qt 6 `QSettings(QSettings::IniFormat)`. | Linux has no system registry. Following FreeDesktop standards: configuration is stored in `$XDG_CONFIG_HOME/outplayer/outplayer.conf` (defaults to `~/.config/outplayer/outplayer.conf`), cache in `$XDG_CACHE_HOME/outplayer/`, and local data in `$XDG_DATA_HOME/outplayer/`. |
| **Portable Mode** | In-folder `PotPlayerMini64.ini` when installed in non-write-protected directories. | Local `outplayer.ini` placed adjacent to the executable, or configured via `--config-dir=<path>`. | Provides 1:1 portable usage for flash drives, containerized environments, or isolated user profiles. |
| **Configuration Structure** | Flat / hierarchical Windows registry keys. | Human-readable INI format with standardized sections: `[General]`, `[Playback]`, `[Video]`, `[Audio]`, `[Subtitles]`, `[Hotkeys]`, `[Filters]`. | Enables simple version control, scripting, and manual editing by advanced Linux users. |

---

### 2.5 Global Hotkeys & Desktop Input Capture

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Global Background Hotkeys** | Win32 `RegisterHotKey` API (allows background process to capture key chords globally). | **XDG Desktop Portal** (`org.freedesktop.portal.GlobalShortcuts`) on Wayland; **`XGrabKey`** fallback on X11. | **Wayland Sandboxing & Security:** Under Wayland, client applications are prohibited from sniffing or intercepting keystrokes outside their focused window surface to prevent keylogging malware. To support global media hotkeys, `outplayer` utilizes the standardized XDG GlobalShortcuts portal (supported in KDE Plasma 5.27+, GNOME 44+, and Sway), asking user consent once and integrating cleanly into system shortcut settings. |
| **Media Key Integration** | Windows System Media Transport Controls (SMTC) and keyboard hook. | **FreeDesktop MPRIS2** (`org.mpris.MediaPlayer2.outplayer`) D-Bus interface. | Integrates with desktop environments (GNOME Shell, KDE Plasma, XFCE, MATE) to support hardware keyboard keys (Play/Pause, Next, Previous, Stop, Volume) and desktop shell playback widgets/notifications. |
| **Window Input Dispatching** | Win32 `WM_KEYDOWN`, `WM_SYSKEYDOWN`, `WM_LBUTTONDOWN`, `WM_MOUSEWHEEL`. | Qt 6 event framework (`QKeyEvent`, `QMouseEvent`, `QWheelEvent`) with unified command router. | Direct platform-independent translation of keyboard chords and mouse gestures to internal `CMD_*` actions. |

---

### 2.6 Device Capture & Broadcast Inputs

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Webcam / Video Capture** | DirectShow Video Capture (`/cap`, `/cam`), Windows Media Foundation capture source. | **Video4Linux2 (V4L2)** (`/dev/video*`) via `libmpv` `avdevice` input (`v4l2:///dev/video0`). | Native Linux kernel V4L2 subsystem provides standard, high-performance streaming access to USB webcams, capture cards (e.g., Elgato, Magewell), and virtual loopback devices. |
| **Screen Capture** | Win32 GDI / DirectX Desktop Duplication API (`/cap`). | **PipeWire ScreenCast Portal** (`org.freedesktop.portal.ScreenCast`) on Wayland; `x11grab` on X11. | Under Wayland, direct display memory grabbing is restricted. The ScreenCast portal prompts the user to select a monitor, window, or virtual desktop for secure streaming into the player pipeline. |
| **Digital TV / Tuner** | DirectShow BDA (Broadcast Driver Architecture) (`/dtv`, `/atv`). | **Linux DVB API** (`/dev/dvb/adapter*`) via `libmpv` `dvb://channel`. | Open-source kernel DVB subsystem supporting DVB-T, DVB-T2, DVB-S2, and ATSC digital television tuners. |

---

### 2.7 Optical Disc & Physical Media

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **DVD-Video Playback** | Windows ASPI / SPTI SCSI pass-through + DirectShow DVD Navigator. | **`libdvdnav`** and **`libdvdread`** via `libmpv` (`dvdnav://[title]`). | Standard POSIX device node access (`/dev/sr0`, `/dev/cdrom`) with full interactive DVD menu traversal, angle selection, and chapter navigation. |
| **Blu-ray Disc Playback** | Proprietary DirectShow Blu-ray parser. | **`libbluray`** via `libmpv` (`bd://[title]`). | Mount point and BDMV structure parsing with AACS/BD+ key database integration (`~/.config/aacs/KEYDB.cfg`). |

---

### 2.8 Single-Instance IPC & File Dispatching

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **Inter-Process Communication** | Win32 `FindWindow` + `SendMessage(WM_COPYDATA)` passing serialized command lines. | **UNIX Domain Socket** via Qt 6 `QLocalServer` / `QLocalSocket` at `$XDG_RUNTIME_DIR/outplayer.sock` and D-Bus service. | Linux POSIX sockets provide robust, non-blocking IPC with file descriptor passing and permission isolation. If an instance of `outplayer` is already active, subsequent command invocations pass CLI arguments over the local socket and terminate cleanly. |

---

### 2.9 Desktop Integration & File Associations

| Dimension | Windows Reference (PotPlayer) | Linux Implementation (outPlayer) | Architecture Rationale & Technical Adaptation |
|---|---|---|---|
| **File Association Mechanism** | Windows Registry `HKEY_CLASSES_ROOT`, `ProgID`, Default Programs registered via IApplicationAssociationRegistration. | **XDG Shared MIME-Info** specification and `.desktop` desktop entry standard. | File associations are defined in `outplayer.desktop` with `MimeType=...` entries, registered using `xdg-mime default outplayer.desktop <mimetype>` and refreshed via `update-desktop-database`. |
| **Desktop Shell Menus** | Windows Explorer context menu shell extension DLLs. | FreeDesktop Desktop Action specifications (`Actions=Play;Enqueue;`) inside `outplayer.desktop`. | Enables context-menu integration in desktop file managers (Nautilus, Dolphin, Nemo, Thunar) without proprietary binary shell extensions. |

---

## 3. Preferences (F5) Tree Linux Compatibility Ledger

Below is the exhaustive mapping of the PotPlayer Preferences hierarchy to `outplayer` Linux implementations:

| Primary Tab | Sub-Category | Windows Reference Behavior | Linux Implementation (`outplayer`) | Deviation Status |
|---|---|---|---|---|
| **General** | **Startup** | Multi-instance toggle via Win32 mutex; Windows registry auto-run (`Run` key). | Single-instance via `QLocalServer`; autostart via `~/.config/autostart/outplayer.desktop`. | Adapted (XDG Standard) |
| **General** | **Skin** | Windows GDI/Direct2D custom skin files (`.dsf` ZIP with XML/bitmaps). | Qt Quick / QML modern responsive styling with user theme overrides and dark/light system sync. | Re-architected (Qt 6) |
| **General** | **Keyboard** | Win32 accelerator tables and keyboard hooks. | Qt 6 shortcut management with configurable JSON keymap profiles (`~/.config/outplayer/hotkeys.json`). | Native Equivalent |
| **General** | **Mouse** | Win32 mouse message interceptors for clicks and mouse wheel actions. | Qt 6 `QMouseEvent` and `QWheelEvent` action mappings with modifier combinations. | Native Equivalent |
| **General** | **OSD** | DirectShow VMR/EVR text overlays and GDI drawing. | Hardware-accelerated OSD engine rendering via `libmpv` OSD surface or Qt Quick overlay. | Native Equivalent |
| **Playback** | **Aspect Ratio** | Custom scaling modes and DirectShow stretching. | `libmpv` `--video-aspect-override` and `--video-unscaled` parameters. | Native Equivalent |
| **Playback** | **Jump (Seek)** | Windows DirectShow seek positions (Keyframe / Time-based). | `libmpv` `seek <seconds> relative+exact` or `relative+keyframes`. | Native Equivalent |
| **Playback** | **Speed** | DirectSound audio pitch shift / sound stretching. | `libmpv` `--speed` combined with `scaletempo2` audio filter for pitch preservation. | Improved Quality |
| **Playback** | **Time** | Windows timer tick counters. | High-resolution monotonic clock (`QElapsedTimer`) and `mpv_get_property(time-pos)`. | Native Equivalent |
| **Video** | **Renderers** | Selection of EVR, Direct3D 9Ex, Direct3D 11, madVR. | Choice of `vo=gpu` (OpenGL/Vulkan) with `gpu-context=wayland` or `gpu-context=x11egl`. | Adapted (Linux Modern) |
| **Video** | **Colors** | Direct3D hardware color controls (ProcAmp API). | GPU shader color adjustments (`--brightness`, `--contrast`, `--saturation`, `--hue`). | Native Equivalent |
| **Video** | **Deinterlacing** | DXVA hardware deinterlace (Bob, Weave, Motion Adaptive). | Hardware VA-API deinterlacing (`deinterlace=yes`, `vf=yadif` or `vf=bwdif`). | Native Equivalent |
| **Video** | **HW Acceleration** | DXVA2 / D3D11VA configuration dialog. | VA-API / NVDEC hardware decoder preferences (`--hwdec=vaapi`, `--hwdec=nvdec-copy`). | Adapted (Linux Modern) |
| **Audio** | **Renderers** | Selection of DirectSound, WASAPI, ASIO endpoints. | Selection of PipeWire, PulseAudio, or ALSA output endpoints (`--ao=pipewire`, `--ao=pulse`, `--ao=alsa`). | Adapted (Linux Modern) |
| **Audio** | **Equalizer** | Windows 10-band graphic equalizer transform. | FFmpeg 10-band audio equalizer filter graph (`af=lavfi=[firequalizer...]`). | Native Equivalent |
| **Audio** | **Normalizer** | Windows DirectShow dynamic range normalizer. | FFmpeg `dynaudnorm` filter (`af=dynaudnorm=f=150:g=15`). | Improved Quality |
| **Audio** | **Volume** | Windows system mixer / per-application endpoint volume. | Software attenuation + PipeWire application stream volume up to 200% boost. | Native Equivalent |
| **Subtitles** | **Font** | Windows GDI font enumeration (`ChooseFont`). | Linux `fontconfig` library via `libass` font selector and FreeType. | Native Equivalent |
| **Subtitles** | **Location** | Pixel-based positioning relative to window client area. | Normalized video-coordinate placement via `libass` margins and vertical alignment. | Native Equivalent |
| **Subtitles** | **Sync** | DirectShow subtitle filter millisecond offset adjustment. | Real-time `sub-delay` adjustments in `libmpv` (0.1s increments). | Native Equivalent |
| **Subtitles** | **Processing** | Built-in text subtitle filter / VobSub filter. | Advanced `libass` rendering with full ASS/SSA styling, karaoke, and embedded font support. | Native Equivalent |
| **Filter** | **Filter Priority** | DirectShow filter graph merit values and CLSID blacklists. | FFmpeg codec priority lists and `libmpv` demuxer/decoder configurations. | Re-architected (FFmpeg) |
| **Association** | **File Types** | Windows Registry `HKEY_CLASSES_ROOT` registration. | XDG MIME database XML definitions and `xdg-mime` associations. | Adapted (XDG Standard) |

---

## 4. Summary of Invariant Guarantees

Despite these necessary underlying architectural deviations:
1. **Zero Compromise in Functionality:** All features visible in the PotPlayer UI—from subtitle font styling to deinterlacing algorithms and playback speed controls—are fully realized on Linux.
2. **Hotkey & Workflow Parity:** Every default keybinding and mouse gesture operates identically to the reference application.
3. **High Performance:** Hardware video acceleration and zero-copy rendering ensure lower CPU and memory utilization than reference playback on Windows.
