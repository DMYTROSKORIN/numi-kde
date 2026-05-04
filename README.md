# numi-kde

<p align="center">
  <img src="logo/numi-kde-readme.png" width="172" alt="numi-kde logo" />
</p>

**A KDE-native document calculator for people who think in text.**

Type expressions like notes. Get aligned results instantly. Keep the window small, fast, native, and out of the way.

`numi-kde` is inspired by Numi, built for Linux/KDE with Qt 6, QML, C++ and `libqalculate`.

> [!CAUTION]
> **Early development — expect bugs.** The app is usable but not production-ready. Features may break, change or disappear between releases. Test before relying on it for real work.

## Install

Latest release:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash
```

Uninstall:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/uninstall.sh | bash
```

Install a specific version:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | NUMI_KDE_VERSION=v0.1.6 bash
```

Dry run:

```sh
curl -fsSL https://github.com/DMYTROSKORIN/numi-kde/releases/latest/download/install.sh | bash -s -- --dry-run
```

The installer detects Fedora (or other RPM-based systems), downloads the matching `.rpm` from GitHub Releases, verifies `SHA256SUMS`, installs through `dnf`, and runs `numi-kde --probe`.

### Manual Build (Ubuntu/Debian/Arch)

If you are not on Fedora, you can build and install the application from source:

1. **Install Dependencies**:
   - **Ubuntu/Debian**: `sudo apt install cmake g++ qt6-base-dev qt6-declarative-dev libkf6windowsystem-dev libkf6globalaccel-dev libqalculate-dev`
   - **Arch**: `sudo pacman -S cmake gcc qt6-base qt6-declarative kwindowsystem kglobalaccel libqalculate`

2. **Clone and Build**:
   ```sh
   git clone https://github.com/DMYTROSKORIN/numi-kde.git
   cd numi-kde
   cmake -S kde -B build -DCMAKE_BUILD_TYPE=Release
   cmake --build build -j$(nproc)
   sudo cmake --install build
   ```

## Why

Most calculators force you into buttons, modes, history panels and copy-paste loops. `numi-kde` treats calculation as a small live document:

```text
Rent = 1200
Internet = 45
Coffee = 4.50 * 18
Total = Rent + Internet + Coffee

500 AED to USD
today - 01.01.2000
```

You keep context. The app does the math.

## Features

- Document-style calculations with one result per line.
- Math, variables, percentages, units, dates and time.
- Live fiat conversion backed by Frankfurter rates.
- Live top-crypto conversion backed by CoinGecko rates.
- Target-prefixed conversion output, such as `USD 136.1471` and `EUR 67,059.8291`.
- Inline `/help` rendered inside the editor with the same syntax highlighting as normal input.
- Total footer that sums only compatible values.
- Auto-expanding result column for long output.
- History drawer and session restore.
- KDE tray icon, global shortcut, launch-at-login and keep-above integration.
- Works with KDE Wayland through a managed KWin window rule; uses `NET::KeepAbove` on X11.

## Examples

| Input | Output |
| --- | --- |
| `2 + 2 * 3^2` | `20` |
| `A = 800 - 200` | `600` |
| `10 m to ft` | converted length |
| `500 AED to USD` | `USD <value>` |
| `1 BTC to EUR` | `EUR <value>` |
| `01.01.2000 + 25 years` | `01.01.2025` |
| `today - 01.01.2000` | compact years/months/days span |
| `/help` | inline usage guide |

## Keyboard

- `Ctrl+Alt+1`: toggle the app globally.
- `Ctrl+N`: clear the current document.
- `Tab`: complete units, functions and variables.
- Click a result: copy it.

## Build From Source

Fedora dependencies:

```sh
sudo dnf install -y \
  cmake gcc-c++ libqalculate-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kwindowsystem-devel kf6-kglobalaccel-devel
```

Build and test:

```sh
cmake -S kde -B build/kde
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde --version
./build/kde/numi-kde --probe
./build/kde/numi-kde
```

Package:

```sh
cmake -S kde -B build/kde-release -DCMAKE_BUILD_TYPE=Release
cmake --build build/kde-release --target package
```

## Project Status

- Runtime: C++ / Qt 6 / QML.
- Engine: `libqalculate`.
- Packaging: `.rpm` / `.deb` release flow with GitHub-hosted installer scripts.
- License: Apache License 2.0.

## Documentation

- `docs/architecture.md`: current layer boundaries.
- `docs/release.md`: release checklist, authorship rules, and local semgrep hook setup.
- `CHANGELOG.md`: release notes.
- `LICENSE`: Apache License 2.0.
- `NOTICE`: copyright and attribution notice.

## Thanks

`numi-kde` exists because of other excellent work:

- Numi, for the document-calculator idea and UX inspiration.
- Qalculate/libqalculate, for the calculation engine.
- KDE and KWin, for the desktop platform and integration points.
- Qt and QML, for the native UI framework.
- CoinGecko, for public crypto market data.
- The open-source Linux desktop community, for the libraries and tooling that make projects like this possible.

Thank you to everyone building and maintaining these projects.

## Contributing

Fork it, improve it, package it, test it, break it, fix it.

Useful areas:

- calculator compatibility;
- KDE Wayland and X11 testing;
- Fedora, Ubuntu and Debian packaging;
- installer validation on clean KDE systems;
- UI polish and accessibility;
- documentation and examples.

## Credits

This project is based on the core logic and is inspired by [Numi](https://github.com/nikolaeu/numi) by Nikolas, used under the MIT License.

## License

`numi-kde` is released under the [Apache License 2.0](LICENSE). See [NOTICE](NOTICE) for third-party attribution.

---
**Contact:** [dev@skorin.online](mailto:dev@skorin.online)
