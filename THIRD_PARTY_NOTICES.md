# Third-Party Notices and Licensing

This document details the licensing of the **outPlayer** project and provides attribution and licensing notices for all third-party open-source components utilized.

---

## 1. outPlayer Project License

**outPlayer** is free and open-source software licensed under the **GNU General Public License, Version 3 or later (GPL-3.0-or-later)**.

```
Copyright (C) 2026 The outPlayer Contributors.

outPlayer is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

outPlayer is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with outPlayer. If not, see <https://www.gnu.org/licenses/>.
```

---

## 2. Third-Party Libraries & Dependencies

### 2.1 Qt 6 Framework
- **Project:** Qt (QtCore, QtGui, QtWidgets, QtOpenGLWidgets, QtDBus)
- **Website:** <https://www.qt.io>
- **License:** GNU Lesser General Public License version 3 (LGPL-3.0-only or LGPL-3.0-or-later) / GNU General Public License version 3 (GPL-3.0).
- **Linking:** **Dynamically Linked**. outPlayer links dynamically against Qt 6 shared libraries (`libQt6Widgets.so`, `libQt6OpenGLWidgets.so`, `libQt6Gui.so`, `libQt6Core.so`). Users have the freedom to replace, relink, or recompile Qt 6 libraries in accordance with LGPLv3 Section 4.
- **Copyright:** Copyright (C) 2026 The Qt Company Ltd. and other contributors.

### 2.2 libmpv (mpv media player library)
- **Project:** mpv
- **Website:** <https://mpv.io>
- **License:** GNU General Public License version 2 or later (GPL-2.0-or-later) / GNU Lesser General Public License version 2.1 or later (LGPL-2.1-or-later). When compiled with GPL components enabled (e.g., FFmpeg GPL components or libass), libmpv as a whole is covered under GPL-2.0-or-later.
- **Copyright:** Copyright (C) mpv developers and contributors.

### 2.3 FFmpeg
- **Project:** FFmpeg
- **Website:** <https://ffmpeg.org>
- **License:** GNU Lesser General Public License version 2.1 or later (LGPL-2.1-or-later) / GNU General Public License version 2 or later (GPL-2.0-or-later) depending on configuration.
- **Copyright:** Copyright (C) FFmpeg developers and contributors.

### 2.4 libass
- **Project:** libass
- **Website:** <https://github.com/libass/libass>
- **License:** ISC License
- **Copyright:** Copyright (C) 2006-2026 libass contributors.

---

## 3. Compliance and Clean-Room Certification

- **Clean-Room Attestation:** No proprietary code, binary assets, skins, audio files, icons, or trademarks from Daum PotPlayer, Kakao Corp, or other third parties have been included or copied into this repository.
- **Dynamic Linking:** All dependencies are resolved dynamically via system package management (`pkg-config`, shared `.so` libraries, or Flatpak runtimes).
