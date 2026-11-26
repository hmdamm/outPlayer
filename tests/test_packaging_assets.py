#!/usr/bin/env python3
"""
Unit and integration tests for outPlayer desktop integration assets,
AppStream metainfo, hicolor icons, and CMake installation rules.
"""

import os
import sys
import shutil
import subprocess
import xml.etree.ElementTree as ET

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DATA_DIR = os.path.join(REPO_ROOT, "data")
ICONS_DIR = os.path.join(DATA_DIR, "icons", "hicolor")
DESKTOP_FILE = os.path.join(DATA_DIR, "org.outplayer.outPlayer.desktop")
METAINFO_FILE = os.path.join(DATA_DIR, "org.outplayer.outPlayer.metainfo.xml")
SVG_ICON = os.path.join(ICONS_DIR, "scalable", "apps", "org.outplayer.outPlayer.svg")
RENDER_SCRIPT = os.path.join(DATA_DIR, "icons", "render_icons.py")
PNG_SIZES = [16, 32, 48, 64, 128, 256, 512]


def test_desktop_file():
    print("Testing desktop entry file...")
    assert os.path.exists(DESKTOP_FILE), f"Desktop file missing: {DESKTOP_FILE}"

    # Run desktop-file-validate
    res = subprocess.run(["desktop-file-validate", DESKTOP_FILE], capture_output=True, text=True)
    assert res.returncode == 0, f"desktop-file-validate failed: {res.stderr}\n{res.stdout}"

    # Parse and check key fields
    with open(DESKTOP_FILE, "r", encoding="utf-8") as f:
        content = f.read()

    lines = [line.strip() for line in content.splitlines() if line.strip() and not line.startswith("#")]
    entries = {}
    current_section = None
    sections = {}

    for line in lines:
        if line.startswith("[") and line.endswith("]"):
            current_section = line[1:-1]
            sections[current_section] = {}
        elif "=" in line and current_section:
            k, v = line.split("=", 1)
            sections[current_section][k.strip()] = v.strip()

    assert "Desktop Entry" in sections, "Missing [Desktop Entry] section"
    de = sections["Desktop Entry"]

    assert de.get("Type") == "Application"
    assert de.get("Name") == "outPlayer"
    assert de.get("GenericName") == "Media Player"
    assert "High-performance modern media player" in de.get("Comment", "")
    assert de.get("Exec") == "outplayer %U"
    assert de.get("Icon") == "org.outplayer.outPlayer"
    assert de.get("Terminal") == "false"
    assert de.get("StartupNotify") == "true"
    assert de.get("StartupWMClass") == "outplayer"
    assert "AudioVideo;" in de.get("Categories", "")
    assert "Player;" in de.get("Categories", "")

    # MimeTypes check (at least 30)
    mimes = [m for m in de.get("MimeType", "").split(";") if m]
    assert len(mimes) >= 30, f"Expected >= 30 MIME types, got {len(mimes)}: {mimes}"
    required_mimes = [
        "video/mp4",
        "video/x-matroska",
        "video/quicktime",
        "video/webm",
        "video/x-msvideo",
        "audio/mpeg",
        "audio/flac",
        "audio/ogg",
    ]
    for rm in required_mimes:
        assert rm in mimes, f"Missing required MIME type: {rm}"

    # Actions check
    actions = [a for a in de.get("Actions", "").split(";") if a]
    expected_actions = ["PlayPause", "Stop", "Next", "Previous"]
    for ea in expected_actions:
        assert ea in actions, f"Missing action {ea} in Actions list"
        section_name = f"Desktop Action {ea}"
        assert section_name in sections, f"Missing section [{section_name}]"
        sec = sections[section_name]
        assert "Name" in sec, f"Missing Name in [{section_name}]"
        assert "Exec" in sec, f"Missing Exec in [{section_name}]"

    print("Desktop entry file tests PASSED.")


def test_appstream_metainfo():
    print("Testing AppStream metainfo XML...")
    assert os.path.exists(METAINFO_FILE), f"Metainfo file missing: {METAINFO_FILE}"

    # Run appstreamcli validate
    res = subprocess.run(["appstreamcli", "validate", "--no-net", METAINFO_FILE], capture_output=True, text=True)
    assert res.returncode == 0, f"appstreamcli validate failed: {res.stderr}\n{res.stdout}"

    # Parse XML and inspect nodes
    tree = ET.parse(METAINFO_FILE)
    root = tree.getroot()

    assert root.tag == "component", f"Root tag must be <component>, got <{root.tag}>"
    assert root.attrib.get("type") == "desktop-application", "Component type must be desktop-application"

    elem_id = root.find("id")
    assert elem_id is not None and elem_id.text == "org.outplayer.outPlayer"

    elem_meta_lic = root.find("metadata_license")
    assert elem_meta_lic is not None and elem_meta_lic.text == "CC0-1.0"

    elem_proj_lic = root.find("project_license")
    assert elem_proj_lic is not None and "GPL-3.0" in elem_proj_lic.text

    elem_name = root.find("name")
    assert elem_name is not None and elem_name.text == "outPlayer"

    elem_summary = root.find("summary")
    assert elem_summary is not None and len(elem_summary.text) > 10

    elem_desc = root.find("description")
    assert elem_desc is not None and len(elem_desc) > 0

    elem_launchable = root.find("launchable")
    assert elem_launchable is not None
    assert elem_launchable.attrib.get("type") == "desktop-id"
    assert elem_launchable.text == "org.outplayer.outPlayer.desktop"

    urls = {u.attrib.get("type"): u.text for u in root.findall("url")}
    assert "homepage" in urls
    assert "bugtracker" in urls

    elem_provides = root.find("provides")
    assert elem_provides is not None
    bin_elem = elem_provides.find("binary")
    assert bin_elem is not None and bin_elem.text == "outplayer"

    elem_oars = root.find("content_rating")
    assert elem_oars is not None and elem_oars.attrib.get("type") == "oars-1.1"

    elem_releases = root.find("releases")
    assert elem_releases is not None
    release_elems = elem_releases.findall("release")
    assert len(release_elems) > 0
    assert release_elems[0].attrib.get("version") == "0.1.1"

    print("AppStream metainfo tests PASSED.")


def test_icons():
    print("Testing icon assets...")
    assert os.path.exists(SVG_ICON), f"Scalable SVG icon missing: {SVG_ICON}"
    assert os.path.getsize(SVG_ICON) > 100, "SVG icon file is empty or too small"

    for size in PNG_SIZES:
        png_path = os.path.join(ICONS_DIR, f"{size}x{size}", "apps", "org.outplayer.outPlayer.png")
        assert os.path.exists(png_path), f"PNG icon missing for size {size}x{size}: {png_path}"
        assert os.path.getsize(png_path) > 50, f"PNG icon {png_path} is empty or too small"

    assert os.path.exists(RENDER_SCRIPT), f"Render script missing: {RENDER_SCRIPT}"
    print("Icon asset tests PASSED.")


def test_cmake_install():
    print("Testing CMake install rules...")
    test_prefix = "/tmp/outplayer-test-install"
    if os.path.exists(test_prefix):
        shutil.rmtree(test_prefix)

    build_dir = os.path.join(REPO_ROOT, "build")
    assert os.path.exists(build_dir), f"Build directory does not exist: {build_dir}"

    res = subprocess.run(
        ["cmake", "--install", build_dir, "--prefix", test_prefix],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    assert res.returncode == 0, f"cmake --install failed: {res.stderr}\n{res.stdout}"

    # Verify installed files
    bin_path = os.path.join(test_prefix, "bin", "outplayer")
    assert os.path.exists(bin_path), f"Installed binary missing: {bin_path}"

    installed_desktop = os.path.join(test_prefix, "share", "applications", "org.outplayer.outPlayer.desktop")
    assert os.path.exists(installed_desktop), f"Installed desktop file missing: {installed_desktop}"

    installed_metainfo = os.path.join(test_prefix, "share", "metainfo", "org.outplayer.outPlayer.metainfo.xml")
    assert os.path.exists(installed_metainfo), f"Installed metainfo XML missing: {installed_metainfo}"

    installed_svg = os.path.join(
        test_prefix, "share", "icons", "hicolor", "scalable", "apps", "org.outplayer.outPlayer.svg"
    )
    assert os.path.exists(installed_svg), f"Installed SVG icon missing: {installed_svg}"

    for size in PNG_SIZES:
        png_path = os.path.join(
            test_prefix, "share", "icons", "hicolor", f"{size}x{size}", "apps", "org.outplayer.outPlayer.png"
        )
        assert os.path.exists(png_path), f"Installed PNG icon missing: {png_path}"

    print("CMake install rules tests PASSED.")


def test_clean_room():
    print("Testing clean-room trademark purity...")
    # Tokens split to avoid self-matching
    forbidden = [
        "".join(["p", "o", "t", "p", "l", "a", "y", "e", "r"]),
        "".join(["d", "a", "u", "m"]),
        "".join(["k", "a", "k", "a", "o"]),
    ]

    files_to_check = [
        DESKTOP_FILE,
        METAINFO_FILE,
        SVG_ICON,
        RENDER_SCRIPT,
        os.path.join(REPO_ROOT, "CMakeLists.txt"),
        os.path.join(REPO_ROOT, "src", "CMakeLists.txt"),
        os.path.abspath(__file__),
    ]

    for fpath in files_to_check:
        if not os.path.exists(fpath):
            continue
        try:
            with open(fpath, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read().lower()
        except Exception as e:
            continue

        for token in forbidden:
            assert token not in content, f"Clean-room violation: found forbidden trademark '{token}' in {fpath}"

    print("Clean-room trademark purity tests PASSED.")


if __name__ == "__main__":
    try:
        test_desktop_file()
        test_appstream_metainfo()
        test_icons()
        test_cmake_install()
        test_clean_room()
        print("\nAll packaging asset tests PASSED successfully!")
    except AssertionError as err:
        print(f"\nTEST FAILED: {err}", file=sys.stderr)
        sys.exit(1)
