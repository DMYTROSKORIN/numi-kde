# Installer Handoff

Last updated: 2026-05-02.

This file is the handoff for the next implementation pass focused on making `numi-kde` installable with one terminal command.

## Current Repository State

- Repository path: `/home/skorin/Git/numi-kde`.
- GitHub repository: `DMYTROSKORIN/numi-kde`.
- GitHub visibility at the time of this handoff: private.
- Current latest pushed tag: `v0.1.6`.
- License: Apache License 2.0.
- Copyright notice: `NOTICE` contains `Copyright 2026 DMYTRO SKORIN`.
- Primary app source: `kde/`.
- Build directory convention used so far: `build/kde`.
- The repository should remain native-only. Do not reintroduce old JS/web prototype files.

## Product Goal

The final installation experience should be:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash
```

Uninstall should be:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/uninstall.sh | bash
```

The repository may become public only after release packaging and installer validation are complete.

## Core Installation Strategy

Do not make `install.sh` compile source on the user's machine.

The installer should be a thin bootstrapper:

1. Detect distro and architecture.
2. Download `SHA256SUMS` from GitHub Releases.
3. Download the matching native package.
4. Verify checksum.
5. Install through the system package manager.
6. Run `numi-kde --probe`.

Native packages are the real installation unit:

- Fedora: `.rpm`
- Ubuntu/Debian: `.deb`

Do not use `/opt` tarball installation as the primary path.

## Already Implemented

The app is already packageable at a basic level:

- `numi-kde --version` exists.
- `numi-kde --probe` exists.
- `cmake --install` installs:
  - binary;
  - desktop file;
  - AppStream metadata;
  - app icon.
- Desktop launcher icon uses `org.skorin.numi-kde`.
- AppStream project license is `Apache-2.0`.
- `LICENSE` and `NOTICE` exist in the repository.

Known good local checks from the previous pass:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde --version
./build/kde/numi-kde --probe
cmake --install build/kde --prefix /tmp/numi-kde-install-stage-codex
/tmp/numi-kde-install-stage-codex/bin/numi-kde --version
/tmp/numi-kde-install-stage-codex/bin/numi-kde --probe
```

Expected CLI output currently:

```text
numi-kde 0.1.6
numi-kde 0.1.6 probe ok
```

## Immediate Next Step

Implement Phase 2: local package generation.

Start with Fedora/RPM because the current development machine is Fedora KDE and the runtime dependencies are easiest to validate there.

After RPM works locally, add DEB generation and validate on Ubuntu/Debian via VM, container, or GitHub Actions runner.

## Phase 2A: Complete CMake Install Rules

Add install rules for project legal files:

```cmake
install(FILES ../LICENSE ../NOTICE
  DESTINATION ${CMAKE_INSTALL_DATADIR}/doc/numi-kde
)
```

Use `../LICENSE` because the CMake source directory is `kde/` when configured with:

```sh
cmake -S kde -B build/kde
```

Then verify:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
cmake --install build/kde --prefix /tmp/numi-kde-install-stage-codex
find /tmp/numi-kde-install-stage-codex -type f
```

Expected installed files should include:

```text
bin/numi-kde
share/applications/org.skorin.numi-kde.desktop
share/metainfo/org.skorin.numi-kde.metainfo.xml
share/icons/hicolor/256x256/apps/org.skorin.numi-kde.png
share/doc/numi-kde/LICENSE
share/doc/numi-kde/NOTICE
```

## Phase 2B: Add CPack

Add CPack configuration to `kde/CMakeLists.txt` after install rules.

Recommended first pass:

```cmake
set(CPACK_PACKAGE_NAME "numi-kde")
set(CPACK_PACKAGE_VENDOR "DMYTRO SKORIN")
set(CPACK_PACKAGE_CONTACT "DMYTROSKORIN@users.noreply.github.com")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "KDE-native document calculator")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/DMYTROSKORIN/numi-kde")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/../LICENSE")
set(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}-${PROJECT_VERSION}-${CMAKE_SYSTEM_PROCESSOR}")

set(CPACK_GENERATOR "RPM;DEB")

set(CPACK_RPM_PACKAGE_LICENSE "Apache-2.0")
set(CPACK_RPM_PACKAGE_GROUP "Applications/Productivity")
set(CPACK_RPM_PACKAGE_REQUIRES "libqalculate, qt6-qtbase, qt6-qtdeclarative, kf6-kwindowsystem")

set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "DMYTRO SKORIN <DMYTROSKORIN@users.noreply.github.com>")

include(CPack)
```

Important: dependency names above are a starting point, not final truth. Verify package names on clean Fedora, Ubuntu and Debian systems.

`kf6-kglobalaccel` is optional at build time in the current project. Decide whether package runtime should require it or whether the app should degrade cleanly without it.

## Phase 2C: Build Local RPM

Use a fresh release build directory:

```sh
cmake -S kde -B build/kde-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/kde-release --target numi-kde numi-kde-tests
ctest --test-dir build/kde-release --output-on-failure
cmake --build build/kde-release --target package
```

Expected output should include an RPM in `build/kde-release/`.

Inspect it:

```sh
rpm -qpi build/kde-release/*.rpm
rpm -qpl build/kde-release/*.rpm
```

The package contents must include:

```text
/usr/bin/numi-kde
/usr/share/applications/org.skorin.numi-kde.desktop
/usr/share/metainfo/org.skorin.numi-kde.metainfo.xml
/usr/share/icons/hicolor/256x256/apps/org.skorin.numi-kde.png
/usr/share/doc/numi-kde/LICENSE
/usr/share/doc/numi-kde/NOTICE
```

## Phase 2D: Test RPM Install

Installing/removing an RPM changes the host system. Ask the user before doing this on the host.

Preferred host test:

```sh
sudo dnf install ./build/kde-release/*.rpm
numi-kde --version
numi-kde --probe
numi-kde
sudo dnf remove numi-kde
```

If avoiding host changes, use a Fedora container/VM for package install smoke tests.

Minimum package install validation:

- package installs without unresolved dependencies;
- `numi-kde --version` prints the package version;
- `numi-kde --probe` exits `0`;
- desktop file exists;
- app icon path exists;
- uninstall removes package files.

## Phase 2E: Add DEB Generation

Do this after RPM works.

Build `.deb` in Ubuntu/Kubuntu or Debian environment, not by guessing dependency names on Fedora.

Likely dependency areas:

- Qt 6 base runtime;
- Qt 6 QML/Quick runtime;
- Qt Quick Controls 2 runtime;
- KF6 WindowSystem;
- KF6 GlobalAccel if required;
- libqalculate runtime.

Use `apt-cache search`, `apt-cache show`, `dpkg -S`, or CI runner package metadata to confirm exact package names.

Then test:

```sh
sudo apt install ./numi-kde_*.deb
numi-kde --version
numi-kde --probe
numi-kde
sudo apt remove numi-kde
```

## Phase 3: Installer Scripts

Only start this after at least one native package installs and probes correctly.

Add:

```text
packaging/install.sh
packaging/uninstall.sh
```

`install.sh` requirements:

- `set -euo pipefail`
- clear, short output;
- `--dry-run`;
- `--help`;
- distro detection through `/etc/os-release`;
- architecture detection through `uname -m`;
- supported package managers:
  - Fedora: `dnf`
  - Ubuntu/Debian: `apt-get`
- latest release download by default;
- pinned version through `NUMI_KDE_VERSION`;
- download package and `SHA256SUMS`;
- verify checksum before `sudo`;
- install with package manager;
- run `numi-kde --probe`;
- fail cleanly on unsupported distro/arch.

`uninstall.sh` requirements:

- detect package manager;
- remove `numi-kde`;
- do not delete user settings/history by default;
- optional future `--purge` mode only after explicit confirmation.

## Phase 4: GitHub Releases

After local package tests:

1. Add GitHub Actions release workflow.
2. Trigger on `v*` tags.
3. Build/test RPM on Fedora.
4. Build/test DEB on Ubuntu.
5. Generate `SHA256SUMS`.
6. Upload:
   - RPM;
   - DEB;
   - `install.sh`;
   - `uninstall.sh`;
   - `SHA256SUMS`.

Do not put the install command in the top README as a finished feature until release assets actually exist and were tested.

## Validation Before Public Repository

Before making the repository public:

- Fedora KDE clean install: one-line install, app launch, uninstall.
- Kubuntu LTS clean install: one-line install, app launch, uninstall.
- Debian KDE clean install if supported in first public release.
- Verify `Ctrl+Alt+1` global shortcut does not crash.
- Verify tray icon appears.
- Verify basic calculator flows:
  - `2 + 2`
  - `1 km to m`
  - `500 AED to USD`
  - `today - 01.01.2000`
  - `/help`

## Current Risks

- Runtime dependency names are not finalized for Ubuntu/Debian.
- AppStream validation has not been run yet.
- Desktop file validation has not been run yet.
- `--probe` constructs `QalcBridge`, whose constructor starts a crypto-rate network request. It currently exits cleanly because no event loop is started for the request, but the next model may consider adding a no-network probe path later.
- KDE Wayland keep-above behavior still needs manual validation on a real session.
- Repository is private, so GitHub Release install URLs will not work for public users until visibility changes.

## Validation Commands To Run After Any Packaging Change

At minimum:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde --version
./build/kde/numi-kde --probe
cmake --install build/kde --prefix /tmp/numi-kde-install-stage-codex
/tmp/numi-kde-install-stage-codex/bin/numi-kde --version
/tmp/numi-kde-install-stage-codex/bin/numi-kde --probe
git diff --check
```

If C++ files change, also run semgrep on the changed files:

```sh
semgrep --config=auto --json <changed-cpp-or-header-files>
```

If package files are generated, inspect contents:

```sh
rpm -qpl build/kde-release/*.rpm
dpkg-deb -c build/kde-release/*.deb
```

## What Not To Do

- Do not publish the one-line install command as working before release artifacts exist.
- Do not make the installer compile from source by default.
- Do not hardcode `skorin.online` as canonical distribution unless GitHub Releases are proven insufficient.
- Do not remove Apache License 2.0 metadata.
- Do not delete user settings during uninstall unless a future `--purge` flow explicitly asks.
- Do not make the repository public until package installation is tested.
