# CLAUDE.md — numi-kde

This file is read by Claude Code and AI agents working on this project.
It describes architecture, conventions, and workflow. Read it fully before making any changes.

---

## Critical Rules

- **Never add `Co-Authored-By`** lines to commits — ever
- **Never change code without owner approval** — propose first, implement after explicit confirmation
- **Release**: tag-push only — see Release Process below. Never `gh release create <file>`
- **Tests must pass** before any commit — run all three suites:
  ```
  ./kde/build/numi-kde-tests qalc
  ./kde/build/numi-kde-tests documentmodel
  ./kde/build/numi-kde-tests editor
  ```

---

## Project Overview

KDE-native document-style calculator for Fedora/KDE Plasma.
The user writes expressions as plain text; results appear live next to each line.

- **GitHub**: https://github.com/DMYTROSKORIN/numi-kde
- **Language**: C++20 + Qt 6 + QML
- **Engine**: libqalculate
- **Packaging**: RPM for Fedora via GitHub Actions CI

---

## Repository Layout

```
kde/
  src/           C++ backend
  qml/           QML frontend
  tests/         Unit tests (qalc_test.cpp)
  resources/     Icons, .desktop, .metainfo.xml
  CMakeLists.txt Build definition (version lives here)
docs/
  architecture.md  Deep-dive architecture doc
  release.md       Release process and contribution rules
packaging/
  install.sh     One-command Fedora installer
  uninstall.sh   Uninstaller
.github/workflows/release.yml  CI/CD — builds RPM on tag push
.githooks/pre-commit           Semgrep static analysis on staged C++ files
.semgrep.yml                   Rules: no system(), popen(), strcpy(), sprintf()
scripts/check-semgrep.sh       Called by pre-commit hook
```

---

## Architecture

### C++ Backend (`kde/src/`)

| File | Purpose |
|------|---------|
| `main.cpp` | Entry point — QApplication, system tray, global hotkey wiring, update checker |
| `qalcbridge.cpp/h` | Core calculation engine — wraps libqalculate; handles currency, date, percent, unit preprocessing |
| `documentmodel.cpp/h` | QAbstractListModel exposed to QML — owns document state, session history, KWin rules, autostart |
| `shortcutmanager.cpp/h` | KGlobalAccel global hotkey management (default: Ctrl+Alt+1) |
| `syntaxhighlighter.cpp/h` | Token-based HTML highlighter for the editor pane |
| `updatechecker.cpp/h` | GitHub API polling for new releases (24h auto-check interval) |

### QML Frontend (`kde/qml/`)

| File | Purpose |
|------|---------|
| `Main.qml` | Root ApplicationWindow — settings/history drawers, window chrome, `hideWindow()` |
| `DocumentPage.qml` | Main layout — editor + results split, splitter drag, Ctrl+N, total footer |
| `EditorPane.qml` | Textarea with syntax-highlight overlay, Tab autocomplete, animated yellow cursor |
| `ResultsPane.qml` | Right-side result column, scroll synced with editor |
| `SettingsPane.qml` | Settings drawer — font size, result width, decimal places, default currency, global hotkey, version/About link |
| `HistoryPane.qml` | Session history drawer — list of past documents, restore on tap |

### Calculation Flow

```
EditorPane.onTextChanged
  → documentModel.source = text            [QML]
  → DocumentModel::setSource()             [C++, debounce 50ms timer]
  → QtConcurrent::run(evaluateDocument)    [background thread via QFutureWatcher]
  → QalcBridge::evaluateDocument()         per line:
      1. tryEvaluateCurrencyExpr()          custom currency arithmetic regex
      2. tryDateDifference()                date span (today - date)
      3. tryDateArithmetic()                date offset (date + N years)
      4. percent regex                      20% of 500
      5. libqalculate::calculate()          everything else
  → DocumentModel::onEvaluationFinished()  [main thread — model reset → QML update]
```

### Currency System

| Source | Coverage | Cache |
|--------|----------|-------|
| Frankfurter API | Fiat (USD base) | 1 hour |
| CoinGecko API | Top-50 crypto | 1 hour |

Rates become `AliasUnit` objects inside libqalculate.
Mixed expressions (`500 EUR - 100 USD`, `1 BTC + 1 ETH`) handled by `tryEvaluateCurrencyExpr()`.
Default currency setting controls output when mixing without explicit target.

### Session History

`DocumentModel::saveSession()` is called on:
- Window hide (`hideWindow()` in Main.qml)
- `Ctrl+N` (before clearing)
- History button click (`↺`) — before drawer opens
- `aboutToQuit`

Built-in deduplication: empty source, `/help`, and exact repeat of last entry are skipped.
Max 25 entries, persisted via `QSettings("numi-kde", "numi-kde")`.

### KDE Integration

| Feature | Implementation |
|---------|---------------|
| Keep-above X11 | `KX11Extras::setState(NET::KeepAbove \| NET::SkipTaskbar \| NET::SkipPager)` |
| Keep-above Wayland | Writes `~/.config/kwinrulesrc` + DBus `org.kde.KWin reconfigure` |
| Global shortcut | `KGlobalAccel` + configurable sequence (default `Ctrl+Alt+1`) |
| Autostart | Writes `~/.config/autostart/numi-kde.desktop` |
| Tray icon | `QSystemTrayIcon` with context menu (toggle, check updates, quit) |

---

## Build

Requirements: **CMake ≥ 3.24**, Qt 6, KF6 (kwindowsystem, kglobalaccel), libqalculate, C++20 compiler.

```sh
# Debug — fast iteration
cmake -S kde -B kde/build -DCMAKE_BUILD_TYPE=Debug
cmake --build kde/build -j$(nproc)
./kde/build/numi-kde-tests          # must be 70/70

# Release + RPM
cmake -S kde -B kde/build-release -DCMAKE_BUILD_TYPE=Release
cmake --build kde/build-release -j$(nproc)
cpack -G RPM --config kde/build-release/CPackConfig.cmake
```

---

## Development Workflow (GitHub Flow)

**All changes go through a feature branch + Pull Request. Never commit directly to `main`.**

1. Create a branch from `main`:
   ```sh
   git checkout main && git pull
   git checkout -b feature/short-description
   ```
2. Develop on the branch — commit freely, tests must pass before each commit
3. Push branch and open PR:
   ```sh
   git push origin feature/short-description
   gh pr create --title "Short description"
   ```
4. CI runs tests on the PR branch — verify it passes
5. Merge PR into `main` (squash or merge commit)
6. Tag + release (see below)

---

## Release Process

Runs **after** the feature PR is merged into `main`.

1. Bump `project(... VERSION X.Y.Z ...)` in `kde/CMakeLists.txt`
2. Add top entry in `CHANGELOG.md`
3. Commit on `main` (version bump only):
   ```sh
   git add kde/CMakeLists.txt CHANGELOG.md
   git commit -m "vX.Y.Z: description"
   ```
4. Push + tag:
   ```sh
   git push origin main
   git tag vX.Y.Z
   git push origin vX.Y.Z
   ```
5. CI builds RPM in clean Fedora container, runs all tests, publishes GitHub Release
   with `numi-kde-X.Y.Z-x86_64.rpm + install.sh + uninstall.sh + SHA256SUMS`

**Never** upload the RPM manually — it creates a race condition where the local RPM is
replaced by the CI-built one while SHA256SUMS is still being generated, causing 404 on install.

---

## Static Analysis

Pre-commit hook runs `scripts/check-semgrep.sh` on staged C++ files.
Rules in `.semgrep.yml`: `system()`, `popen()`, `strcpy()`, `sprintf()` are forbidden.

Activate the hook once per repo clone:
```sh
git config core.hooksPath .githooks
```

---

## Personal Data Policy

- Commit author: `DMYTROSKORIN` / `DMYTROSKORIN@users.noreply.github.com`
- Public contact in source files: `dev@skorin.online`
- No other personal emails, tokens, or private paths anywhere in the repository
