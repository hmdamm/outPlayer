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
        assert r["evidence_ref"] == "CR-004", f"Invalid evidence_ref '{r['evidence_ref']}' at row {i}"
        assert r["id"] not in seen_ids, f"Duplicate id '{r['id']}' at row {i}"
        seen_ids.add(r["id"])

    # Check clean-room log entry CR-004
    with open("docs/clean-room-log.md", "r", encoding="utf-8") as f:
        log_content = f.read()
    assert "### Entry CR-004:" in log_content, "CR-004 missing in clean-room-log.md"

    print(f"Task 2 spec verification passed ({len(rows)} menu features across {len(found_areas)} areas).")

if __name__ == "__main__":
    test_task1()
    test_task2()
