#!/usr/bin/env python3
"""
Unit and integration tests for outPlayer AppImage packaging,
AppRun entrypoint, AppDir staging, and standalone execution.
"""

import os
import platform
import shutil
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
APPIMAGE_DIR = os.path.join(REPO_ROOT, "packaging", "appimage")
APPRUN_FILE = os.path.join(APPIMAGE_DIR, "AppRun")
BUILD_APPIMAGE_SCRIPT = os.path.join(APPIMAGE_DIR, "build-appimage.sh")
DIST_DIR = os.path.join(REPO_ROOT, "dist")

ARCH = platform.machine()
VERSION = "0.1.1"
APPDIR_NAME = f"outPlayer-{VERSION}-{ARCH}.AppDir"
APPDIR_PATH = os.path.join(DIST_DIR, APPDIR_NAME)
TARBALL_PATH = os.path.join(DIST_DIR, f"{APPDIR_NAME}.tar.gz")
APPIMAGE_PATH = os.path.join(DIST_DIR, f"outPlayer-{VERSION}-{ARCH}.AppImage")


def test_files_exist():
    print("Testing AppImage packaging files existence and permissions...")
    required_files = [APPRUN_FILE, BUILD_APPIMAGE_SCRIPT]
    for rf in required_files:
        assert os.path.exists(rf), f"Required packaging file missing: {rf}"
        assert os.access(rf, os.X_OK), f"File is not executable: {rf}"
    print("AppImage packaging files existence PASSED.")


def test_bash_syntax():
    print("Testing bash syntax for AppRun and build-appimage.sh...")
    for script in [APPRUN_FILE, BUILD_APPIMAGE_SCRIPT]:
        res = subprocess.run(["bash", "-n", script], capture_output=True, text=True)
        assert res.returncode == 0, f"Bash syntax check failed for {script}:\n{res.stderr}"
    print("Bash syntax validation PASSED.")


def test_apprun_content():
    print("Testing AppRun entrypoint script contents and variables...")
    with open(APPRUN_FILE, "r", encoding="utf-8") as f:
        content = f.read()

    assert "readlink -f" in content, "AppRun must resolve its real directory using readlink -f"
    assert "APPDIR=" in content, "AppRun must export APPDIR"
    assert "PATH=" in content, "AppRun must export PATH"
    assert "LD_LIBRARY_PATH=" in content, "AppRun must export LD_LIBRARY_PATH"
    assert "QT_PLUGIN_PATH=" in content, "AppRun must export QT_PLUGIN_PATH"
    assert "XDG_DATA_DIRS=" in content, "AppRun must export XDG_DATA_DIRS"
    assert 'exec "${HERE}/usr/bin/outplayer" "$@"' in content or 'exec "$HERE/usr/bin/outplayer" "$@"' in content, (
        "AppRun must exec usr/bin/outplayer passing arguments"
    )
    print("AppRun entrypoint script contents PASSED.")


def test_build_appimage_script_execution():
    print("Testing build-appimage.sh execution and AppDir hierarchy...")
    res = subprocess.run(
        ["bash", BUILD_APPIMAGE_SCRIPT, DIST_DIR],
        cwd=REPO_ROOT,
        capture_output=True,
        text=True,
    )
    assert res.returncode == 0, (
        f"build-appimage.sh failed with exit code {res.returncode}:\n"
        f"Stdout:\n{res.stdout}\nStderr:\n{res.stderr}"
    )

    # Verify AppDir exists
    assert os.path.isdir(APPDIR_PATH), f"AppDir bundle not found at: {APPDIR_PATH}"

    # Verify AppDir tarball exists
    assert os.path.isfile(TARBALL_PATH), f"AppDir tarball not found at: {TARBALL_PATH}"
    assert os.path.getsize(TARBALL_PATH) > 0, f"AppDir tarball is empty: {TARBALL_PATH}"

    # Verify Root AppDir entries
    root_apprun = os.path.join(APPDIR_PATH, "AppRun")
    root_desktop = os.path.join(APPDIR_PATH, "org.outplayer.outPlayer.desktop")
    root_icon = os.path.join(APPDIR_PATH, "org.outplayer.outPlayer.png")
    root_diricon = os.path.join(APPDIR_PATH, ".DirIcon")

    assert os.path.exists(root_apprun), f"Missing {root_apprun}"
    assert os.access(root_apprun, os.X_OK), f"{root_apprun} is not executable"
    assert os.path.exists(root_desktop), f"Missing {root_desktop}"
    assert os.path.exists(root_icon), f"Missing {root_icon}"
    assert os.path.exists(root_diricon), f"Missing {root_diricon}"

    # Verify usr/bin/outplayer binary
    bin_file = os.path.join(APPDIR_PATH, "usr", "bin", "outplayer")
    assert os.path.isfile(bin_file), f"Missing outplayer binary: {bin_file}"
    assert os.access(bin_file, os.X_OK), f"outplayer binary is not executable: {bin_file}"

    # Verify usr/share assets
    share_desktop = os.path.join(APPDIR_PATH, "usr", "share", "applications", "org.outplayer.outPlayer.desktop")
    share_metainfo = os.path.join(APPDIR_PATH, "usr", "share", "metainfo", "org.outplayer.outPlayer.metainfo.xml")
    share_svg = os.path.join(APPDIR_PATH, "usr", "share", "icons", "hicolor", "scalable", "apps", "org.outplayer.outPlayer.svg")
    share_png_256 = os.path.join(APPDIR_PATH, "usr", "share", "icons", "hicolor", "256x256", "apps", "org.outplayer.outPlayer.png")

    assert os.path.isfile(share_desktop), f"Missing desktop asset: {share_desktop}"
    assert os.path.isfile(share_metainfo), f"Missing metainfo asset: {share_metainfo}"
    assert os.path.isfile(share_svg), f"Missing svg icon asset: {share_svg}"
    assert os.path.isfile(share_png_256), f"Missing 256x256 icon asset: {share_png_256}"

    # Verify plugins
    platforms_dir = os.path.join(APPDIR_PATH, "usr", "plugins", "platforms")
    assert os.path.isdir(platforms_dir), f"Missing platforms plugin directory: {platforms_dir}"
    qxcb_plugin = os.path.join(platforms_dir, "libqxcb.so")
    assert os.path.isfile(qxcb_plugin), f"Missing libqxcb.so plugin in {platforms_dir}"

    # Verify usr/lib directory exists
    lib_dir = os.path.join(APPDIR_PATH, "usr", "lib")
    assert os.path.isdir(lib_dir), f"Missing usr/lib directory: {lib_dir}"

    # If appimagetool was available, check that AppImage exists
    appimagetool_bin = shutil.which("appimagetool") or os.path.exists(os.path.join(REPO_ROOT, "tools", "appimagetool"))
    if appimagetool_bin:
        assert os.path.isfile(APPIMAGE_PATH), f"AppImage was not built at: {APPIMAGE_PATH}"
        assert os.access(APPIMAGE_PATH, os.X_OK), f"AppImage is not executable: {APPIMAGE_PATH}"

    print("build-appimage.sh execution and AppDir hierarchy PASSED.")


def test_appdir_runtime_execution():
    print("Testing AppDir standalone runnable execution with AppRun under xvfb...")
    root_apprun = os.path.join(APPDIR_PATH, "AppRun")
    assert os.path.isfile(root_apprun), f"AppRun not found: {root_apprun}"

    # In WSL2 environments where the repository resides on Windows drvfs (/mnt/c/...),
    # executing mmap'd shared libraries from drvfs can trigger Windows 9P lock contention.
    # We verify standalone execution by running AppRun in an isolated staging root in /tmp.
    staging_appdir = os.path.join("/tmp", APPDIR_NAME)
    shutil.rmtree(staging_appdir, ignore_errors=True)
    subprocess.run(["tar", "-xzf", TARBALL_PATH, "-C", "/tmp"], check=True)
    staged_apprun = os.path.join(staging_appdir, "AppRun")

    cmd = [
        "xvfb-run",
        "-a",
        staged_apprun,
        "--test-run",
        "lavfi://nullsrc=s=640x360:d=1",
    ]
    env = os.environ.copy()
    env["QT_QPA_PLATFORM"] = "xcb"

    res = subprocess.run(cmd, env=env, capture_output=True, text=True, timeout=30)
    assert res.returncode == 0, (
        f"AppDir AppRun failed with exit code {res.returncode}:\n"
        f"Stdout:\n{res.stdout}\nStderr:\n{res.stderr}"
    )
    shutil.rmtree(staging_appdir, ignore_errors=True)
    print("AppDir standalone runnable execution PASSED.")


def test_clean_room():
    print("Testing clean-room trademark purity...")
    forbidden = [
        "".join(["p", "o", "t", "p", "l", "a", "y", "e", "r"]),
        "".join(["d", "a", "u", "m"]),
        "".join(["k", "a", "k", "a", "o"]),
    ]

    files_to_check = [
        APPRUN_FILE,
        BUILD_APPIMAGE_SCRIPT,
        os.path.abspath(__file__),
    ]

    if os.path.exists(APPDIR_PATH):
        root_desktop = os.path.join(APPDIR_PATH, "org.outplayer.outPlayer.desktop")
        if os.path.exists(root_desktop):
            files_to_check.append(root_desktop)

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
        test_bash_syntax()
        test_apprun_content()
        test_build_appimage_script_execution()
        test_appdir_runtime_execution()
        test_clean_room()
        print("\nAll AppImage packaging tests PASSED successfully!")
    except AssertionError as err:
        print(f"\nTEST FAILED: {err}", file=sys.stderr)
        sys.exit(1)
