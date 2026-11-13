# Phase 5: Packaging & Distribution Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Create production-ready, reproducible packaging manifests, desktop integration assets, AppStream metadata, and installation blueprints for all major Linux distribution channels (Flatpak/Flathub, Debian/Ubuntu DEB, Fedora/RHEL RPM, Arch Linux AUR, and standalone AppImage), along with installation documentation and ADR 011.

**Architecture:**
- `data/`: Universal desktop integration assets:
  - `data/org.outplayer.outPlayer.desktop`: FreeDesktop compliant desktop file with extensive MIME associations, categories, and quicklist desktop actions.
  - `data/org.outplayer.outPlayer.metainfo.xml`: AppStream 1.0 metadata with release notes, OARS content rating, screenshots, categories, and developer identity.
  - `data/icons/hicolor/`: Scalable SVG vector icon and pre-rendered PNG icons (16, 32, 48, 64, 128, 256, 512).
- `CMakeLists.txt`: GNUInstallDirs compliant `install()` rules for binary, desktop entry, metainfo XML, and hicolor icons.
- `packaging/flatpak/`: Flathub-ready Flatpak manifest (`org.outplayer.outPlayer.json`) using KDE 6.8 runtime / Freedesktop SDK with full Wayland, X11, PipeWire, PulseAudio, and GPU acceleration permissions.
- `packaging/debian/`: Debian/Ubuntu source packaging structure (`debian/control`, `debian/rules`, `debian/changelog`, `debian/copyright`, `debian/compat`) targeting `dh $@ --buildsystem=cmake+ninja`.
- `packaging/rpm/`: Fedora/RHEL RPM spec file (`packaging/rpm/outplayer.spec`) with standard macro invocation (`%cmake`, `%ninja_build`, `%ninja_install`).
- `packaging/arch/`: Arch Linux / AUR package recipe (`packaging/arch/PKGBUILD`) with standard `build()` and `package()` routines.
- `packaging/appimage/`: AppImage deployment recipe and packaging script (`packaging/appimage/build-appimage.sh` and `packaging/appimage/AppRun`).
- `docs/distribution.md` & `README.md`: End-user installation guide, package verification, checksums, and ADR 011.

**Tech Stack:** CMake 3.22+, GNUInstallDirs, FreeDesktop Desktop Entry 1.5, AppStream 1.0, Flatpak, Debian debhelper, RPM spec, Arch PKGBUILD, AppImage.

**Spec:**
- `docs/spec/formats.md` (MIME types for desktop entry)
- `docs/spec/hotkeys.csv` (Quicklist actions)
- `docs/spec/features.csv` (Features list for AppStream description)
- `TASKS.md` (Phase 5 specifications)

## Global Constraints

- Modern C++20 standard; clean RAII memory management and parent-child `QObject` hierarchies.
- Strict Clean-Room rules: zero occurrences of forbidden proprietary trademarks (`PotPlayer`, `Daum`, `Kakao`) in packaging metadata, desktop files, AppStream XML, or documentation.
- Strict FreeDesktop.org standards: reverse-DNS naming `org.outplayer.outPlayer` for desktop files, AppStream metainfo, icon names, and D-Bus interfaces.
- 100% automated test pass rate under WSL2 Kali Linux with headless X11 (`QT_QPA_PLATFORM=xcb xvfb-run -a ctest ...`).

## Review Focus

1. **Clean-Room Trademark Purity:** Zero forbidden brand references in all packaging files, spec files, manifests, control files, changelogs, and documentation.
2. **FreeDesktop & AppStream Validation:** `desktop-file-validate` and `appstreamcli validate` must pass with zero fatal errors or warnings.
3. **CMake Installation Correctness:** `ninja install` under `DESTDIR` must install all components into standard hierarchy (`bin/`, `share/applications/`, `share/metainfo/`, `share/icons/hicolor/...`).
4. **Package Manifest Integrity:** Flatpak manifest, Debian control, RPM spec, and Arch PKGBUILD must parse cleanly without syntax errors and declare correct dependencies (`Qt6`, `libmpv`).
5. **Release Documentation Completeness:** Detailed step-by-step installation instructions for every supported packaging format.

---

### Task 1: Desktop Integration Assets, AppStream Metadata & CMake Installation Rules

**Files:**
- Create: `data/org.outplayer.outPlayer.desktop`
- Create: `data/org.outplayer.outPlayer.metainfo.xml`
- Create: `data/icons/hicolor/scalable/apps/org.outplayer.outPlayer.svg`
- Create: `data/icons/render_icons.py` (generates PNG icons 16, 32, 48, 64, 128, 256, 512)
- Modify: `CMakeLists.txt`
- Modify: `src/CMakeLists.txt`
- Create: `tests/test_packaging_assets.py`

**Interfaces:**
- Consumes: `docs/spec/formats.md`, `src/ui/ThemeManager.cpp`
- Produces:
  1. Validated `.desktop` entry with categories `AudioVideo;Player;Recorder;TV;` and 40+ MIME types.
  2. Validated `.metainfo.xml` AppStream file with `<component type="desktop-application">`.
  3. Scalable SVG and rasterized PNG icons in `data/icons/hicolor/`.
  4. CMake `install()` targets for all components.
  5. Validation test verifying desktop and appstream files via `desktop-file-validate` and `appstreamcli validate`.

- [ ] **Step 1: Write failing test in `tests/test_packaging_assets.py`**
Assert that desktop file exists, passes `desktop-file-validate`, metainfo exists and passes `appstreamcli validate`, icons exist in all standard sizes, and CMake `install` exports to `DESTDIR`.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl python3 tests/test_packaging_assets.py`
Expected: FAIL (files missing).

- [ ] **Step 3: Create desktop file and AppStream metainfo XML**
Create `data/org.outplayer.outPlayer.desktop` and `data/org.outplayer.outPlayer.metainfo.xml`.

- [ ] **Step 4: Create SVG icon and PNG icon generator**
Create `data/icons/hicolor/scalable/apps/org.outplayer.outPlayer.svg` with dark/amber motif and render standard PNG sizes.

- [ ] **Step 5: Add CMake install rules to `CMakeLists.txt` and `src/CMakeLists.txt`**
Use `include(GNUInstallDirs)` to install binary, desktop entry, metainfo, and icon directories.

- [ ] **Step 6: Run test to verify it passes**
Run: `wsl python3 tests/test_packaging_assets.py`
Expected: PASS.

- [ ] **Step 7: Commit**
```bash
git add data/ CMakeLists.txt src/CMakeLists.txt tests/test_packaging_assets.py
git commit -m "feat(packaging): add desktop entry, appstream metainfo, hicolor icons, and cmake install rules"
```

---

### Task 2: Flatpak Manifest & Flathub Blueprint

**Files:**
- Create: `packaging/flatpak/org.outplayer.outPlayer.json`
- Create: `tests/test_flatpak_manifest.py`

**Interfaces:**
- Consumes: `data/org.outplayer.outPlayer.desktop`, `data/org.outplayer.outPlayer.metainfo.xml`
- Produces:
  1. Complete Flatpak JSON manifest using `org.kde.Platform` 6.8 (or `org.freedesktop.Platform` 24.08).
  2. Finely tuned sandbox permissions: `--socket=wayland`, `--socket=fallback-x11`, `--share=ipc`, `--socket=pulseaudio`, `--device=all` (DRI/GPU acceleration and optical drives), `--filesystem=xdg-run/pipewire-0`, `--filesystem=host:ro` (media playback), `--talk-name=org.mpris.MediaPlayer2.*`, `--talk-name=org.freedesktop.ScreenSaver`.
  3. Module build definition invoking cmake with Ninja.

- [ ] **Step 1: Write failing test in `tests/test_flatpak_manifest.py`**
Validate JSON schema, permissions, app-id, and manifest consistency.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl python3 tests/test_flatpak_manifest.py`
Expected: FAIL (manifest missing).

- [ ] **Step 3: Implement `packaging/flatpak/org.outplayer.outPlayer.json`**
Write the complete manifest.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl python3 tests/test_flatpak_manifest.py`
Expected: PASS. Also verify using `flatpak-builder --show-manifest packaging/flatpak/org.outplayer.outPlayer.json` if available.

- [ ] **Step 5: Commit**
```bash
git add packaging/flatpak/ tests/test_flatpak_manifest.py
git commit -m "feat(packaging): create Flathub-ready Flatpak manifest and validation test"
```

---

### Task 3: Debian / Ubuntu Packaging (DEB)

**Files:**
- Create: `packaging/debian/control`
- Create: `packaging/debian/rules`
- Create: `packaging/debian/compat`
- Create: `packaging/debian/changelog`
- Create: `packaging/debian/copyright`
- Create: `packaging/debian/outplayer.install`
- Create: `packaging/debian/build-deb.sh`
- Create: `tests/test_debian_packaging.py`

**Interfaces:**
- Consumes: CMake build system and dependencies (`libmpv-dev`, `qt6-base-dev`, `qt6-declarative-dev`, `libgl-dev`)
- Produces:
  1. Standard Debian source packaging control files.
  2. Standalone packaging script `build-deb.sh` generating `.deb` binary package using `fakeroot` / `dpkg-deb`.
  3. Validation test verifying debian control syntax, copyright machine-readable format (DEP-5), and binary `.deb` creation.

- [ ] **Step 1: Write failing test in `tests/test_debian_packaging.py`**
Verify debian files exist, control syntax is valid, DEP-5 copyright is valid, and packaging builds cleanly.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl python3 tests/test_debian_packaging.py`
Expected: FAIL.

- [ ] **Step 3: Implement Debian packaging files and build script**
Implement `packaging/debian/*`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl python3 tests/test_debian_packaging.py`
Expected: PASS (generates valid `.deb` package and verifies content).

- [ ] **Step 5: Commit**
```bash
git add packaging/debian/ tests/test_debian_packaging.py
git commit -m "feat(packaging): implement Debian/Ubuntu packaging structure and deb builder"
```

---

### Task 4: Fedora / RPM Spec & Arch Linux PKGBUILD

**Files:**
- Create: `packaging/rpm/outplayer.spec`
- Create: `packaging/rpm/build-rpm.sh`
- Create: `packaging/arch/PKGBUILD`
- Create: `tests/test_rpm_arch_packaging.py`

**Interfaces:**
- Consumes: CMake install rules, `data/` assets
- Produces:
  1. Complete RPM spec file with `%prep`, `%build`, `%install`, `%files` macros and dependency definitions (`mpv-libs`, `qt6-qtbase`).
  2. RPM build validation via `rpmbuild` or spec syntax validation.
  3. Arch Linux `PKGBUILD` adhering to Arch Packaging Standards.
  4. Validation test checking RPM spec syntax and PKGBUILD bash syntax/variables.

- [ ] **Step 1: Write failing test in `tests/test_rpm_arch_packaging.py`**
Validate RPM spec syntax with `rpmbuild -bl` / `rpmlint` and Arch `PKGBUILD` variables (`pkgname`, `pkgver`, `depends`, `makedepends`).

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl python3 tests/test_rpm_arch_packaging.py`
Expected: FAIL.

- [ ] **Step 3: Implement RPM spec, build script, and Arch PKGBUILD**
Implement `packaging/rpm/outplayer.spec`, `packaging/rpm/build-rpm.sh`, and `packaging/arch/PKGBUILD`.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl python3 tests/test_rpm_arch_packaging.py`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add packaging/rpm/ packaging/arch/ tests/test_rpm_arch_packaging.py
git commit -m "feat(packaging): implement Fedora/RPM spec and Arch Linux PKGBUILD"
```

---

### Task 5: Standalone AppImage Recipe & Build Script

**Files:**
- Create: `packaging/appimage/AppRun`
- Create: `packaging/appimage/build-appimage.sh`
- Create: `tests/test_appimage_packaging.py`

**Interfaces:**
- Consumes: Installed `AppDir` tree (binary, libraries, Qt plugins, `data/` assets)
- Produces:
  1. Custom `AppRun` shell script with library path detection, Qt plugin path setup, and desktop file launch.
  2. `build-appimage.sh` staging an AppDir and packaging standalone AppImage.
  3. Validation test verifying AppDir layout, binary execution in AppDir environment, and clean-room compliance.

- [ ] **Step 1: Write failing test in `tests/test_appimage_packaging.py`**
Validate AppDir staging structure, `AppRun` script permissions and syntax, and application launch capability.

- [ ] **Step 2: Run test to verify it fails**
Run: `wsl python3 tests/test_appimage_packaging.py`
Expected: FAIL.

- [ ] **Step 3: Implement `packaging/appimage/AppRun` and `packaging/appimage/build-appimage.sh`**
Implement AppDir staging, desktop file / icon symlinking, and packaging script.

- [ ] **Step 4: Run test to verify it passes**
Run: `wsl python3 tests/test_appimage_packaging.py`
Expected: PASS.

- [ ] **Step 5: Commit**
```bash
git add packaging/appimage/ tests/test_appimage_packaging.py
git commit -m "feat(packaging): implement AppImage AppRun launcher and AppDir staging script"
```

---

### Task 6: Distribution Documentation, Release Portal & ADR 011

**Files:**
- Create: `docs/distribution.md`
- Modify: `docs/decisions.md` (record ADR 011)
- Modify: `README.md`
- Modify: `TASKS.md`

**Interfaces:**
- Consumes: All packaging deliverables from Tasks 1-5
- Produces:
  1. `docs/distribution.md`: Comprehensive user & package maintainer guide covering Flatpak, DEB, RPM, Arch AUR, and AppImage installation, checksums, and update channels.
  2. `ADR 011`: Architectural Decision Record for Multi-Distribution Packaging & Desktop Integration.
  3. `README.md`: Updated master project README with install instructions, architecture, screenshot mockups, feature matrix, and shortcuts.
  4. `TASKS.md`: Mark Phase 5 and entire project as 100% COMPLETE with full verification evidence.

- [ ] **Step 1: Author `docs/distribution.md`**
Detail installation methods for Flatpak, Ubuntu/Debian PPA/DEB, Fedora RPM, Arch AUR, and AppImage.

- [ ] **Step 2: Record ADR 011 in `docs/decisions.md`**
Record context, decisions, consequences, and compliance for multi-distribution packaging.

- [ ] **Step 3: Update `README.md`**
Incorporate packaging instructions, badges, architecture summary, hotkey table, and usage.

- [ ] **Step 4: Update `TASKS.md` with complete Phase 5 clearance evidence**
Mark P5-T1 through P5-T6 as Done. Summarize full project completion status.

- [ ] **Step 5: Run full verification suite**
Run:
- `wsl bash -c 'cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Debug && cmake --build build -j$(nproc)'`
- `wsl bash -c 'QT_QPA_PLATFORM=xcb xvfb-run -a ctest --test-dir build --output-on-failure'`
- `wsl python3 tests/test_packaging_assets.py`
- `wsl python3 tests/test_flatpak_manifest.py`
- `wsl python3 tests/test_debian_packaging.py`
- `wsl python3 tests/test_rpm_arch_packaging.py`
- `wsl python3 tests/test_appimage_packaging.py`
- `wsl python3 tests/test_phase1_specs.py`
- `wsl python3 tests/test_project_scaffold.py`

- [ ] **Step 6: Commit**
```bash
git add docs/distribution.md docs/decisions.md README.md TASKS.md
git commit -m "docs: complete Phase 5 release documentation, record ADR 011, and certify 100% project completion"
```
