#!/usr/bin/env python3
"""
Unit and integration tests for outPlayer Debian/Ubuntu packaging structure,
DEP-5 copyright format, and .deb binary package builder.
"""

import os
import re
import shutil
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
DEBIAN_DIR = os.path.join(REPO_ROOT, "packaging", "debian")
CONTROL_FILE = os.path.join(DEBIAN_DIR, "control")
RULES_FILE = os.path.join(DEBIAN_DIR, "rules")
SOURCE_FORMAT_FILE = os.path.join(DEBIAN_DIR, "source", "format")
CHANGELOG_FILE = os.path.join(DEBIAN_DIR, "changelog")
COPYRIGHT_FILE = os.path.join(DEBIAN_DIR, "copyright")
INSTALL_FILE = os.path.join(DEBIAN_DIR, "outplayer.install")
BUILD_DEB_SCRIPT = os.path.join(DEBIAN_DIR, "build-deb.sh")


def parse_rfc822_paragraphs(filepath):
    """Parses a Debian control or DEP-5 file into a list of field dictionaries."""
    assert os.path.exists(filepath), f"File not found: {filepath}"
    with open(filepath, "r", encoding="utf-8") as f:
        content = f.read()

    paragraphs = []
    current_para = {}
    current_key = None

    for line in content.splitlines():
        if not line.strip():
            if current_para:
                paragraphs.append(current_para)
                current_para = {}
                current_key = None
            continue

        if line.startswith("#"):
            continue

        if line[0].isspace():
            if current_key and current_key in current_para:
                current_para[current_key] += "\n" + line.strip()
        elif ":" in line:
            key, val = line.split(":", 1)
            current_key = key.strip()
            current_para[current_key] = val.strip()

    if current_para:
        paragraphs.append(current_para)

    return paragraphs


def test_debian_files_exist():
    print("Testing Debian packaging files existence...")
    required_files = [
        CONTROL_FILE,
        RULES_FILE,
        SOURCE_FORMAT_FILE,
        CHANGELOG_FILE,
        COPYRIGHT_FILE,
        INSTALL_FILE,
        BUILD_DEB_SCRIPT,
    ]
    for rf in required_files:
        assert os.path.exists(rf), f"Required Debian packaging file missing: {rf}"
    print("Debian packaging files existence PASSED.")


def test_control_file():
    print("Testing packaging/debian/control syntax and requirements...")
    paras = parse_rfc822_paragraphs(CONTROL_FILE)
    assert len(paras) >= 2, f"control file must contain at least Source and Package blocks, got {len(paras)}"

    source_block = paras[0]
    assert source_block.get("Source") == "outplayer", f"Unexpected Source: {source_block.get('Source')}"
    assert source_block.get("Section") == "video", f"Unexpected Section: {source_block.get('Section')}"
    assert source_block.get("Priority") == "optional", f"Unexpected Priority: {source_block.get('Priority')}"
    assert "outPlayer Developers" in source_block.get("Maintainer", ""), f"Unexpected Maintainer: {source_block.get('Maintainer')}"
    assert "<dev@outplayer.org>" in source_block.get("Maintainer", ""), f"Missing maintainer email: {source_block.get('Maintainer')}"

    build_deps = source_block.get("Build-Depends", "")
    expected_deps = [
        "debhelper-compat (= 13)",
        "cmake (>= 3.22)",
        "ninja-build",
        "pkg-config",
        "qt6-base-dev",
        "libqt6opengl6-dev",
        "qt6-declarative-dev",
        "libqt6svg6-dev",
        "libmpv-dev (>= 0.35.0)",
    ]
    for dep in expected_deps:
        assert dep in build_deps, f"Missing Build-Depends '{dep}' in: {build_deps}"

    assert source_block.get("Standards-Version") == "4.6.2", f"Unexpected Standards-Version: {source_block.get('Standards-Version')}"
    assert source_block.get("Homepage") == "https://github.com/outplayer/outplayer", f"Unexpected Homepage: {source_block.get('Homepage')}"

    pkg_block = paras[1]
    assert pkg_block.get("Package") == "outplayer", f"Unexpected Package: {pkg_block.get('Package')}"
    assert pkg_block.get("Architecture") == "any", f"Unexpected Architecture: {pkg_block.get('Architecture')}"
    
    deps = pkg_block.get("Depends", "")
    assert "${shlibs:Depends}" in deps, f"Missing '${{shlibs:Depends}}' in Depends: {deps}"
    assert "${misc:Depends}" in deps, f"Missing '${{misc:Depends}}' in Depends: {deps}"

    desc = pkg_block.get("Description", "")
    assert "Modern, high-performance Linux media player" in desc, f"Description header mismatch: {desc}"
    assert len(desc.splitlines()) > 1, f"Description must be multiline, got: {desc}"

    print("packaging/debian/control PASSED.")


def test_rules_file():
    print("Testing packaging/debian/rules...")
    assert os.path.exists(RULES_FILE), f"Missing rules file: {RULES_FILE}"
    assert os.access(RULES_FILE, os.X_OK), f"rules file must be executable: {RULES_FILE}"

    with open(RULES_FILE, "r", encoding="utf-8") as f:
        content = f.read()

    assert "dh $@ --buildsystem=cmake+ninja" in content, (
        f"rules file must invoke 'dh $@ --buildsystem=cmake+ninja', got:\n{content}"
    )
    print("packaging/debian/rules PASSED.")


def test_source_format():
    print("Testing packaging/debian/source/format...")
    assert os.path.exists(SOURCE_FORMAT_FILE), f"Missing source format file: {SOURCE_FORMAT_FILE}"
    with open(SOURCE_FORMAT_FILE, "r", encoding="utf-8") as f:
        fmt = f.read().strip()
    assert fmt == "3.0 (native)", f"Expected '3.0 (native)', got: '{fmt}'"
    print("packaging/debian/source/format PASSED.")


def test_changelog():
    print("Testing packaging/debian/changelog...")
    assert os.path.exists(CHANGELOG_FILE), f"Missing changelog: {CHANGELOG_FILE}"
    with open(CHANGELOG_FILE, "r", encoding="utf-8") as f:
        lines = [l.strip() for l in f if l.strip()]

    assert len(lines) >= 3, f"changelog too short: {lines}"
    header = lines[0]
    match = re.match(r"^outplayer\s+\((.+)\)\s+([a-zA-Z0-9_-]+);\s+urgency=([a-zA-Z0-9]+)", header)
    assert match, f"Changelog header does not match Debian standard: {header}"
    version, dist, urgency = match.groups()
    assert version == "0.1.0-1", f"Expected version 0.1.0-1, got: {version}"

    trailer = lines[-1]
    assert trailer.startswith("-- "), f"Changelog trailer must start with '-- ', got: {trailer}"
    assert "outPlayer Developers <dev@outplayer.org>" in trailer, f"Missing maintainer in changelog trailer: {trailer}"
    print("packaging/debian/changelog PASSED.")


def test_copyright_dep5():
    print("Testing packaging/debian/copyright (DEP-5)...")
    paras = parse_rfc822_paragraphs(COPYRIGHT_FILE)
    assert len(paras) >= 2, f"DEP-5 copyright must have at least header and files paragraph, got {len(paras)}"

    header = paras[0]
    expected_format = "https://www.debian.org/doc/packaging-manuals/copyright-format/1.0/"
    assert header.get("Format") == expected_format, f"Unexpected Format: {header.get('Format')}"
    assert header.get("Upstream-Name") == "outplayer", f"Unexpected Upstream-Name: {header.get('Upstream-Name')}"

    files_para = paras[1]
    assert files_para.get("Files") == "*", f"Expected Files: *, got: {files_para.get('Files')}"
    assert "outPlayer Developers <dev@outplayer.org>" in files_para.get("Copyright", ""), (
        f"Unexpected Copyright: {files_para.get('Copyright')}"
    )
    assert files_para.get("License") == "GPL-3.0-or-later", f"Unexpected License: {files_para.get('License')}"

    print("packaging/debian/copyright (DEP-5) PASSED.")


def test_install_file():
    print("Testing packaging/debian/outplayer.install...")
    assert os.path.exists(INSTALL_FILE), f"Missing install file: {INSTALL_FILE}"
    with open(INSTALL_FILE, "r", encoding="utf-8") as f:
        lines = [l.strip() for l in f if l.strip() and not l.startswith("#")]

    required_install_patterns = [
        "usr/bin/outplayer",
        "usr/share/applications",
        "usr/share/metainfo",
        "usr/share/icons/hicolor",
    ]
    for pattern in required_install_patterns:
        matched = any(pattern in line for line in lines)
        assert matched, f"Missing install path pattern '{pattern}' in {INSTALL_FILE}: {lines}"

    print("packaging/debian/outplayer.install PASSED.")


def test_build_deb_script():
    print("Testing packaging/debian/build-deb.sh execution and package verification...")
    assert os.path.exists(BUILD_DEB_SCRIPT), f"Missing build script: {BUILD_DEB_SCRIPT}"
    assert os.access(BUILD_DEB_SCRIPT, os.X_OK), f"build script must be executable: {BUILD_DEB_SCRIPT}"

    # Determine architecture
    arch_res = subprocess.run(["dpkg", "--print-architecture"], capture_output=True, text=True)
    arch = arch_res.stdout.strip() if arch_res.returncode == 0 else "amd64"

    dist_dir = os.path.join(REPO_ROOT, "dist")
    expected_deb = os.path.join(dist_dir, f"outplayer_0.1.0-1_{arch}.deb")

    # Run build-deb.sh
    res = subprocess.run([BUILD_DEB_SCRIPT, dist_dir], cwd=REPO_ROOT, capture_output=True, text=True)
    assert res.returncode == 0, f"build-deb.sh failed with code {res.returncode}:\nStdout: {res.stdout}\nStderr: {res.stderr}"

    assert os.path.exists(expected_deb), f"Generated .deb package not found at: {expected_deb}"
    deb_size = os.path.getsize(expected_deb)
    assert deb_size > 50000, f"Generated .deb package suspiciously small ({deb_size} bytes)"

    # Inspect control information using dpkg-deb -I
    info_res = subprocess.run(["dpkg-deb", "-I", expected_deb], capture_output=True, text=True)
    assert info_res.returncode == 0, f"dpkg-deb -I failed:\n{info_res.stderr}\n{info_res.stdout}"
    control_info = info_res.stdout

    assert "Package: outplayer" in control_info, f"dpkg-deb -I missing 'Package: outplayer':\n{control_info}"
    assert "Version: 0.1.0-1" in control_info, f"dpkg-deb -I missing 'Version: 0.1.0-1':\n{control_info}"
    assert f"Architecture: {arch}" in control_info, f"dpkg-deb -I missing 'Architecture: {arch}':\n{control_info}"
    assert "Maintainer: outPlayer Developers <dev@outplayer.org>" in control_info, (
        f"dpkg-deb -I missing Maintainer:\n{control_info}"
    )

    # Inspect file contents using dpkg-deb -c
    contents_res = subprocess.run(["dpkg-deb", "-c", expected_deb], capture_output=True, text=True)
    assert contents_res.returncode == 0, f"dpkg-deb -c failed:\n{contents_res.stderr}\n{contents_res.stdout}"
    contents = contents_res.stdout

    required_contents = [
        "./usr/bin/outplayer",
        "./usr/share/applications/org.outplayer.outPlayer.desktop",
        "./usr/share/metainfo/org.outplayer.outPlayer.metainfo.xml",
        "./usr/share/icons/hicolor/scalable/apps/org.outplayer.outPlayer.svg",
        "./usr/share/icons/hicolor/48x48/apps/org.outplayer.outPlayer.png",
        "./usr/share/icons/hicolor/128x128/apps/org.outplayer.outPlayer.png",
    ]
    for item in required_contents:
        assert item in contents, f"Missing expected path '{item}' in .deb contents:\n{contents}"

    print(f".deb package built successfully ({deb_size} bytes) and validated with dpkg-deb!")
    print("packaging/debian/build-deb.sh PASSED.")


def test_clean_room():
    print("Testing clean-room trademark purity...")
    forbidden = [
        "".join(["p", "o", "t", "p", "l", "a", "y", "e", "r"]),
        "".join(["d", "a", "u", "m"]),
        "".join(["k", "a", "k", "a", "o"]),
    ]

    files_to_check = [
        CONTROL_FILE,
        RULES_FILE,
        SOURCE_FORMAT_FILE,
        CHANGELOG_FILE,
        COPYRIGHT_FILE,
        INSTALL_FILE,
        BUILD_DEB_SCRIPT,
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
        test_debian_files_exist()
        test_control_file()
        test_rules_file()
        test_source_format()
        test_changelog()
        test_copyright_dep5()
        test_install_file()
        test_build_deb_script()
        test_clean_room()
        print("\nAll Debian packaging tests PASSED successfully!")
    except AssertionError as err:
        print(f"\nTEST FAILED: {err}", file=sys.stderr)
        sys.exit(1)
