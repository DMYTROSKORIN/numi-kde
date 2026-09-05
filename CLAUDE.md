# CLAUDE.md — numi-kde

This file is read by Claude Code and AI agents working on this project.
It describes architecture, conventions, and workflow. Read it fully before making any changes.

---

## Critical Rules

- **Never add `Co-Authored-By`** lines to commits — ever
- **Never change code without owner approval** — propose first, implement after explicit confirmation
- **Release**: tag-push only — see Release Process below. Never `gh release create <file>`
- **Tests must pass** before any commit — run the whole suite (needs a session bus for the D-Bus contract test):
  ```
  dbus-run-session -- ctest --test-dir kde/build --output-on-failure
  ```
  Suites: `qalc`, `documentmodel`, `editor`, `kwinrules` (+ window memory), `kwinscript` (+ D-Bus contract),
  `golden`, `stress`, and the shell test `kde/tests/shell/test-install-helper.sh`.
  Golden documents live in `kde/tests/fixtures/golden/*.numi`; after an intended output change run
  `./kde/build/numi-kde-tests golden --record` and review the `.expected` diff before committing.
  Exchange rates in tests come from `kde/tests/fixtures/rates.json` (`NUMI_KDE_OFFLINE_RATES=1`), never from the network.

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
  resources/     Icons (original PNG logo, pre-rendered hicolor sizes in icons/hicolor — no SVG, owner's decision;
                 tray: numi-kde-tray.png for dark schemes, numi-kde-tray-light.png = inverted, picked by palette),
                 online.skorin.numi-kde.desktop / .metainfo.xml,
                 numi-kde.notifyrc, polkit policy + update helper script
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
| `updatechecker.cpp/h` | GitHub API polling for new releases (24h auto-check interval); runs the polkit helper with a version number; restart logic lives in `main.cpp` |
| `kwinrulemanager.cpp/h` | KConfig-based management of our KWin window rules in `kwinrulesrc` (KWin 6 format, UUID groups, `rules=`/`Order=` list) |
| `windowmemory.cpp/h` | D-Bus endpoint `/WindowMemory` (`online.skorin.numi_kde.WindowMemory`) used by the KWin script `resources/kwin-script` to save/restore window positions on Wayland |

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
| Keep-above Wayland | Two KWin rules (main / settings window, exact `app_id` + title match) via `KWinRuleManager` + DBus `org.kde.KWin reconfigure`. Never rewrite `kwinrulesrc` by hand — KWin owns it |
| Window position Wayland | **KWin never persists `positionrule=4` for Wayland windows** (verified in KWin 6.7 sources: no `finishWindowRules()` for xdg toplevels). Shipped KWin script `numi-kde-window-memory` → D-Bus `WindowMemory` → `QSettings [WindowMemory]`; restored on `workspace.windowAdded`. D-Bus interface names cannot contain `-`, hence `numi_kde` |
| Single instance | `KDBusService(Unique)`, service `online.skorin.numi-kde`; second launch → `activateRequested` → show window |
| Notifications | `KNotification` events `updateInstalled`, `updated`, `updateFailed` from `resources/numi-kde.notifyrc` |
| Global shortcut | `KGlobalAccel` + configurable sequence (default `Ctrl+Alt+1`) |
| Autostart | Writes `$XDG_CONFIG_HOME/autostart/online.skorin.numi-kde.desktop` (`Exec=numi-kde --hidden`) |
| Tray icon | `QSystemTrayIcon` with context menu (toggle, check updates, quit) |

---

## Build

Requirements: **CMake ≥ 3.24**, Qt 6, KF6 (kwindowsystem, kglobalaccel, kconfig, kdbusaddons, knotifications), libqalculate, C++20 compiler.
Runtime: `kf6-qqc2-desktop-style` (QML style `org.kde.desktop`), `kf6-kdeclarative` (`KeySequenceItem`), `curl` (update helper).

Application id is `online.skorin.numi-kde` (`NUMI_KDE_APP_ID` in CMake). `applicationName`/`organizationName` stay `numi-kde` on purpose: QSettings path, history and the KGlobalAccel component depend on them.

```sh
# Debug — fast iteration
cmake -S kde -B kde/build -DCMAKE_BUILD_TYPE=Debug
cmake --build kde/build -j$(nproc)
./kde/build/numi-kde-tests qalc          # plus documentmodel, editor, kwinrules — all green

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

## Self-Update Flow

1. `UpdateChecker::checkAsync()` polls `releases/latest` (hourly timer, 24 h gate).
2. Newer tag → `installUpdate()` runs `pkexec /usr/libexec/numi-kde-install-update <version>`.
   The helper (`resources/numi-kde-install-update.sh`, polkit `allow_active=yes`) downloads the RPM
   and `SHA256SUMS` from the release itself, verifies checksum + package name, refuses downgrades, then `dnf install`.
   **Never pass a file path to the helper** — that was a local privilege escalation in ≤ 0.1.80.
3. `main.cpp`: window hidden → `restartApp()` immediately (`numi-kde --hidden`); window visible → restart on next hide,
   KNotification with "Restart now". First start after an update shows one "Numi-KDE updated" notification.

## Test Layers

| Layer | What it guards | Where |
|-------|----------------|-------|
| `qalc` | math, units, dates, currency (fixture rates), completions | `tests/qalc_test.cpp` |
| `golden` | whole documents line-by-line vs `.expected` (rounding, dates, unicode vars, edge cases) | `tests/fixtures/golden/` |
| `documentmodel` | model roles, totals, history, autostart + migration | `tests/qalc_test.cpp` |
| `editor` | QML key handlers (Tab completion, Esc) in a QQuickView | `tests/qalc_test.cpp` |
| `kwinrules` | KConfig rule writing in KWin 6 format, legacy cleanup, `WindowMemory` storage | `tests/qalc_test.cpp` |
| `kwinscript` | `resources/kwin-script` run in QJSEngine with mocked `workspace`/`callDBus`; D-Bus interface/path/service contract incl. live round trip | `tests/qalc_test.cpp` |
| `stress` | abort of a running evaluation; concurrent completions/highlighting while evaluating (run under TSAN in CI) | `tests/qalc_test.cpp` |
| shell | polkit helper with stubbed curl/rpm/dnf: rejects paths, downgrades, bad checksums, foreign packages | `tests/shell/test-install-helper.sh` |
| packaging | RPM must contain every runtime file and dependency | `scripts/check-rpm-contents.sh` (CI + release) |
| e2e (manual) | real KWin: show → move → hide → show → restart, geometry compared | `scripts/e2e-window-position.sh` — run on a Plasma Wayland session before a release that touches window handling |

CI: `ci.yml` runs ctest under `dbus-run-session`, builds the RPM and checks its contents, runs the shell tests,
`desktop-file-validate`, `appstreamcli validate`, and a ThreadSanitizer job (`-DNUMI_KDE_TSAN=ON`, suppressions in `tests/tsan.supp`).

## Static Analysis

Pre-commit hook runs `scripts/check-semgrep.sh` on staged C++ files.
Rules in `.semgrep.yml`: `system()`, `popen()`, `strcpy()`, `sprintf()` are forbidden.
Only the project rules are used (no `--config auto` / online registry) — builds must not depend on network access.
CI additionally runs `desktop-file-validate` and `appstreamcli validate` on `kde/resources/`.

Activate the hook once per repo clone:
```sh
git config core.hooksPath .githooks
```

---

## Personal Data Policy

- Commit author: `DMYTROSKORIN` / `dev@skorin.online`
- Public contact in source files: `dev@skorin.online`
- No other personal emails, tokens, or private paths anywhere in the repository
