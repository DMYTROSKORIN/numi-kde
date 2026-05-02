# One-Line Installer Plan

Last updated: 2026-05-02.

## Goal

Provide a one-command installation path for KDE-based Ubuntu, Fedora and Debian systems, with GitHub Releases as the primary distribution channel.

Detailed implementation handoff for the next packaging pass lives in `docs/installer-handoff.md`.

Recommended public command:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash
```

Recommended uninstall command:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/uninstall.sh | bash
```

Pinned version command:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/download/v0.1.4/install.sh | bash
```

`skorin.online` can be used later as a short redirect, but GitHub should remain the canonical source for scripts, packages, checksums and release history.

## Core Decision

Use native Linux packages as the real installation unit:

- Ubuntu/Debian: `.deb`
- Fedora: `.rpm`

Do not use a source build or `/opt` tarball as the primary path. The installer should be a thin bootstrapper that downloads a verified release package and hands installation to the system package manager.

This gives:

- normal dependency resolution;
- normal uninstall behavior;
- standard desktop/icon installation;
- fewer user-machine build failures;
- cleaner security story than compiling arbitrary code during install.

## Supported Scope

Initial support target:

- Fedora KDE, current stable releases.
- Ubuntu KDE/Kubuntu LTS and current stable releases.
- Debian KDE stable and testing where required packages are available.
- `x86_64` first.
- `aarch64` only after release artifacts and test coverage exist.

Unsupported systems should fail clearly and leave no partial installation behind.

## GitHub Release Layout

Each release should publish:

```text
install.sh
uninstall.sh
SHA256SUMS
numi-kde_<version>_amd64.deb
numi-kde-<version>.x86_64.rpm
```

Optional later artifacts:

```text
numi-kde_<version>_arm64.deb
numi-kde-<version>.aarch64.rpm
manifest.json
```

`SHA256SUMS` must include every downloadable package and script. Release notes should include the commit, supported distros and known limitations.

## Installer Flow

`install.sh` should:

1. Start with strict shell settings.
2. Create and clean up a temporary working directory.
3. Detect OS through `/etc/os-release`.
4. Detect architecture through `uname -m`.
5. Verify that the package manager is supported:
   - Fedora: `dnf`
   - Ubuntu/Debian: `apt-get`
6. Optionally warn when KDE/Plasma is not detected.
7. Resolve the GitHub Release URL:
   - latest stable by default;
   - pinned release when `NUMI_KDE_VERSION` is set or the user downloads a versioned script URL.
8. Download `SHA256SUMS`.
9. Download the matching `.deb` or `.rpm`.
10. Verify the package checksum before installing.
11. Ask for `sudo` only when installation is about to happen.
12. Install package:
   - Debian/Ubuntu: `sudo apt-get install ./numi-kde_<version>_amd64.deb`
   - Fedora: `sudo dnf install ./numi-kde-<version>.x86_64.rpm`
13. Run a non-GUI smoke test:
   - preferred: `numi-kde --probe`
   - acceptable: `numi-kde --version`
14. Print installed version and launch instructions.

The installer must not compile source code on the user's machine in the normal path.

## Uninstaller Flow

`uninstall.sh` should:

1. Detect OS and package manager.
2. Check whether `numi-kde` is installed.
3. Remove through the package manager:
   - Debian/Ubuntu: `sudo apt-get remove numi-kde`
   - Fedora: `sudo dnf remove numi-kde`
4. Leave user settings/history intact by default.
5. Support a future `--purge` mode for removing user config after explicit confirmation.

## Command Variants

Latest stable:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash
```

Pinned version:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/download/v0.1.4/install.sh | bash
```

Pinned version through environment:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | NUMI_KDE_VERSION=v0.1.4 bash
```

Uninstall:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/uninstall.sh | bash
```

Dry run:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash -s -- --dry-run
```

## Safety Requirements

- Never pipe package artifacts into privileged commands.
- Never run downloaded package content before checksum verification.
- Use `sudo -v` only after detection and download verification.
- Keep privileged operations narrow and visible.
- Print exactly which package will be installed.
- Make repeated installs safe and update in place.
- Fail early on unsupported distros, unsupported architectures and missing package managers.
- On failure, remove temporary files and do not damage an existing installation.

## Required App Changes

Before the installer is real, the app needs:

1. `--version` command-line mode. Done in `0.1.5`.
2. `--probe` command-line mode that exits without opening the GUI and verifies basic runtime availability. Done in `0.1.5`.
3. CMake install rules. Initial rules are done in `0.1.5` for:
   - binary;
   - `.desktop` file;
   - app icon;
   - AppStream metadata.
4. Package metadata:
   - name: `numi-kde`;
   - version from CMake project version;
   - license: `Apache-2.0`;
   - maintainer;
   - dependencies.

## Packaging Strategy

Use CPack first unless it becomes too limiting.

Expected CPack outputs:

- `DEB` generator for Ubuntu/Debian.
- `RPM` generator for Fedora.

Runtime dependencies must be explicit and tested on clean systems. Package names will likely differ between Fedora and Debian-family distros, especially for Qt/KF6 runtime modules.

## GitHub Actions Plan

Release CI should:

1. Trigger on version tags such as `v0.1.5`.
2. Build and test on Fedora container/runner for RPM.
3. Build and test on Ubuntu runner/container for DEB.
4. Add Debian validation once package dependencies are confirmed.
5. Run:
   - CMake configure;
   - native build;
   - native CTest;
   - package build;
   - package install smoke test where possible.
6. Generate `SHA256SUMS`.
7. Upload packages, scripts and checksums to GitHub Releases.

## Validation Matrix

Fresh KDE installs to test before documenting the command publicly:

- Fedora KDE stable.
- Kubuntu LTS.
- Ubuntu with KDE Plasma installed.
- Debian KDE stable.
- Debian testing KDE.

Validation steps:

- one-line install completes;
- package manager shows `numi-kde` installed;
- app appears in launcher;
- `numi-kde` command starts the app;
- tray icon loads;
- global shortcut registration does not crash;
- `libqalculate` math/unit/currency flows work;
- uninstall removes package files;
- reinstall works after uninstall;
- user settings survive normal uninstall.

## Implementation Phases

### Phase 1: Packageable App

Status: mostly complete.

Done:

- Add `--version`.
- Add `--probe`.
- Add CMake install rules for binary, desktop file, AppStream metadata and app icon.
- Confirm installed binary starts outside the build tree.

Remaining:

- Re-check install layout after CPack package generation.

### Phase 2: Local Packages

Detailed next steps are documented in `docs/installer-handoff.md`.

- Add CPack configuration.
- Build `.deb` locally or in Ubuntu container.
- Build `.rpm` locally or in Fedora container.
- Install and uninstall packages manually on test machines.

### Phase 3: Installer Scripts

- Add `packaging/install.sh`.
- Add `packaging/uninstall.sh`.
- Implement distro and architecture detection.
- Implement GitHub Release asset download.
- Implement checksum verification.
- Implement package-manager install/remove.
- Add `--dry-run`.

### Phase 4: CI Releases

- Add GitHub Actions release workflow.
- Generate packages and checksums on tag push.
- Upload `install.sh`, `uninstall.sh`, packages and `SHA256SUMS`.
- Test installation from the uploaded release assets.

### Phase 5: Public Documentation

- Make the repository public after installer validation.
- Add the one-line install command to `README.md`.
- Document supported distros, uninstall, pinned install and troubleshooting.
- Optionally add `skorin.online` redirects after GitHub flow is stable.

## Open Decisions

- Final public URL style: GitHub-only or GitHub plus `skorin.online` redirect.
- Exact Qt/KF6 runtime dependency names for Ubuntu, Debian and Fedora.
- Whether to support Debian stable immediately or after dependency validation.
- Whether to add AppImage later as a fallback for unsupported distros.
