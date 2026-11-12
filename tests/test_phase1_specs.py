# tests/test_phase1_specs.py
import os
import sys

def test_task1():
    # 1. Verify docs/spec/formats.md exists and contains expected matrix sections
    assert os.path.exists("docs/spec/formats.md"), "formats.md missing"
    with open("docs/spec/formats.md", "r", encoding="utf-8") as f:
        formats_content = f.read()
    
    assert "# Media Formats & Protocol Support Matrix" in formats_content
    for container in ["MKV", "MP4", "AVI", "WebM", "MPEG-TS", "FLV", "MOV", "OGG", "FLAC", "MP3"]:
        assert container in formats_content, f"Container {container} missing in formats.md"
    for vcodec in ["H.264", "HEVC", "AV1", "VP9", "MPEG-2", "ProRes", "VC-1"]:
        assert vcodec in formats_content, f"Video codec {vcodec} missing in formats.md"
    for acodec in ["AAC", "AC-3", "E-AC-3", "TrueHD", "DTS", "FLAC", "Opus", "MP3"]:
        assert acodec in formats_content, f"Audio codec {acodec} missing in formats.md"
    for sub in ["ASS/SSA", "SRT", "WebVTT", "PGS", "VobSub"]:
        assert sub in formats_content, f"Subtitle format {sub} missing in formats.md"
    for proto in ["HTTP", "HTTPS", "HLS", "DASH", "RTSP", "RTMP"]:
        assert proto in formats_content, f"Streaming protocol {proto} missing in formats.md"
    assert "CR-003" in formats_content, "Evidence reference CR-003 missing in formats.md"

    # 2. Verify docs/spec/cli.md exists and contains full CLI mapping
    assert os.path.exists("docs/spec/cli.md"), "cli.md missing"
    with open("docs/spec/cli.md", "r", encoding="utf-8") as f:
        cli_content = f.read()

    assert "--help" in cli_content
    assert "/filedlg" in cli_content
    for switch in ["/filedlg", "/urldlg", "/folderdlg", "/simple", "/cap", "/cam", "/atv", "/dtv", "/dvd", "/cd",
                   "/add", "/insert", "/autoplay", "/same", "/sort", "/randomize", "/new", "/current",
                   "/clipboard", "/seek", "/sub", "/aud", "/user_agent", "/referer", "/headers", "/title", "/volume", "config="]:
        assert switch in cli_content, f"PotPlayer switch {switch} missing in cli.md"
    for opt in ["--file-dialog", "--url-dialog", "--folder-dialog", "--seek", "--sub", "--audio", "--add", "--new-window"]:
        assert opt in cli_content, f"outplayer option {opt} missing in cli.md"
    assert "CR-002" in cli_content, "Evidence reference CR-002 missing in cli.md"

    # 3. Verify docs/clean-room-log.md contains entries CR-002 and CR-003
    with open("docs/clean-room-log.md", "r", encoding="utf-8") as f:
        log_content = f.read()
    assert "### Entry CR-002:" in log_content, "CR-002 missing in clean-room-log.md"
    assert "### Entry CR-003:" in log_content, "CR-003 missing in clean-room-log.md"

    print("Task 1 spec verification passed.")

def test_task2():
    import csv
    csv_path = "docs/spec/features.csv"
    assert os.path.exists(csv_path), "features.csv missing"

    with open(csv_path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        required_columns = ["id", "area", "feature", "path_or_hotkey", "default_value", "observed_behavior", "evidence_ref", "status"]
        assert reader.fieldnames == required_columns, f"Invalid columns: {reader.fieldnames} != {required_columns}"
        rows = list(reader)

    assert len(rows) >= 150, f"Expected at least 150 rows, got {len(rows)}"

    required_areas = {
        "Open", "Play", "Subtitles", "Video", "Audio", "Filters",
        "Bookmarks", "Capture", "Skins", "Aspect Ratio", "Window Size", "Help"
    }
    found_areas = {r["area"] for r in rows}
    missing_areas = required_areas - found_areas
    assert not missing_areas, f"Missing required areas: {missing_areas}"

    seen_ids = set()
    for i, r in enumerate(rows, 1):
        for col in required_columns:
            assert r[col] is not None and r[col].strip() != "", f"Empty field '{col}' at row {i} (id: {r.get('id')})"
        if r["area"] != "Preferences":
            assert r["evidence_ref"] == "CR-004", f"Invalid evidence_ref '{r['evidence_ref']}' at row {i}"
        else:
            assert r["evidence_ref"] == "CR-006", f"Invalid evidence_ref '{r['evidence_ref']}' at row {i}"
        assert r["id"] not in seen_ids, f"Duplicate id '{r['id']}' at row {i}"
        seen_ids.add(r["id"])

    # Check clean-room log entry CR-004
    with open("docs/clean-room-log.md", "r", encoding="utf-8") as f:
        log_content = f.read()
    assert "### Entry CR-004:" in log_content, "CR-004 missing in clean-room-log.md"

    print(f"Task 2 spec verification passed ({len(rows)} menu features across {len(found_areas)} areas).")

def test_task3():
    import csv
    csv_path = "docs/spec/hotkeys.csv"
    assert os.path.exists(csv_path), "hotkeys.csv missing"

    with open(csv_path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        required_columns = ["id", "function_name", "key", "modifiers", "scope", "menu_path", "command_id"]
        assert reader.fieldnames == required_columns, f"Invalid columns: {reader.fieldnames} != {required_columns}"
        rows = list(reader)

    assert len(rows) >= 100, f"Expected at least 100 hotkeys, got {len(rows)}"

    seen_ids = set()
    shortcuts = set()
    for i, r in enumerate(rows, 1):
        for col in required_columns:
            assert r[col] is not None and r[col].strip() != "", f"Empty field '{col}' at row {i} (id: {r.get('id')})"
        assert r["id"] not in seen_ids, f"Duplicate id '{r['id']}' at row {i}"
        seen_ids.add(r["id"])

        mod = r["modifiers"].strip()
        key = r["key"].strip()
        full_key = f"{mod}+{key}" if mod != "None" else key
        shortcuts.add(full_key)

    canonical_shortcuts = [
        "Space", "Enter", "Z", "X", "C", "Left", "Right", "Up", "Down",
        "PageUp", "PageDown", "[", "]", "Ctrl+O", "F5", "F6", "F7", "Tab"
    ]
    for cs in canonical_shortcuts:
        assert cs in shortcuts, f"Canonical shortcut '{cs}' missing in hotkeys.csv"

    # Verify presence of mouse actions
    mouse_actions = {"LeftClick", "LeftDblClick", "WheelUp", "WheelDown", "MiddleClick"}
    found_mouse = {r["key"] for r in rows if r["key"] in mouse_actions}
    missing_mouse = mouse_actions - found_mouse
    assert not missing_mouse, f"Missing mouse actions in hotkeys.csv: {missing_mouse}"

    # Verify clean room audit entry CR-005
    with open("docs/clean-room-log.md", "r", encoding="utf-8") as f:
        log_content = f.read()
    assert "### Entry CR-005:" in log_content, "CR-005 missing in clean-room-log.md"

    print(f"Task 3 spec verification passed ({len(rows)} hotkey/mouse mappings verified).")

def test_task4():
    import csv
    # 1. Verify docs/spec/linux-deviations.md exists and contains expected technical adaptations
    deviations_path = "docs/spec/linux-deviations.md"
    assert os.path.exists(deviations_path), "linux-deviations.md missing"
    with open(deviations_path, "r", encoding="utf-8") as f:
        dev_content = f.read()

    assert "# Technical Deviations & Linux Architecture Ledger" in dev_content
    # Adaptation: DirectShow/D3D -> VA-API/libmpv
    assert "DirectShow" in dev_content and "D3D" in dev_content, "DirectShow/D3D missing in linux-deviations.md"
    assert "VA-API" in dev_content and "libmpv" in dev_content, "VA-API/libmpv adaptation missing in linux-deviations.md"

    # Adaptation: DirectSound/WASAPI -> PipeWire/PulseAudio/ALSA
    assert ("DirectSound" in dev_content or "WASAPI" in dev_content), "DirectSound/WASAPI missing in linux-deviations.md"
    assert "PipeWire" in dev_content and "PulseAudio" in dev_content and "ALSA" in dev_content, "PipeWire/PulseAudio/ALSA missing in linux-deviations.md"

    # Adaptation: Exclusive Fullscreen -> Wayland Borderless
    assert "Exclusive Fullscreen" in dev_content or "Exclusive full-screen" in dev_content or "exclusive fullscreen" in dev_content.lower(), "Exclusive Fullscreen missing in linux-deviations.md"
    assert "Wayland Borderless" in dev_content or "Wayland" in dev_content, "Wayland Borderless missing in linux-deviations.md"

    # Adaptation: Registry -> XDG QSettings
    assert "Registry" in dev_content, "Windows Registry missing in linux-deviations.md"
    assert "XDG" in dev_content and "QSettings" in dev_content, "XDG / QSettings missing in linux-deviations.md"

    # Adaptation: Global Hotkeys -> XDG Portal / XGrabKey
    assert ("XDG Portal" in dev_content or "GlobalShortcuts" in dev_content or "Global Shortcuts" in dev_content) and "XGrabKey" in dev_content, "Global Hotkeys portal/XGrabKey missing in linux-deviations.md"

    # Evidence citation CR-006
    assert "CR-006" in dev_content, "Evidence reference CR-006 missing in linux-deviations.md"

    # 2. Verify docs/spec/features.csv contains preferences rows
    csv_path = "docs/spec/features.csv"
    assert os.path.exists(csv_path), "features.csv missing"
    with open(csv_path, "r", encoding="utf-8") as f:
        reader = csv.DictReader(f)
        rows = list(reader)

    pref_rows = [r for r in rows if r["area"] == "Preferences"]
    assert len(pref_rows) >= 50, f"Expected at least 50 preferences rows, got {len(pref_rows)}"

    primary_categories = ["General", "Playback", "Video", "Audio", "Subtitles", "Filter", "Association"]
    for cat in primary_categories:
        cat_matches = [r for r in pref_rows if f"Preferences > {cat}" in r["path_or_hotkey"]]
        assert len(cat_matches) > 0, f"Primary preferences category '{cat}' missing in features.csv"

    # Verify subcategories for thoroughness
    subcategories = [
        "Preferences > General > Startup",
        "Preferences > General > Skin",
        "Preferences > General > Keyboard",
        "Preferences > General > Mouse",
        "Preferences > General > OSD",
        "Preferences > Playback > Aspect Ratio",
        "Preferences > Playback > Jump",
        "Preferences > Playback > Speed",
        "Preferences > Playback > Time",
        "Preferences > Video > Renderers",
        "Preferences > Video > Colors",
        "Preferences > Video > Deinterlacing",
        "Preferences > Video > HW Acceleration",
        "Preferences > Audio > Renderers",
        "Preferences > Audio > Equalizer",
        "Preferences > Audio > Normalizer",
        "Preferences > Audio > Volume",
        "Preferences > Subtitles > Font",
        "Preferences > Subtitles > Location",
        "Preferences > Subtitles > Sync",
        "Preferences > Subtitles > Processing",
    ]
    for subcat in subcategories:
        sub_matches = [r for r in pref_rows if subcat in r["path_or_hotkey"]]
        assert len(sub_matches) > 0, f"Preferences subcategory '{subcat}' missing in features.csv"

    # 3. Verify clean room audit entry CR-006
    with open("docs/clean-room-log.md", "r", encoding="utf-8") as f:
        log_content = f.read()
    assert "### Entry CR-006:" in log_content, "CR-006 missing in clean-room-log.md"

    print(f"Task 4 spec verification passed ({len(pref_rows)} preferences rows across {len(primary_categories)} categories).")

if __name__ == "__main__":
    test_task1()
    test_task2()
    test_task3()
    test_task4()
