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

if __name__ == "__main__":
    test_task1()
