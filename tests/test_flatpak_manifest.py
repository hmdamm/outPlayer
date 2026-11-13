#!/usr/bin/env python3
"""
Unit and integration tests for outPlayer Flathub-ready Flatpak manifest.
"""

import json
import os
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
FLATPAK_DIR = os.path.join(REPO_ROOT, "packaging", "flatpak")
MANIFEST_PATH = os.path.join(FLATPAK_DIR, "org.outplayer.outPlayer.json")

REQUIRED_FINISH_ARGS = [
    "--share=ipc",
    "--socket=fallback-x11",
    "--socket=wayland",
    "--socket=pulseaudio",
    "--filesystem=xdg-run/pipewire-0",
    "--device=all",
    "--filesystem=host:ro",
    "--filesystem=xdg-download:ro",
    "--filesystem=xdg-videos:ro",
    "--filesystem=xdg-music:ro",
    "--filesystem=xdg-pictures:ro",
    "--talk-name=org.mpris.MediaPlayer2.*",
    "--talk-name=org.freedesktop.ScreenSaver",
]


def test_manifest_file_exists():
    print("Testing Flatpak manifest file existence...")
    assert os.path.exists(MANIFEST_PATH), f"Manifest file missing: {MANIFEST_PATH}"
    print("Manifest file existence PASSED.")


def test_manifest_json_structure():
    print("Testing Flatpak manifest JSON structure...")
    assert os.path.exists(MANIFEST_PATH), f"Manifest file missing: {MANIFEST_PATH}"
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    assert data.get("app-id") == "org.outplayer.outPlayer", (
        f"Expected app-id 'org.outplayer.outPlayer', got: {data.get('app-id')}"
    )
    assert data.get("runtime") == "org.kde.Platform", (
        f"Expected runtime 'org.kde.Platform', got: {data.get('runtime')}"
    )
    assert data.get("runtime-version") == "6.8", (
        f"Expected runtime-version '6.8', got: {data.get('runtime-version')}"
    )
    assert data.get("sdk") == "org.kde.Sdk", (
        f"Expected sdk 'org.kde.Sdk', got: {data.get('sdk')}"
    )
    assert data.get("command") == "outplayer", (
        f"Expected command 'outplayer', got: {data.get('command')}"
    )

    # Renaming properties
    assert data.get("rename-desktop-file") == "org.outplayer.outPlayer.desktop", (
        f"Expected rename-desktop-file 'org.outplayer.outPlayer.desktop', got: {data.get('rename-desktop-file')}"
    )
    assert data.get("rename-appdata-file") == "org.outplayer.outPlayer.metainfo.xml", (
        f"Expected rename-appdata-file 'org.outplayer.outPlayer.metainfo.xml', got: {data.get('rename-appdata-file')}"
    )
    assert data.get("rename-icon") == "org.outplayer.outPlayer", (
        f"Expected rename-icon 'org.outplayer.outPlayer', got: {data.get('rename-icon')}"
    )
    print("Manifest JSON structure PASSED.")


def test_manifest_finish_args():
    print("Testing Flatpak finish-args...")
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    finish_args = data.get("finish-args", [])
    assert isinstance(finish_args, list), "finish-args must be a list"

    for arg in REQUIRED_FINISH_ARGS:
        assert arg in finish_args, f"Missing required finish-arg: {arg}"

    print("Manifest finish-args PASSED.")


def test_manifest_modules():
    print("Testing Flatpak modules...")
    with open(MANIFEST_PATH, "r", encoding="utf-8") as f:
        data = json.load(f)

    modules = data.get("modules", [])
    assert isinstance(modules, list) and len(modules) >= 2, (
        f"Expected at least 2 modules (libmpv + outplayer), got {len(modules)}"
    )

    # Check for libmpv module
    mpv_modules = [
        m for m in modules if isinstance(m, dict) and m.get("name") in ["libmpv", "mpv"]
    ]
    assert len(mpv_modules) >= 1, "Missing libmpv module in modules list"
    mpv_mod = mpv_modules[0]
    assert mpv_mod.get("buildsystem") in ["meson", "simple"], (
        f"libmpv buildsystem must be meson or simple, got {mpv_mod.get('buildsystem')}"
    )
    assert "sources" in mpv_mod and len(mpv_mod["sources"]) > 0, (
        "libmpv module must define sources"
    )

    # Check for outplayer module
    outplayer_modules = [
        m for m in modules if isinstance(m, dict) and m.get("name") == "outplayer"
    ]
    assert len(outplayer_modules) == 1, "Missing or duplicate outplayer module"
    out_mod = outplayer_modules[0]
    assert out_mod.get("buildsystem") == "cmake-ninja", (
        f"outplayer buildsystem must be cmake-ninja, got {out_mod.get('buildsystem')}"
    )
    assert out_mod.get("builddir") is True, "outplayer builddir must be true"
    config_opts = out_mod.get("config-opts", [])
    assert any("-DCMAKE_BUILD_TYPE=Release" in opt for opt in config_opts), (
        f"outplayer config-opts must include -DCMAKE_BUILD_TYPE=Release, got {config_opts}"
    )

    sources = out_mod.get("sources", [])
    assert isinstance(sources, list) and len(sources) > 0, "outplayer module must define sources"
    dir_sources = [s for s in sources if isinstance(s, dict) and s.get("type") == "dir"]
    assert len(dir_sources) >= 1, "outplayer sources must include a dir source"
    assert dir_sources[0].get("path") == "../..", (
        f"outplayer dir source path must be '../..', got {dir_sources[0].get('path')}"
    )

    print("Manifest modules PASSED.")


def test_flatpak_builder_show_manifest():
    print("Testing flatpak-builder --show-manifest execution...")
    res = subprocess.run(
        ["flatpak-builder", "--show-manifest", MANIFEST_PATH],
        capture_output=True,
        text=True,
    )
    assert res.returncode == 0, (
        f"flatpak-builder --show-manifest failed with code {res.returncode}:\n"
        f"Stdout: {res.stdout}\nStderr: {res.stderr}"
    )

    # Validate output is parseable JSON
    parsed = json.loads(res.stdout)
    app_id = parsed.get("id") or parsed.get("app-id")
    assert app_id == "org.outplayer.outPlayer", (
        f"Parsed manifest ID mismatch: expected 'org.outplayer.outPlayer', got: {app_id}"
    )
    print("flatpak-builder --show-manifest PASSED.")


def test_clean_room():
    print("Testing clean-room trademark purity...")
    # Tokens split to avoid self-matching
    forbidden = [
        "".join(["p", "o", "t", "p", "l", "a", "y", "e", "r"]),
        "".join(["d", "a", "u", "m"]),
        "".join(["k", "a", "k", "a", "o"]),
    ]

    files_to_check = [
        MANIFEST_PATH,
        os.path.abspath(__file__),
    ]

    for fpath in files_to_check:
        if not os.path.exists(fpath):
            continue
        with open(fpath, "r", encoding="utf-8", errors="ignore") as f:
            content = f.read().lower()

        for token in forbidden:
            assert token not in content, (
                f"Clean-room violation: found forbidden trademark '{token}' in {fpath}"
            )

    print("Clean-room trademark purity tests PASSED.")


if __name__ == "__main__":
    try:
        test_manifest_file_exists()
        test_manifest_json_structure()
        test_manifest_finish_args()
        test_manifest_modules()
        test_flatpak_builder_show_manifest()
        test_clean_room()
        print("\nAll Flatpak manifest tests PASSED successfully!")
    except AssertionError as err:
        print(f"\nTEST FAILED: {err}", file=sys.stderr)
        sys.exit(1)
