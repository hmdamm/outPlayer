#!/usr/bin/env python3
"""
Unit and integration tests for outPlayer Fedora/RPM spec, RPM build script,
and Arch Linux PKGBUILD packaging definitions.
"""

import glob
import os
import re
import shutil
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
RPM_DIR = os.path.join(REPO_ROOT, "packaging", "rpm")
ARCH_DIR = os.path.join(REPO_ROOT, "packaging", "arch")

SPEC_FILE = os.path.join(RPM_DIR, "outplayer.spec")
BUILD_RPM_SCRIPT = os.path.join(RPM_DIR, "build-rpm.sh")
PKGBUILD_FILE = os.path.join(ARCH_DIR, "PKGBUILD")


def test_files_exist():
    print("Testing RPM and Arch packaging files existence...")
    required_files = [
        SPEC_FILE,
        BUILD_RPM_SCRIPT,
        PKGBUILD_FILE,
    ]
    for rf in required_files:
        assert os.path.exists(rf), f"Required packaging file missing: {rf}"
    print("RPM and Arch packaging files existence PASSED.")


def test_rpm_spec_query():
    print("Testing packaging/rpm/outplayer.spec with rpmspec...")
    assert os.path.exists(SPEC_FILE), f"Missing RPM spec: {SPEC_FILE}"

    # Query tags via rpmspec
    query_cmd = [
        "rpmspec",
        "-q",
        "--queryformat",
        "%{NAME}|%{VERSION}|%{RELEASE}|%{LICENSE}|%{URL}|%{SUMMARY}\n",
        SPEC_FILE,
    ]
    res = subprocess.run(query_cmd, cwd=REPO_ROOT, capture_output=True, text=True)
    assert res.returncode == 0, f"rpmspec query failed:\nStdout: {res.stdout}\nStderr: {res.stderr}"

    lines = [line.strip() for line in res.stdout.strip().splitlines() if line.strip()]
    assert len(lines) >= 1, f"rpmspec returned no output: {res.stdout}"

    fields = lines[0].split("|")
    assert len(fields) >= 6, f"Expected 6 pipe-delimited fields, got: {fields}"

    name, version, release, license_tag, url, summary = fields[0], fields[1], fields[2], fields[3], fields[4], fields[5]

    assert name == "outplayer", f"Expected package name 'outplayer', got: '{name}'"
    assert version == "0.1.0", f"Expected version '0.1.0', got: '{version}'"
    assert release.startswith("1"), f"Expected release starting with '1', got: '{release}'"
    assert "GPL-3.0" in license_tag, f"Expected GPL-3.0 license, got: '{license_tag}'"
    assert url == "https://github.com/outplayer/outplayer", f"Expected URL 'https://github.com/outplayer/outplayer', got: '{url}'"
    assert "media player" in summary.lower(), f"Expected media player in summary, got: '{summary}'"

    print("packaging/rpm/outplayer.spec rpmspec query PASSED.")


def test_rpm_spec_content():
    print("Testing packaging/rpm/outplayer.spec contents and sections...")
    with open(SPEC_FILE, "r", encoding="utf-8") as f:
        spec_text = f.read()

    # Check BuildRequires
    expected_build_deps = [
        "cmake",
        "ninja-build",
        "gcc-c++",
        "pkgconfig",
        "qt6-qtbase-devel",
        "qt6-qtsvg-devel",
        "mpv-libs-devel",
    ]
    for dep in expected_build_deps:
        assert dep in spec_text, f"Missing BuildRequires '{dep}' in spec file"

    # Check Requires
    expected_runtime_deps = [
        "qt6-qtbase",
        "qt6-qtsvg",
        "mpv-libs",
        "hicolor-icon-theme",
    ]
    for dep in expected_runtime_deps:
        assert dep in spec_text, f"Missing Requires '{dep}' in spec file"

    # Check Sections
    assert "%description" in spec_text, "Missing %description in spec file"
    assert "%build" in spec_text, "Missing %build in spec file"
    assert "%install" in spec_text, "Missing %install in spec file"
    assert "%files" in spec_text, "Missing %files in spec file"
    assert "%changelog" in spec_text, "Missing %changelog in spec file"

    # Check %files list
    expected_files = [
        "%{_bindir}/outplayer",
        "%{_datadir}/applications/org.outplayer.outPlayer.desktop",
        "%{_datadir}/metainfo/org.outplayer.outPlayer.metainfo.xml",
        "%{_datadir}/icons/hicolor",
    ]
    for ef in expected_files:
        assert ef in spec_text, f"Missing file entry '{ef}' in %files section"

    # Check changelog
    assert "dev@outplayer.org" in spec_text, "Missing maintainer email in %changelog"
    assert "0.1.0-1" in spec_text, "Missing version 0.1.0-1 in %changelog"

    print("packaging/rpm/outplayer.spec contents PASSED.")


def test_build_rpm_script():
    print("Testing packaging/rpm/build-rpm.sh execution and package verification...")
    assert os.path.exists(BUILD_RPM_SCRIPT), f"Missing build script: {BUILD_RPM_SCRIPT}"
    assert os.access(BUILD_RPM_SCRIPT, os.X_OK), f"build script must be executable: {BUILD_RPM_SCRIPT}"

    test_dist = os.path.join(REPO_ROOT, "dist", "rpm_test")
    if os.path.exists(test_dist):
        shutil.rmtree(test_dist)
    os.makedirs(test_dist, exist_ok=True)

    try:
        # Run build-rpm.sh
        res = subprocess.run([BUILD_RPM_SCRIPT, test_dist], cwd=REPO_ROOT, capture_output=True, text=True)
        assert res.returncode == 0, f"build-rpm.sh failed with code {res.returncode}:\nStdout: {res.stdout}\nStderr: {res.stderr}"

        rpm_files = glob.glob(os.path.join(test_dist, "**", "*.rpm"), recursive=True)
        assert len(rpm_files) > 0, f"No RPM packages found in {test_dist}"

        # Find binary RPM (non-src)
        binary_rpms = [r for r in rpm_files if not r.endswith(".src.rpm")]
        assert len(binary_rpms) > 0, f"No binary RPM package generated in {test_dist}: {rpm_files}"

        bin_rpm = binary_rpms[0]
        rpm_size = os.path.getsize(bin_rpm)
        assert rpm_size > 50000, f"Generated RPM package suspiciously small ({rpm_size} bytes)"

        # Inspect RPM file contents using rpm -qpl
        qpl_res = subprocess.run(["rpm", "-qpl", bin_rpm], capture_output=True, text=True)
        assert qpl_res.returncode == 0, f"rpm -qpl failed:\n{qpl_res.stderr}\n{qpl_res.stdout}"
        contents = qpl_res.stdout

        required_rpm_contents = [
            "/usr/bin/outplayer",
            "/usr/share/applications/org.outplayer.outPlayer.desktop",
            "/usr/share/metainfo/org.outplayer.outPlayer.metainfo.xml",
            "/usr/share/icons/hicolor/scalable/apps/org.outplayer.outPlayer.svg",
            "/usr/share/icons/hicolor/48x48/apps/org.outplayer.outPlayer.png",
            "/usr/share/icons/hicolor/128x128/apps/org.outplayer.outPlayer.png",
        ]
        for item in required_rpm_contents:
            assert item in contents, f"Missing expected path '{item}' in RPM contents:\n{contents}"

        # Inspect RPM metadata using rpm -qpi
        qpi_res = subprocess.run(["rpm", "-qpi", bin_rpm], capture_output=True, text=True)
        assert qpi_res.returncode == 0, f"rpm -qpi failed:\n{qpi_res.stderr}\n{qpi_res.stdout}"
        qpi_out = qpi_res.stdout

        assert re.search(r"^Name\s*:\s*outplayer\b", qpi_out, re.MULTILINE), f"Missing Name in rpm -qpi:\n{qpi_out}"
        assert re.search(r"^Version\s*:\s*0\.1\.0\b", qpi_out, re.MULTILINE), f"Missing Version in rpm -qpi:\n{qpi_out}"
        assert "GPL-3.0" in qpi_out, f"Missing GPL-3.0 in rpm -qpi:\n{qpi_out}"

        print(f"RPM package successfully built ({rpm_size} bytes) and validated: {bin_rpm}")
    finally:
        if os.path.exists(test_dist):
            shutil.rmtree(test_dist)

    print("packaging/rpm/build-rpm.sh PASSED.")


def test_arch_pkgbuild():
    print("Testing packaging/arch/PKGBUILD syntax and standards...")
    assert os.path.exists(PKGBUILD_FILE), f"Missing PKGBUILD: {PKGBUILD_FILE}"

    # Check bash syntax using bash -n
    syntax_res = subprocess.run(["bash", "-n", PKGBUILD_FILE], capture_output=True, text=True)
    assert syntax_res.returncode == 0, f"PKGBUILD syntax error:\n{syntax_res.stderr}"

    # Extract variables by sourcing in bash
    inspect_cmd = [
        "bash",
        "-c",
        f"source '{PKGBUILD_FILE}' && "
        "echo pkgname=$pkgname && "
        "echo pkgver=$pkgver && "
        "echo pkgrel=$pkgrel && "
        "echo pkgdesc=$pkgdesc && "
        "echo url=$url && "
        "echo arch=${arch[*]} && "
        "echo license=${license[*]} && "
        "echo depends=${depends[*]} && "
        "echo makedepends=${makedepends[*]}",
    ]
    res = subprocess.run(inspect_cmd, capture_output=True, text=True)
    assert res.returncode == 0, f"Failed to extract variables from PKGBUILD:\nStderr: {res.stderr}"

    var_dict = {}
    for line in res.stdout.strip().splitlines():
        if "=" in line:
            k, v = line.split("=", 1)
            var_dict[k.strip()] = v.strip()

    assert var_dict.get("pkgname") == "outplayer", f"Unexpected pkgname: {var_dict.get('pkgname')}"
    assert var_dict.get("pkgver") == "0.1.0", f"Unexpected pkgver: {var_dict.get('pkgver')}"
    assert var_dict.get("pkgrel") == "1", f"Unexpected pkgrel: {var_dict.get('pkgrel')}"
    assert "media player" in var_dict.get("pkgdesc", "").lower(), f"Unexpected pkgdesc: {var_dict.get('pkgdesc')}"
    assert var_dict.get("url") == "https://github.com/outplayer/outplayer", f"Unexpected url: {var_dict.get('url')}"

    archs = var_dict.get("arch", "").split()
    assert "x86_64" in archs, f"Missing x86_64 in arch: {archs}"

    licenses = var_dict.get("license", "").split()
    assert any("GPL-3.0" in lic for lic in licenses), f"Missing GPL-3.0 in license: {licenses}"

    deps = var_dict.get("depends", "").split()
    assert "qt6-base" in deps, f"Missing qt6-base in depends: {deps}"
    assert "qt6-svg" in deps, f"Missing qt6-svg in depends: {deps}"
    assert "mpv" in deps, f"Missing mpv in depends: {deps}"

    makedeps = var_dict.get("makedepends", "").split()
    assert "cmake" in makedeps, f"Missing cmake in makedepends: {makedeps}"
    assert "ninja" in makedeps, f"Missing ninja in makedepends: {makedeps}"
    assert "pkgconf" in makedeps, f"Missing pkgconf in makedepends: {makedeps}"

    with open(PKGBUILD_FILE, "r", encoding="utf-8") as f:
        pkgbuild_text = f.read()

    # Verify functions
    assert re.search(r"build\s*\(\s*\)", pkgbuild_text), "Missing build() function in PKGBUILD"
    assert re.search(r"package\s*\(\s*\)", pkgbuild_text), "Missing package() function in PKGBUILD"
    assert "cmake" in pkgbuild_text, "Missing cmake invocation in PKGBUILD"
    assert "$pkgdir" in pkgbuild_text or "${pkgdir}" in pkgbuild_text, "Missing pkgdir reference in PKGBUILD"

    print("packaging/arch/PKGBUILD PASSED.")


def test_clean_room():
    print("Testing clean-room trademark purity...")
    forbidden = [
        "".join(["p", "o", "t", "p", "l", "a", "y", "e", "r"]),
        "".join(["d", "a", "u", "m"]),
        "".join(["k", "a", "k", "a", "o"]),
    ]

    files_to_check = [
        SPEC_FILE,
        BUILD_RPM_SCRIPT,
        PKGBUILD_FILE,
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
        test_files_exist()
        test_rpm_spec_query()
        test_rpm_spec_content()
        test_build_rpm_script()
        test_arch_pkgbuild()
        test_clean_room()
        print("\nAll RPM and Arch packaging tests PASSED successfully!")
    except AssertionError as err:
        print(f"\nTEST FAILED: {err}", file=sys.stderr)
        sys.exit(1)
