# Changelog

All notable changes to **outPlayer** are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.1.1] - 2026-11-26

### Fixed
- **`libmpv` End-of-File & Track Switching (`MpvEngine`)**: Inspect `mpv_event_end_file->reason` so `MPV_END_FILE_REASON_STOP` / `QUIT` / `ERROR` emit `playbackStopped()` (and `errorOccurred()` on errors) instead of `playbackFinished()`, preventing unintended auto-advance skipping tracks on manual switches or infinite error retry loops.
- **Stop State Reset (`MpvEngine`, `MainWindow`)**: `MpvEngine::stop()` now resets and emits `positionChanged(0.0)`, `durationChanged(0.0)`, `tracksChanged()`, and `chaptersChanged()`, while `MainWindow::stopPlayback()` stops the autohide timer, restores chrome, clears active file state, and resets the title.
- **Aspect Ratio Reset (`MpvEngine`)**: Map `"auto"`, `"no"`, `"-1"`, and empty strings to `"no"` when writing `video-aspect-override`, fixing `-9` (`MPV_ERROR_PROPERTY_FORMAT`) failures in `libmpv` when resetting from a fixed aspect ratio.
- **Audio Equalizer Filter Graph (`MpvEngine`)**: Replace invalid `equalizer=g1=...` syntax with `lavfi=[firequalizer=gain_entry='...']` so the 10-band graphic equalizer initializes without FFmpeg/libmpv errors alongside `lavfi=[dynaudnorm]`.
- **Desktop Integration Double-Toggle (`DesktopIntegration`, `MainWindow`)**: Separate `playRequested()` and `pauseRequested()` signals from `playPauseRequested()` and avoid double-invoking engine methods when delegated to `MainWindow`, fixing MPRIS2 and system tray `PlayPause`, `Play`, `Pause`, and `Stop` behavior.
- **Hotkey Collision & Text Input Handling (`InputManager`)**: Filter `Playlist`-scoped shortcuts from `MainWindow`'s global `Qt::WindowShortcut` map, deduplicate bound window key sequences, and guard `QEvent::ShortcutOverride` for focused text-input widgets (`QLineEdit`, `QTextEdit`, `QPlainTextEdit`, `QAbstractSpinBox`, editable `QComboBox`) so single-key hotkeys do not steal typing in search boxes.
- **Playlist Dock Chrome Overlap (`MainWindow`)**: Apply top/bottom `QWidget::contentsMargins` to `PlaylistDock` matching visible `CustomTitleBar` and `ControlBar` heights so dock tabs, close button, and bottom toolbar remain unobstructed.
- **Command Dispatch Coverage (`MainWindow`)**: Wire `CMD_OPEN_FILE`, `CMD_OPEN_FILE_STD`, `CMD_OPEN_FOLDER`, `CMD_OPEN_URL`, `CMD_OPEN_BLURAY`, `CMD_PLAY_PAUSE_EXPLICIT`, `CMD_SEEK_START`, `CMD_SEEK_END`, `CMD_VIDEO_ROTATE_90`, `CMD_AUDIO_NORMALIZE`, `CMD_ASPECT_RATIO_1..5`, `CMD_WINDOW_MAXIMUM`, and `CMD_EXIT`.
- **Preferences Synchronization & Config Validation (`PreferencesDialog`, `SettingsManager`, `MpvEngine`)**: Support `"auto-safe"` in hardware decoding combo box, unify `Video/Deinterlacing` and `Video/Deinterlace` keys, apply hardware decoder, aspect ratio, deinterlace, normalizer, speed, and subtitle language on startup and Preferences Apply, and reject corrupted/empty/binary files in `SettingsManager::importFromFile()`.
- **Compiler Warnings & Packaging Metadata**: Eliminated `-Wreorder` (`MainWindow.h`) and `-Wshadow` (`PreferencesDialog.cpp`) warnings and corrected upstream GitHub URLs in AppStream/Debian/RPM/Arch packaging metadata.
- **Documentation Accuracy**: Updated `README.md` and `docs/architecture.md` to reflect 34 CTest targets, 56px 2-tier ControlBar, accurate default hotkeys, and module dependencies.

### Added
- **`test_audit_regressions` (`tests/test_audit_regressions.cpp`)**: 13 automated regression and stress tests covering `libmpv` track switching, stop signal reset, aspect ratio reset, `firequalizer` filter validation, MPRIS2/tray control dispatch, shortcut deduplication and `QLineEdit` typing, dock non-overlap geometry, command state mutation, preferences sync, corrupted config rejection, UTF-8 Persian subtitle loading, and repeated open/close/fullscreen stability.

## [0.1.0] - 2026-11-13

### Added
- Initial public release of outPlayer with Qt 6 + `libmpv` playback engine, frameless UI, 253 PotPlayer-parity commands, 56px 2-tier control bar, subtitle pipeline, playlist dock, control panel, preferences dialog, MPRIS2/tray integration, and Linux packaging (Debian, RPM, Arch, Flatpak, AppImage).
