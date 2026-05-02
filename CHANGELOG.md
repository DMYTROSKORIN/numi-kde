# Changelog

## 0.1.7 - 2026-05-02

### Packaging

- Added CPack configuration to `kde/CMakeLists.txt` for `.rpm` (Fedora) and `.deb` (Debian/Ubuntu) generation.
- Added install rules for `LICENSE` and `NOTICE` to `share/doc/numi-kde`.
- Fedora runtime dependencies confirmed from live binary: `libqalculate`, `qt6-qtbase`, `qt6-qtbase-gui`, `qt6-qtdeclarative`, `kf6-kwindowsystem`, `kf6-kglobalaccel`.
- Local RPM built, installed, probed and removed cleanly on Fedora KDE.
- Added `packaging/install.sh`: distro detection via `/etc/os-release`, GitHub Releases download, SHA256 verification, `dnf`/`apt-get` install, `--probe` smoke test, `--dry-run` and `--help` flags, `NUMI_KDE_VERSION` pin.
- Added `packaging/uninstall.sh`: package manager detection, removal, optional `--purge` with explicit confirmation.
- Added `.github/workflows/release.yml`: builds RPM on Fedora container and DEB on Ubuntu runner on `v*` tag push, generates `SHA256SUMS`, publishes GitHub Release.

### Window behavior

- Window position is now remembered correctly across sessions and within a session. Fixed a race condition where the compositor's initial placement overwrote the saved position before `Component.onCompleted` could restore it.
- X button and Alt+F4 now hide the window instead of quitting the application. The app can only be quit from the system tray **Quit Numi-KDE** action.
- The app no longer appears in the KDE panel task manager (`NET::SkipTaskbar | NET::SkipPager` always applied, not only when "Always on top" is enabled).

### Calculator

- `X% from/of N CURRENCY` now preserves the original currency. Previously `21.75% from 3654 AED` returned `USD 217.02` because libqalculate normalises currency results to USD when multiplying by a dimensionless fraction. The pattern is now computed directly.

### Visual

- Comment lines (`# …`) are now colored neon cyan (`#22d3ee`) instead of operator yellow, making them visually distinct.
- Added project logo to README.

## 0.1.6 - 2026-05-02

- Released the project under the Apache License 2.0.
- Updated AppStream metadata from proprietary placeholder licensing to `Apache-2.0`.
- Removed the README logo image and added a clear license section.
- Bumped the native project version to `0.1.6`.

## 0.1.5 - 2026-05-02

- Reworked `README.md` into a public-facing project overview with status, features, build instructions, acknowledgements and contribution notes.
- Refined the one-line installer plan around GitHub Releases, `.deb`/`.rpm` packages, checksums and thin install/uninstall scripts.
- Added `numi-kde --version` and `numi-kde --probe` for package smoke tests.
- Updated CMake install rules to install the binary, desktop file, AppStream metadata and the app icon.
- Changed the desktop launcher to use the installed `org.skorin.numi-kde` icon.

## 0.1.4 - 2026-05-02

- Changed inline `/help` date examples to use `01.01.2000`.
- Kept editor syntax colors stable while selecting text by drawing selection over the shared highlight layer.
- Shortened date-span output to a compact English single-line format without end-date wording or negative spans.
- Added a dedicated one-line installer implementation plan for KDE Ubuntu, Fedora and Debian support.

## 0.1.3 - 2026-05-02

- Routed inline `/help` examples through the shared C++ syntax highlighter so currencies, units and operators are colored consistently with normal input.
- Changed manual crypto conversion results to keep the target prefix, for example `1 BTC to EUR` returns `EUR <value>`.
- Added explicit date-to-date spans such as `26.08.1983 - 02.05.2026`, returning an English calendar span and total day count.
- Added regression tests for shared help highlighting, target-prefixed crypto output and detailed date spans.
- Refreshed docs for the current native-only behavior and verification status.

## 0.1.2 - 2026-05-01

- Matched the result separator default color to the app divider color used by the Total row and settings controls.
- Added operator highlighting inside inline `/help` examples without changing the help layout.
- Changed Total semantics so totals are shown only when every numeric row has the same total key; mixed units/currencies hide Total.
- Removed agent-only documents from the repository.
- Refreshed documentation to describe compatible totals and the cleaned native-only project structure.

## 0.1.1 - 2026-05-01

- Changed `/help` from a panel/card overlay to inline editor help text.
- Made the result separator visibly present while keeping the lag-free drag behavior.
- Changed Total to appear only for compatible numeric rows; mixed units/currencies such as `500 AED to USD` plus `1 BTC to UAH` no longer show a total.
- Added `today` operator highlighting.
- Added date arithmetic for inputs such as `26.08.1983 + 42 years`.
- Added regression tests for the reported help, total, highlighting and date arithmetic cases.

## 0.1.0 - 2026-05-01

Release candidate for the native KDE runtime.

- Reworked `/help` into a readable in-app guide with examples for math, variables, units, currency, crypto, dates, percentages and controls.
- Added adaptive result-column sizing for long results.
- Improved result separator dragging by using page-space pointer coordinates and clamping the minimum width to the rendered result content.
- Added typed Total support for compatible converted unit/currency/crypto rows.
- Added explicit date-span support for inputs such as `today - 26.08.1983`.
- Added native tests for date spans, converted-result totals and numeric conversion metadata.
- Removed non-native files so the repository contains only the native KDE project.
- Removed stale includes from `DocumentModel`.

Verification:

```sh
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
```
