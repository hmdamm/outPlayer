# Command-Line Interface (CLI) Specification

- **Specification ID:** SPEC-CLI-001
- **Clean-Room Evidence Reference:** `CR-002` (docs/clean-room-log.md, derived from `C:\Program Files\DAUM\PotPlayer\CmdLine64.txt`)
- **Target Application:** `outplayer`
- **Reference Binary:** `PotPlayerMini64.exe` / `PotPlayer64.exe`

---

## 1. Overview & Invocation Syntax

`outplayer` provides a dual command-line syntax:
1. **Canonical Linux / POSIX Syntax**: Standard GNU-style long options (`--option`, `--option=value`) and short switches (`-o`, `-u`, `-s`).
2. **PotPlayer Compatibility Syntax**: Seamless parsing of Windows-style forward-slash switches (`/switch`, `/switch=value`) allowing drop-in replacement for scripts and launchers developed for Daum PotPlayer.

### Invocation Pattern
```bash
outplayer [OPTIONS] [TARGETS...]
```
Where `TARGETS` can be one or more file paths, directory paths, streaming URLs, or `-` (for standard input pipe streaming).

---

## 2. Command-Line Options Mapping Matrix

This table maps all switches documented in `CmdLine64.txt` (reference installation) to `outplayer` GNU long options, short flags, argument types, and operational behaviors.

| Reference PotPlayer Switch | `outplayer` Long Option | Short Flag | Parameter / Type | Description & Operational Behavior | Parity Status |
|---|---|---|---|---|---|
| `/filedlg` | `--file-dialog` | `-o` | None | Immediately opens the "Open File" dialog upon application startup. | Full Parity |
| `/urldlg` | `--url-dialog` | `-u` | None | Immediately opens the "Open URL / Address" dialog upon startup. | Full Parity |
| `/folderdlg` | `--folder-dialog` | None | None | Immediately opens the "Open Folder" directory picker dialog. | Full Parity |
| `/simple` | `--simple-navigator` | None | None | Opens the lightweight File Navigator / Media Browser overlay. | Full Parity |
| `/cap` | `--capture-screen` | None | None | Launches screen capture mode (X11 / Wayland desktop portal capture). | Full Parity |
| `/cam` | `--webcam` | None | Optional device path (e.g. `/dev/video0`) | Starts video capture from default or specified webcam / V4L2 device. | Full Parity |
| `/atv[:ch]` | `--analog-tv[=CH]` | None | Optional channel integer | Opens analog TV tuner device tuned to the specified channel. | Parity / Optional |
| `/dtv[:ch[-idx]]` | `--digital-tv[=CH[-IDX]]` | None | Optional channel and stream index | Opens digital TV tuner (DVB-T/T2/C/S) tuned to channel and service index. | Parity / Optional |
| `/dvd` | `--dvd` | None | Optional path / mount | Plays default DVD device, ISO image, or `VIDEO_TS` directory structure. | Full Parity |
| `/cd[:drive]` | `--cd[=DEVICE]` | None | Optional device node (e.g. `/dev/cdrom`) | Plays specified optical CD / Audio CD drive or device node. | Full Parity |
| `/add` | `--add` | None | None | Appends specified file(s)/URL(s) to the end of the current playlist without interrupting active playback. | Full Parity |
| `/insert` | `--insert` | None | None | Inserts specified target(s) into the playlist directly after the currently playing item (or appends if idle). | Full Parity |
| `/autoplay` | `--autoplay` | `-a` | None | Automatically resumes playback of the most recently played media file from stored history. | Full Parity |
| `/same` | `--same-stream`, `--multi-stream` | None | None | Loads and plays specified media targets simultaneously as multiple synchronized audio/video streams. | Full Parity |
| `/sort` | `--sort` | None | None | Sorts target files alphabetically by filename before appending to the playlist. | Full Parity |
| `/randomize` | `--randomize`, `--shuffle` | None | None | Shuffles target files into a random order before appending to the playlist. | Full Parity |
| `/new` | `--new-window`, `--new-instance` | None | None | Forces execution in a newly spawned player process / window, overriding single-instance restrictions. | Full Parity |
| `/current` | `--current-window`, `--reuse-instance` | None | None | Directs targets to the already running player instance via IPC, overriding multiple-instance settings. | Full Parity |
| `/clipboard` | `--clipboard` | None | None | Reads media URL or file path from system clipboard, appends to playlist, and begins playback immediately. | Full Parity |
| `/seek=time` | `--seek=TIME` | `-s TIME` | `hh:mm:ss.ms` or integer seconds | Starts playback of the target media (or resume item) at the designated timestamp. | Full Parity |
| `/sub="file"` | `--sub=FILE`, `--subtitle=FILE` | None | Path or URL to subtitle file | Loads external subtitle file(s) (`.ass`, `.srt`, `.vtt`, etc.) alongside the primary media file. | Full Parity |
| `/aud="file"` | `--audio=FILE`, `--audio-file=FILE` | None | Path or URL to audio file | Loads external audio soundtrack file(s) (`.m4a`, `.aac`, `.flac`, etc.) alongside the video. | Full Parity |
| `/user_agent="ua"` | `--user-agent="UA"` | None | User-Agent string | Sets custom HTTP(S) `User-Agent` request header for network streaming. | Full Parity |
| `/referer="ref"` | `--referer="REF"` | None | Referer URL string | Sets custom HTTP(S) `Referer` request header for streaming servers requiring referrer verification. | Full Parity |
| `/headers="hdrs"` | `--headers="HDRS"` | None | HTTP header lines / pairs | Appends custom HTTP headers (e.g. `Cookie: session=xyz\r\nOrigin: ...`) for network requests. | Full Parity |
| `/title="title"` | `--title="TITLE"` | None | Title string | Overrides the display title shown in the window title bar, OSD, and MPRIS metadata. | Full Parity |
| `/volume="num"` | `--volume="NUM"` | None | Integer (0–100 or 0–200) | Sets the initial playback volume level upon startup. | Full Parity |
| `config="preset"` | `--config="PRESET"`, `--profile="PRESET"` | None | Preset configuration profile name | Activates the specified preset profile / configuration group on startup. | Full Parity |
| `-` | `-` | None | Standard Input (`stdin`) | Reads streaming media directly from the standard input pipe (e.g. `yt-dlp -o - URL \| outplayer -`). | Full Parity |

---

## 3. General Desktop & Extended Options

In addition to PotPlayer's switches, `outplayer` provides standard modern Linux desktop flags:

| Option | Short Flag | Value | Description |
|---|---|---|---|
| `--fullscreen` | `-f` | None | Start player in fullscreen mode. |
| `--minimized` | `-m` | None | Start player minimized (or to system notification area). |
| `--mute` | None | None | Start player with audio muted. |
| `--speed=RATE` | None | Float (e.g. `1.5`) | Set initial playback speed factor (0.2x – 12.0x). |
| `--hwdec=API` | None | `auto`, `vaapi`, `nvdec`, `vulkan`, `no` | Explicitly choose hardware video decoding backend. |
| `--vo=BACKEND` | None | `gpu`, `gpu-next`, `dmabuf-wayland` | Select video output renderer backend. |
| `--input-ipc-server=PATH` | None | Socket path | Bind a Unix domain socket for external IPC control. |
| `--version` | `-v` | None | Print version, build date, and compiler/library dependencies. |
| `--help` | `-h` | None | Print comprehensive command-line help and option summary. |

---

## 4. URL Title Notation & Special Argument Syntax

### 4.1 URL Title Suffix Parsing
As documented in PotPlayer CP0 syntax:
```text
"http://stream.example.com/live/ch1.m3u8\Live News HD"
```
When a URL argument contains a trailing backslash `\` followed by a string, `outplayer` extracts the substring before `\` as the stream URL and the substring after `\` as the user-defined display title (`--title`).

### 4.2 Standard Input Streaming (`-`)
When `-` is passed as a positional target argument, `outplayer` opens `fd://0` through `libmpv`, enabling piped streaming:
```bash
yt-dlp -o - -f best "https://www.youtube.com/watch?v=example" | outplayer -
```

### 4.3 Shell & Glob Wildcard Expansion
PotPlayer supports `*` and `?` wildcards on Windows. On Linux, shell globbing expands wildcards automatically, but `outplayer` also includes an internal wildcard resolver to handle quoted wildcards passed directly to the binary (e.g. `outplayer "/media/episodes/s01e*.mkv"`).

---

## 5. Linux Process Model & Single-Instance IPC

### 5.1 Single-Instance vs Multi-Instance Resolution
By default, `outplayer` respects user preferences in `F5 > General > Multiple Instances`:
- **Single Instance (Default)**: If another `outplayer` instance is already running on the same Linux session (`XDG_RUNTIME_DIR`), newly invoked commands communicate with the active instance via a Unix domain socket located at `$XDG_RUNTIME_DIR/outplayer/ipc.sock` (or D-Bus MPRIS2).
- **`/new` (`--new-window`, `--new-instance`)**: Bypasses the active instance check and spawns an independent process.
- **`/current` (`--current-window`, `--reuse-instance`)**: Enforces forwarding to the active instance regardless of configuration settings.

### 5.2 IPC Action Mapping
When targets are forwarded to a running instance:
- Plain target(s) -> Clears active playlist and plays new item (unless "Add files when opened" setting is enabled).
- Target(s) with `/add` -> Appends items to active playlist without interrupting playback.
- Target(s) with `/insert` -> Inserts items after the active track.
- `/seek=time` -> Dispatches immediate seek command to running player.

---

## 6. Built-in Help Screen (`outplayer --help`)

```text
Usage: outplayer [OPTIONS] [TARGETS...]

Arguments:
  TARGETS...                       Files, folders, URLs, or '-' for stdin streaming.

Options:
  -o, --file-dialog, /filedlg      Open the file selection dialog.
  -u, --url-dialog, /urldlg        Open the URL input dialog.
      --folder-dialog, /folderdlg  Open the directory picker dialog.
      --simple, /simple            Open the simple file navigator.
      --cap, /cap                  Start screen capture mode.
      --webcam, /cam               Open webcam or video capture device.
      --analog-tv[=CH], /atv[:ch]  Open analog TV (optional channel).
      --digital-tv[=CH], /dtv[:ch] Open digital TV (optional channel[-index]).
      --dvd, /dvd                  Open DVD drive or VIDEO_TS folder.
      --cd[=DRIVE], /cd[:drive]    Open optical CD/DVD drive.
      --add, /add                  Add targets to playlist without stopping playback.
      --insert, /insert            Insert targets directly after the current playing item.
  -a, --autoplay, /autoplay        Resume playback of the last played media item.
      --same-stream, /same         Play targets simultaneously as multiple streams.
      --sort, /sort                Sort targets alphabetically before adding.
      --randomize, /randomize      Shuffle targets randomly before adding.
      --new-window, /new           Open in a new player window/instance.
      --current-window, /current   Direct targets into the current running instance.
      --clipboard, /clipboard      Play media URL or path from clipboard.
  -s, --seek=TIME, /seek=TIME      Seek to specified timestamp (hh:mm:ss.ms or seconds).
      --sub=FILE, /sub=FILE        Load external subtitle file.
      --audio=FILE, /aud=FILE      Load external audio soundtrack file.
      --user-agent=UA, /user_agent Set custom User-Agent header for HTTP(S).
      --referer=REF, /referer      Set custom Referer header for HTTP(S).
      --headers=HDRS, /headers     Append custom headers for HTTP(S).
      --title=TITLE, /title        Override media title in window bar and OSD.
      --volume=NUM, /volume        Set initial volume level (0-100).
      --config=NAME, config=NAME   Activate specified preset configuration.
  -f, --fullscreen                 Launch in fullscreen mode.
  -m, --minimized                  Launch minimized to system tray.
  -v, --version                    Display version and build information.
  -h, --help                       Show this help message and exit.
```

---

## 7. Invocation Parity Examples

### Example 1: Direct Playback & Seeking
- **Windows (PotPlayer):**
  ```cmd
  PotPlayerMini64.exe "D:\Movies\Inception.mkv" /seek=00:45:00
  ```
- **Linux (outplayer):**
  ```bash
  outplayer ~/Videos/Inception.mkv --seek=00:45:00
  # Or with backward-compatible switch:
  outplayer ~/Videos/Inception.mkv /seek=00:45:00
  ```

### Example 2: External Subtitle & Custom Streaming User-Agent
- **Windows (PotPlayer):**
  ```cmd
  PotPlayerMini64.exe "https://stream.example.com/live.m3u8" /sub="C:\Subs\eng.srt" /user_agent="Mozilla/5.0"
  ```
- **Linux (outplayer):**
  ```bash
  outplayer "https://stream.example.com/live.m3u8" --sub=~/Subs/eng.srt --user-agent="Mozilla/5.0"
  # Or with backward-compatible switch:
  outplayer "https://stream.example.com/live.m3u8" /sub="~/Subs/eng.srt" /user_agent="Mozilla/5.0"
  ```

### Example 3: Stdin Piped Streaming
- **Windows (PotPlayer):**
  ```cmd
  youtube-dl.exe -o - -f best https://youtu.be/xyz | PotPlayerMini64.exe -
  ```
- **Linux (outplayer):**
  ```bash
  yt-dlp -o - -f best https://youtu.be/xyz | outplayer -
  ```
