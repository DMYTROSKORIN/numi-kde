# One-Line Installer Plan

Last updated: 2026-05-02.

## Goal

Provide a single terminal command that installs a working `numi-kde` build on KDE-based Ubuntu, Fedora and Debian systems:

```sh
curl -fsSL https://numi-kde.example/install.sh | bash
```

The script must detect the system, install required runtime dependencies when possible, download the correct release artifact, install desktop integration, and verify that the app can start.

## Supported Scope

Initial support target:

- Fedora KDE, current stable releases.
- Ubuntu KDE/Kubuntu LTS and current stable releases.
- Debian KDE stable and testing where required packages are available.
- `x86_64` first; `aarch64` can be added once release artifacts exist.

Unsupported systems should fail with a clear message and no partial install.

## Release Artifact Strategy

Preferred release layout:

- Build reproducible release artifacts in CI.
- Publish artifacts on GitHub Releases.
- Ship one portable archive per distro family when ABI/package names differ.
- Include:
  - `numi-kde` binary;
  - QML/resource files if not fully embedded;
  - `.desktop` file;
  - icon files if needed outside Qt resources;
  - `manifest.json` with version, commit, target distro family, architecture and checksums.

The installer should not compile from source on the user's machine in the normal path. Source builds are too slow, need too many dev packages, and fail more often on end-user systems.

## Installer Flow

1. Start in strict shell mode and create a temporary directory.
2. Detect OS from `/etc/os-release`.
3. Detect architecture through `uname -m`.
4. Check that the session is KDE or KDE packages are present.
5. Choose the package manager:
   - Fedora: `dnf`;
   - Ubuntu/Debian: `apt-get`.
6. Install runtime dependencies only:
   - Qt 6 runtime/QML modules;
   - KF6 window/global shortcut runtime libraries where packaged separately;
   - `libqalculate`;
   - `curl`, `tar`, `desktop-file-utils` when missing.
7. Resolve latest stable GitHub Release unless `NUMI_KDE_VERSION` is set.
8. Download artifact and checksum file.
9. Verify checksum before installing.
10. Install under `/opt/numi-kde`.
11. Install wrapper command at `/usr/local/bin/numi-kde`.
12. Install desktop file under `/usr/local/share/applications`.
13. Install icons under `/usr/local/share/icons/hicolor` if external icons are shipped.
14. Run `update-desktop-database` when available.
15. Smoke-test `numi-kde --version` or a dedicated non-GUI probe command.
16. Print the installed version and launch instructions.

## Safety Requirements

- Never pipe downloaded artifacts directly into privileged commands.
- Ask for `sudo` only when installation actually needs system writes.
- Use `sudo -v` once, then keep privileged operations narrow.
- Verify checksums before copying files into final locations.
- Make the install idempotent: repeated runs update in place.
- Keep previous installation until the new artifact is verified.
- On failure, remove temporary files and leave the previous installation working.

## Command Variants

Stable latest:

```sh
curl -fsSL https://numi-kde.example/install.sh | bash
```

Pinned version:

```sh
curl -fsSL https://numi-kde.example/install.sh | NUMI_KDE_VERSION=v0.1.4 bash
```

Uninstall:

```sh
curl -fsSL https://numi-kde.example/install.sh | bash -s -- --uninstall
```

Dry run:

```sh
curl -fsSL https://numi-kde.example/install.sh | bash -s -- --dry-run
```

## Implementation Phases

### Phase 1: Prepare App Artifacts

- Add install rules to CMake for binary, desktop file and icons.
- Add `--version` or `--probe` command-line mode for non-GUI installer verification.
- Decide whether QML is fully embedded or shipped beside the binary.
- Create release archive layout and manifest.

### Phase 2: CI Release Build

- Add GitHub Actions builds for Fedora and Ubuntu.
- Add Debian build once dependency names are confirmed.
- Upload release archives and checksum files.
- Run native tests before artifact upload.

### Phase 3: Installer Script

- Add `packaging/install.sh`.
- Implement distro/architecture detection.
- Implement package-manager dependency installation.
- Implement artifact download, checksum verification, install, update and uninstall.
- Keep all user-facing messages short and specific.

### Phase 4: Validation Matrix

Test fresh KDE installs:

- Fedora KDE stable.
- Kubuntu LTS.
- Ubuntu with KDE Plasma installed.
- Debian KDE stable.
- Debian testing KDE.

Validation steps:

- one-line install completes;
- app appears in launcher;
- `numi-kde` command starts the app;
- tray icon loads;
- global shortcut registration does not crash;
- `libqalculate` conversions work;
- uninstall removes installed files.

### Phase 5: Publish

- Host `install.sh` at a stable HTTPS URL.
- Link the command from `README.md`.
- Document supported distros, architecture, uninstall and troubleshooting.
- Keep old release artifacts available for rollback.

## Open Decisions

- Final hosting URL for `install.sh`.
- Whether to ship distro-specific archives or a single AppImage/portable bundle.
- Whether to depend on system Qt/KF6 packages or bundle more libraries.
- Whether first public installer should install latest stable only or support release channels.
