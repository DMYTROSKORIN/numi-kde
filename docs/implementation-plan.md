# numi-kde Implementation Plan

Last updated: 2026-05-02.

## Goal

`numi-kde` is a KDE-native document calculator:

- text document on the left, aligned results on the right;
- instant recalculation while typing;
- compact scratchpad window;
- native KDE integration: tray, global shortcut, clipboard, autostart and window behavior;
- strong math / unit / currency / date support through `libqalculate`;
- only native KDE application code, resources, tests and current documentation in the repository.

## What Is Done

- Qt/QML frameless compact window with persistent geometry.
- C++ backend through `DocumentModel`.
- `libqalculate` bridge in `QalcBridge`.
- `--version` and `--probe` CLI modes for package smoke tests.
- CMake install rules for binary, desktop file, AppStream metadata, app icon, LICENSE and NOTICE.
- Apache License 2.0 project licensing.
- C++ syntax highlighter with semantic token classification.
- Settings window.
- History drawer with persisted sessions.
- Result copy and Total copy.
- Total sums only compatible numeric values; hidden for mixed units/currencies.
- Readable inline `/help` guide using shared syntax highlighting.
- Adaptive result column with separator clamped to visible content.
- Date differences: `today - 01.01.2000`, `01.01.2000 - 02.05.2026`.
- Date arithmetic: `01.01.2000 + 25 years`.
- Target-prefixed conversion output: `1 BTC to EUR` → `EUR <value>`.
- Percentage: `20% of 300`, `20% from 300`; currency-preserving: `21.75% from 3654 AED` → `AED 794.745`.
- Comments: lines starting with `#` are colored and ignored by the calculator.
- App/tray transparent PNG resources.
- Tray menu with show/hide and quit.
- Window hides on close (X button, Alt+F4); quit only from tray.
- Skip taskbar and skip pager — app stays out of the panel task manager.
- Global shortcut via `KGlobalAccel`, default `Ctrl+Alt+1`.
- Configurable global shortcut with KDE conflict handling.
- `Ctrl+N` clears the current document.
- `Tab` completion for units, functions and variables.
- Launch-at-login.
- X11 keep-above through `KX11Extras` and `NET::KeepAbove`.
- KDE Wayland keep-above via managed KWin Window Rule.
- CPack packaging: `.rpm` for Fedora, `.deb` scaffold for Debian/Ubuntu.
- `packaging/install.sh` — thin bootstrapper for GitHub Releases.
- `packaging/uninstall.sh` — package manager removal.
- GitHub Actions release workflow on `v*` tags.
- Native tests in `kde/tests/qalc_test.cpp`.

## Roadmap

### Calculator

- Improve diagnostic messages beyond the compact `Error` label.
- Broaden date/time format compatibility.
- Offline / failed-rate UI state for crypto.

### KDE Integration

- Validate KWin keep-above rule on a real KDE Wayland session.
- Add installed desktop-file smoke test.

### Packaging and Release

- Validate DEB dependencies on Ubuntu/Kubuntu and Debian.
- Test `install.sh` end-to-end against published GitHub Release artifacts.
- Validate one-line install on a clean Fedora KDE system.
- Validate one-line install on a clean Kubuntu LTS system.
- Publish repository after installer validation passes.

### UX

- Visual screenshot checks for KDE Wayland, X11, HiDPI and fractional scaling.
- Decide whether result click should copy immediately or show copy feedback.
