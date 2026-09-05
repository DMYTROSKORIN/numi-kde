<div align="center">
  <img src="logo/numi-kde-readme.png" width="160" alt="Numi-KDE" />
  <h1>Numi-KDE</h1>
  <p><strong>Document-style calculator for KDE Plasma.<br>Type like notes — get live results.</strong></p>

  [![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
  [![Release](https://img.shields.io/github/v/release/DMYTROSKORIN/numi-kde?color=brightgreen)](https://github.com/DMYTROSKORIN/numi-kde/releases/latest)
  [![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20KDE%20Plasma%206-5294e2?logo=kde&logoColor=white)](https://kde.org)
  [![Built with Qt 6](https://img.shields.io/badge/Qt-6-41CD52?logo=qt&logoColor=white)](https://qt.io)
  [![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)](https://isocpp.org)
</div>

---

![Numi-KDE — inline /help screen](docs/screenshot.png)

## What is this?

Most calculators force you into buttons, modes, history panels, and copy-paste loops. `numi-kde` treats your calculations as a small live document:

```
Rent      = 1200
Internet  = 45
Coffee    = 4.50 * 18
Total     = Rent + Internet + Coffee

500 AED to USD
today - 01.01.2000
```

You write. It calculates inline. Results appear next to each line as you type.

Inspired by [Numi](https://numi.app) on macOS — built from scratch for KDE Plasma 6 on Linux using C++, Qt 6, QML and `libqalculate`.

---

## Install

### Fedora — one command

```sh
curl -fsSL https://raw.githubusercontent.com/DMYTROSKORIN/numi-kde/main/packaging/install.sh | bash
```

The installer downloads the `.rpm` from GitHub Releases, verifies `SHA256SUMS` and the GPG signature (project key `7022A791 58931F41 31646599 411C68B8 56CEC16E`, shipped as `RPM-GPG-KEY-numi-kde`), imports the key, installs via `dnf`, and runs a smoke test.

<details>
<summary>More install options</summary>

**Install a specific version** (0.1.86 or newer — earlier releases are unsigned):
```sh
curl -fsSL https://raw.githubusercontent.com/DMYTROSKORIN/numi-kde/main/packaging/install.sh | NUMI_KDE_VERSION=v0.1.87 bash
```

**Dry run (no changes made):**
```sh
curl -fsSL https://raw.githubusercontent.com/DMYTROSKORIN/numi-kde/main/packaging/install.sh | bash -s -- --dry-run
```

**Uninstall** (add `--purge` to also remove settings, history, the autostart entry and the imported key):
```sh
curl -fsSL https://raw.githubusercontent.com/DMYTROSKORIN/numi-kde/main/packaging/uninstall.sh | bash
```
</details>

### Updates

The app checks GitHub Releases once a day. A new version is downloaded and verified by a small privileged helper — checksum, package name, GPG signature and signer key must all match, downgrades are refused — and installed without asking. If the window is hidden the app restarts right away; if you are working in it, it restarts the next time you hide it and shows a notification with a *Restart now* button. Turn automatic installation off in Settings if you prefer to update by hand.

### Build from Source

For Arch, Ubuntu, Debian and other distributions — build and install from source. The build produces two binaries: `numi-kde` (the app) and `numi-kde-engine` (the calculation engine, installed to `libexec`).

<details>
<summary>Fedora</summary>

```sh
sudo dnf install -y \
  cmake ninja-build gcc-c++ libqalculate-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kwindowsystem-devel kf6-kglobalaccel-devel \
  kf6-kconfig-devel kf6-kdbusaddons-devel kf6-knotifications-devel
# runtime: kf6-qqc2-desktop-style kf6-kdeclarative curl dnf polkit
```
</details>

<details>
<summary>Arch Linux</summary>

```sh
sudo pacman -S cmake ninja gcc qt6-base qt6-declarative libqalculate \
  kwindowsystem kglobalaccel kconfig kdbusaddons knotifications \
  qqc2-desktop-style kdeclarative curl
```
</details>

<details>
<summary>Ubuntu 24.04 / Debian Trixie</summary>

```sh
sudo apt install cmake ninja-build g++ \
  qt6-base-dev qt6-declarative-dev libqalculate-dev \
  libkf6windowsystem-dev libkf6globalaccel-dev \
  libkf6config-dev libkf6dbusaddons-dev libkf6notifications-dev \
  qml6-module-org-kde-desktop qml6-module-org-kde-kquickcontrols curl
```

> Ubuntu 22.04 LTS: KF6 packages are not available in standard repositories. Use Ubuntu 24.04+, a PPA, or build KF6 from source.
> The self-update helper uses `dnf`; on non-Fedora systems update from source instead.
</details>

**Clone, build, test and install:**

> Requires **CMake ≥ 3.24** and Qt ≥ 6.6.

```sh
git clone https://github.com/DMYTROSKORIN/numi-kde.git
cd numi-kde
cmake -S kde -B build -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build build -j$(nproc)
dbus-run-session -- ctest --test-dir build --output-on-failure   # optional
sudo cmake --install build
```

**Build and package as RPM (Fedora):**

```sh
cmake -S kde -B kde/build-release -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build kde/build-release -j$(nproc)
cmake --build kde/build-release --target package
# RPM: kde/build-release/numi-kde-<version>-x86_64.rpm (unsigned — CI signs the official ones)
sudo dnf install -y kde/build-release/numi-kde-*.rpm
```

---

## Features

| | |
|---|---|
| **Document mode** | One expression per line, results aligned in a live column, automatic total when the lines are compatible |
| **Math** | Arithmetic, variables, functions — `sqrt`, `sin`, `log`, `sum(x^2, 1, 10, x)` and everything else libqalculate knows |
| **Unit conversion** | Length, weight, speed, area, data, time — natural syntax like `10 m to ft`, `60 km/h to m/s`, `1 GiB to MB` |
| **Currency** | Live fiat rates via Frankfurter · top-50 crypto via CoinGecko · mixed arithmetic like `600 AED + 400 USD` · configurable default currency |
| **Date math** | `today/now/tomorrow/yesterday ± N days/weeks/months/years` · `01.01.2000 + 25 years` · `15.05.26 + 2 weeks` · spans like `today - 01.01.2000` |
| **Time arithmetic** | `time + 60 min` · `2 hours + 30 minutes` · `90 minutes to hours` |
| **Percentages** | `20% of 500` · `500 + 10%` · `500 - 10%` |
| **Variables** | `A = 800 - 200` · `B := A * 2` · multi-word names: `Monthly income = 5000` |
| **Numbers** | Grouped digits, configurable decimals (0–10), half-away-from-zero rounding, scientific notation for very large and very small values, complex results like `sqrt(-1)` → `i` |
| **Autocomplete** | Tab offers your variables, keywords, units, currencies and functions with descriptions |
| **Explained errors** | Hover a red *Error* to see why: division by zero, below absolute zero, vectors, timeouts, libqalculate's own message |
| **Inline help** | Type `/help` or press `?` — rendered inside the editor with full syntax highlighting |
| **History** | Drawer with the last 25 documents, restore with one tap |
| **KDE integration** | Tray icon (light and dark schemes) · global shortcut · single instance · launch at login · keep above · remembers its position on Wayland and X11 · KDE notifications |
| **Robust engine** | Calculations run in a separate process that is restarted automatically if a formula crashes or stalls it — the app never freezes |

---

## Keyboard

| Shortcut | Action |
|---|---|
| `Ctrl+Alt+1` | Toggle the window globally (configurable in Settings with the standard KDE shortcut recorder) |
| `Esc` | Hide the window (can be turned off in Settings) |
| `Ctrl+N` | Clear the current document |
| `Tab` | Autocomplete variables, keywords, units, currencies and functions; `Tab`/`↑`/`↓` cycle, `Esc` reverts |
| Click a result | Copy it to the clipboard |

---

## Command-line Flags

| Flag | Description |
|---|---|
| `numi-kde` | Launch with the main window visible (or bring the running instance's window up) |
| `numi-kde --hidden` | Start silently in the system tray — used by the autostart entry |
| `numi-kde --version` | Print the version string and exit |
| `numi-kde --probe` | Self-test through the real engine process (`2+2`, `1 km to m`); exit code `0` on success |
| `numi-kde --help` | Print usage and exit |

---

## Why

I use KDE on Linux daily and missed having a Numi-style calculator — the kind that sits in a small floating window, stays out of the way, and understands expressions written like plain notes. The macOS original is excellent; Linux had nothing close with proper KDE integration.

`numi-kde` is not a port. It is a clean-room implementation in C++ / Qt 6 / QML, backed by `libqalculate`, designed to feel fully native on KDE Plasma.

---

## Project

| | |
|---|---|
| **Language** | C++20 |
| **UI** | Qt 6 · QML · KDE Frameworks 6 |
| **Engine** | libqalculate in a separate, self-healing `numi-kde-engine` process |
| **Packaging** | GPG-signed `.rpm` for Fedora via GitHub Actions; in-app updates verify the signature |
| **Quality** | Nine test suites incl. golden documents, engine stall/crash recovery, KWin script contract, ThreadSanitizer job, RPM contents check |
| **Docs** | [Architecture](docs/architecture.md) · [Release process](docs/release.md) · [Changelog](CHANGELOG.md) |

---

## Contributing

Fork it, improve it, package it, break it, fix it. All welcome.

Good places to start:
- Calculator compatibility and edge cases — add a golden document in `kde/tests/fixtures/golden/`
- KDE Wayland and X11 testing
- Packaging for other distributions
- UI polish and accessibility
- Documentation and examples

Every change goes through a pull request; see [docs/release.md](docs/release.md) for the workflow and test commands.

---

## Acknowledgements

`numi-kde` exists because of excellent prior work:

- **[Numi](https://numi.app)** by Nikolaeu — the document-calculator idea and the UX that made me want this on Linux. The core concept is directly inspired by his work (used under the MIT License).
- **[libqalculate](https://qalculate.github.io)** — the powerful calculation engine that handles the hard parts.
- **[KDE](https://kde.org) and KWin** — the desktop platform, integration APIs, and the community that makes Linux worth using.
- **[Qt / QML](https://qt.io)** — the native UI framework.
- **[Frankfurter](https://www.frankfurter.app)** — open fiat exchange rate API.
- **[CoinGecko](https://www.coingecko.com)** — public crypto market data.
- The broader open-source Linux desktop community — for the libraries and tooling that make independent projects like this possible.

---

## License

[![Apache 2.0](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)

Released under the [Apache License 2.0](LICENSE).  
See [NOTICE](NOTICE) for third-party attribution.

---

<sub>Contact: <a href="mailto:dev@skorin.online">dev@skorin.online</a></sub>
