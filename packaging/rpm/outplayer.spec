# Spec file for outPlayer on Fedora / RHEL / CentOS Stream

%global debug_package %{nil}

Name:           outplayer
Version:        0.1.1
Release:        1%{?dist}
Summary:        Modern, high-performance Linux media player

License:        GPL-3.0-or-later
URL:            https://github.com/hmdamm/outPlayer
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake >= 3.22
BuildRequires:  ninja-build
BuildRequires:  gcc-c++
BuildRequires:  pkgconfig
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtsvg-devel
BuildRequires:  mpv-libs-devel >= 0.35.0

Requires:       qt6-qtbase
Requires:       qt6-qtsvg
Requires:       mpv-libs >= 0.35.0
Requires:       hicolor-icon-theme

# Fallback definitions for build environments lacking cmake-rpm-macros
%{!?cmake: %global cmake cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=%{_prefix} -DBUILD_TESTING=OFF}
%{!?cmake_build: %global cmake_build cmake --build build}
%{!?cmake_install: %global cmake_install DESTDIR=%{buildroot} cmake --install build}

%description
outPlayer is a modern, high-performance desktop media player for Linux
built with Qt 6 and libmpv. It provides hardware-accelerated video
playback, customizable hotkeys, an advanced playlist engine, and seamless
desktop environment integration.

%prep
%autosetup

%build
%cmake
%cmake_build

%install
%cmake_install

%files
%{_bindir}/outplayer
%{_datadir}/applications/org.outplayer.outPlayer.desktop
%{_datadir}/metainfo/org.outplayer.outPlayer.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/org.outplayer.outPlayer.*

%changelog
* Thu Nov 26 2026 outPlayer Developers <dev@outplayer.org> - 0.1.1-1
- Production stability release fixing libmpv EOF/equalizer/aspect handling, MPRIS2 toggles, shortcuts, and Preferences sync

* Fri Nov 13 2026 outPlayer Developers <dev@outplayer.org> - 0.1.0-1
- Initial release of outPlayer 0.1.0
