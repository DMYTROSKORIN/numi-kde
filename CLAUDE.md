# CLAUDE.md — numi-kde

This file is read by Claude Code and AI agents working on this project.
It describes architecture, conventions, and workflow. Read it fully before making any changes.

---

## Critical Rules

- **Never add `Co-Authored-By`** lines to commits — ever
- **Never change code without owner approval** — propose first, implement after explicit confirmation
- **GitHub Flow** — every change is a branch + PR; `main` receives merges and the release version bump only
- **Release**: tag-push only — see Release Process below. Never `gh release create <file>`
- **Tests must pass** before any commit — run the whole suite (needs a session bus for the D-Bus contract test):
  ```
  dbus-run-session -- ctest --test-dir kde/build --output-on-failure --timeout 300
  ```
  Suites: `qalc`, `documentmodel`, `editor`, `kwinrules` (+ window memory), `kwinscript` (+ D-Bus contract),
  `golden`, `stress`, `engine`, and the shell test `kde/tests/shell/test-install-helper.sh`.
  Golden documents live in `kde/tests/fixtures/golden/*.numi`; after an intended output change run
  `./kde/build/numi-kde-tests golden --record` and review the `.expected` diff before committing.
  Exchange rates in tests come from `kde/tests/fixtures/rates.json` (`NUMI_KDE_OFFLINE_RATES=1`), never from the network.
- **libqalculate never runs in the GUI process and never on two threads** — see Engine Process below
- **Icons**: the original PNG logo only (hicolor sizes pre-rendered); no SVG or symbolic redesigns — owner's decision
- **No artifacts / documents outside the repo** unless the owner asks; reports go to the terminal

---

## Project Overview

KDE-native document-style calculator for Fedora/KDE Plasma 6 (Wayland and X11).
The user writes expressions as plain text; results appear live next to each line.

- **GitHub**: https://github.com/DMYTROSKORIN/numi-kde
- **Language**: C++20 + Qt 6 + QML
- **Engine**: libqalculate, hosted in a separate process (`numi-kde-engine`)
- **Packaging**: GPG-signed RPM for Fedora via GitHub Actions CI; in-app self-update

---

## Repository Layout

```
kde/
  src/                   GUI process (numi-kde): main, DocumentModel, EngineClient, UpdateChecker, ShortcutManager,
                         KWinRuleManager, WindowMemory
  src/engine/            numi-kde-engine process (enginemain.cpp)
  src/qalcbridge.*       libqalculate bridge (engine only); syntaxhighlighter.* (engine only)
  src/engineprotocol.*   JSON-lines wire format; lineresult.h shared result struct
  qml/                   QML frontend
  tests/qalc_test.cpp    All C++ suites (one binary, suite name as argv[1])
  tests/fixtures/        rates.json (offline exchange rates), golden/*.numi + *.expected
  tests/shell/           test-install-helper.sh (polkit helper with stubbed curl/rpm/rpmkeys/dnf)
  tests/tsan.supp        ThreadSanitizer suppressions (libqalculate/GMP/Qt internals)
  resources/             online.skorin.numi-kde.desktop / .metainfo.xml, numi-kde.notifyrc, polkit policy,
                         numi-kde-install-update.sh (helper), kwin-script/ (window position memory),
                         icons/hicolor/<size>/apps/*.png (from numi-kde.png), numi-kde-tray*.png (dark/light)
  CMakeLists.txt         Build definition — version lives here; targets numi-kde, numi-kde-engine, numi-kde-tests
docs/
  architecture.md        Process model, runtime flow, KDE integration, self-update
  release.md             Workflow, tests, signing, release checklist, failed-release recovery
packaging/
  install.sh             One-command Fedora installer (checksum + signature)
  uninstall.sh           Uninstaller (--purge removes settings, autostart entry, imported key)
  RPM-GPG-KEY-numi-kde   Public half of the release signing key
scripts/
  check-semgrep.sh       Called by the pre-commit hook
  check-rpm-contents.sh  Required files/dependencies of the RPM (CI + release)
  e2e-window-position.sh Live Plasma check of Wayland position memory (manual, before window-related releases)
.github/workflows/ci.yml       Build & Test (ctest under dbus-run-session, RPM build + contents check),
                               Static Analysis (semgrep, shellcheck, desktop-file-validate, appstreamcli, helper tests),
                               ThreadSanitizer (-DNUMI_KDE_TSAN=ON)
.github/workflows/release.yml  Tag push → build, test, sign, verify, install + probe, publish
.githooks/pre-commit           Semgrep on staged source files (by extension)
.semgrep.yml                   Rules: no system(), popen(), strcpy(), sprintf()
```

---

## Architecture

### Engine Process

```
numi-kde (GUI)                                      numi-kde-engine (libexec)
DocumentModel → EngineClient ── JSON lines ──▶ EngineServer → one-thread QThreadPool → QalcBridge → libqalculate
                             ◀── progress/lines/events ──
```

- **Why**: libqalculate's `Calculator` is a process-wide singleton; a force-stopped calculation thread hangs
  `~Calculator()` and process exit, and driving it from two threads (even under a mutex) stalls it for the whole
  per-line timeout on every line. The child process turns all of that into a restart.
- **Threading contract** (`qalcbridge.h`): evaluation, highlight, single completion, setters, rate application —
  one thread only. Thread-safe from anywhere: `getCompletions()` (snapshot), `abortCalculation()`, `networkStatus()`.
- **EngineClient**: request ids from one counter (never reuse generation numbers as ids); watchdog 6 s without a
  finished line → kill, report *Calculation took too long*, **poison** the line text (sent as `skip` until edited),
  respawn with backoff 150 ms…5 s; sync completions/highlight capped at 400 ms; engine found via `$NUMI_KDE_ENGINE`,
  next to the binary, `<bindir>/../libexec`, compile-time libexec, `/usr/libexec`.
- **Engine**: per-line libqalculate timeout 3 s; exits with `_Exit(0)` on stdin EOF (never destroys the Calculator).
- Protocol reference: `kde/src/engineprotocol.h`. `_crash` op exists for tests only.

### C++ Sources (`kde/src/`)

| File | Purpose |
|------|---------|
| `main.cpp` | QApplication, KDBusService (single instance), tray (palette-aware icon), global hotkey wiring, update/restart policy, KNotifications; `--probe` evaluates through the engine process |
| `engine/enginemain.cpp` | `numi-kde-engine`: `StdinReader` thread + `EngineServer`; streams per-line progress |
| `engineclient.cpp/h` | GUI side of the engine (see above) |
| `engineprotocol.cpp/h`, `lineresult.h` | LineResult ⇄ JSON, framing; the shared result struct |
| `qalcbridge.cpp/h` | libqalculate wrapper: currency/date/percent/temperature preprocessing, `smartFormat`, error texts, completion index; `evaluateDocument(source, progress, skipLines)` |
| `syntaxhighlighter.cpp/h` | Token-based HTML highlighter (engine only) |
| `documentmodel.cpp/h` | QAbstractListModel for QML — document state, totals, history, clipboard, autostart, KWin rules, legacy migration |
| `shortcutmanager.cpp/h` | KGlobalAccel global hotkey (default `Ctrl+Alt+1`), `applyKeySequence()` for `KeySequenceItem` |
| `updatechecker.cpp/h` | GitHub API polling (hourly timer, 24 h gate); runs the polkit helper with a version number |
| `kwinrulemanager.cpp/h` | KConfig-based KWin rules in `kwinrulesrc` (KWin 6 format, UUID groups, `rules=`/`Order=`) |
| `windowmemory.cpp/h` | D-Bus `/WindowMemory` (`online.skorin.numi_kde.WindowMemory`) used by the KWin script to save/restore window positions on Wayland |

### QML Frontend (`kde/qml/`)

| File | Purpose |
|------|---------|
| `Main.qml` | Frameless ApplicationWindow — settings persistence (size only when `Windowed`), `hideWindow()`, `escHidesWindow`, history drawer, resize/move handles |
| `DocumentPage.qml` | Editor + results split, splitter, total footer, inline `/help` overlay, settings/help buttons, `Ctrl+N` |
| `EditorPane.qml` | `TextEdit` + highlight overlay ListView, Tab completion popup, Esc handling; exposes `flickable` (real ScrollView Flickable) and `textEdit` |
| `ResultsPane.qml` | Result column synced to the editor Flickable; click copies; tooltip with the error explanation |
| `SettingsWindow.qml` / `SettingsPane.qml` | Native settings window: always on top, autostart, separator, Esc hides, auto-install updates, font/result width/decimals, default currency, `KeySequenceItem` hotkey, version link |
| `HistoryPane.qml` | Session history drawer |

### Calculation Flow

```
EditorPane.onTextChanged
  → documentModel.source = text            [QML]
  → DocumentModel::setSource()             [50 ms debounce, ++generation]
  → EngineClient::evaluate(generation)     [cancel older ids; evaluate + poisoned skip list]
  → engine: QalcBridge::evaluateDocument   per line: blank/comment → skip flags → division by zero →
      currency expr → temperature → dates/time → percent → assignment → libqalculate::calculate (3 s)
  → engine replies {id, lines}; EngineClient::evaluated(generation, lines)
  → DocumentModel::onEvaluated()           [drops superseded generations, model reset → QML update]
```

Number formatting (`smartFormat`): half-away-from-zero rounding; scientific notation for ≥ 1e15 and for non-zero
values that would show as 0 at the chosen precision (never at 0 decimals); complex results printed by libqalculate.

### Currency System

| Source | Coverage | Cache |
|--------|----------|-------|
| Frankfurter API | Fiat (USD base) | 1 hour, `rates.json` in the engine's `CacheLocation` |
| CoinGecko API | Top-50 crypto | 1 hour |

Rates become USD-based `AliasUnit` objects inside libqalculate (existing units are updated in place).
Mixed expressions (`500 EUR - 100 USD`, `1 BTC + 1 ETH`) handled by `tryEvaluateCurrencyExpr()`.
Default currency setting controls output when mixing without explicit target.

### Session History

`DocumentModel::saveSession()` is called on window hide, `Ctrl+N`, history button click, `aboutToQuit`.
Deduplication: empty source, `/help`, exact repeat of last entry are skipped. Max 25 entries in
`QSettings("numi-kde", "numi-kde")`.

### KDE Integration

| Feature | Implementation |
|---------|---------------|
| App id | `online.skorin.numi-kde` (desktop, AppStream, icons, Wayland app_id); `applicationName` stays `numi-kde` for QSettings/KGlobalAccel/cache paths |
| Keep-above X11 | `KX11Extras::setState(NET::KeepAbove \| NET::SkipTaskbar \| NET::SkipPager)` |
| Keep-above Wayland | Two KWin rules (main / settings window, exact `app_id` + title) via `KWinRuleManager` + `org.kde.KWin reconfigure`. Never rewrite `kwinrulesrc` by hand |
| Window position Wayland | KWin never persists `positionrule=4` for Wayland windows. KWin script `numi-kde-window-memory` (shipped, enabled by default) → D-Bus `WindowMemory` → `QSettings [WindowMemory]`; restored on `workspace.windowAdded`. D-Bus interface names cannot contain `-` |
| Single instance | `KDBusService(Unique)`, service `online.skorin.numi-kde`; second launch → `activateRequested` → show |
| Window activation | `KWindowSystem::activateWindow` (honours xdg-activation tokens from kglobalacceld) |
| Notifications | `KNotification` events `updateInstalled`, `updated`, `updateFailed` (`resources/numi-kde.notifyrc`) |
| Global shortcut | `KGlobalAccel`, default `Ctrl+Alt+1`, recorded with `org.kde.kquickcontrols` `KeySequenceItem` |
| Autostart | `$XDG_CONFIG_HOME/autostart/online.skorin.numi-kde.desktop` (`Exec=numi-kde --hidden`); legacy entry migrated |
| Tray icon | `QSystemTrayIcon`; `numi-kde-tray.png` (dark schemes) / `numi-kde-tray-light.png` (light), swapped on palette change |

---

## Build

Requirements: **CMake ≥ 3.24**, Qt 6.6+, KF6 (kwindowsystem, kglobalaccel, kconfig, kdbusaddons, knotifications),
libqalculate, C++20 compiler. Runtime: `kf6-qqc2-desktop-style`, `kf6-kdeclarative` (`KeySequenceItem`), `curl`, `dnf`, `polkit`.

```sh
# Debug — fast iteration (builds numi-kde, numi-kde-engine, numi-kde-tests)
cmake -S kde -B kde/build -DCMAKE_BUILD_TYPE=Debug -GNinja
cmake --build kde/build -j$(nproc)
dbus-run-session -- ctest --test-dir kde/build --output-on-failure --timeout 300
./kde/build/numi-kde --probe                 # spawns kde/build/numi-kde-engine

# Release + RPM (what CI does)
cmake -S kde -B kde/build-release -DCMAKE_BUILD_TYPE=Release -GNinja
cmake --build kde/build-release -j$(nproc)
cmake --build kde/build-release --target package
bash scripts/check-rpm-contents.sh kde/build-release/numi-kde-*.rpm
```

The local `semgrep-check` target (project rules only, 60 s per file) runs as part of the `numi-kde` build when semgrep is installed.

---

## Development Workflow (GitHub Flow)

1. `git checkout main && git pull && git checkout -b feature/short-description`
2. Develop; tests green before each commit; `CHANGELOG.md` entry and metainfo `<release>` belong in the PR
3. `git push -u origin feature/short-description && gh pr create --base main --head feature/short-description ...`
4. CI: Build & Test, Static Analysis, ThreadSanitizer — all must pass
5. `gh pr merge <n> --merge --delete-branch`
6. Release (below)

---

## Release Process

Runs **after** the PR is merged into `main`.

1. Bump `project(... VERSION X.Y.Z ...)` in `kde/CMakeLists.txt`, commit on `main`: `vX.Y.Z: description`
2. `git push origin main && git tag vX.Y.Z && git push origin vX.Y.Z`
3. `release.yml`: build in a clean Fedora 44 container → ctest → cpack → **sign** (`rpmsign`, secrets
   `RPM_SIGNING_KEY`/`RPM_SIGNING_KEY_ID`) → verify signature against `packaging/RPM-GPG-KEY-numi-kde` → verify RPM
   contents → `dnf install` + `numi-kde --probe` → publish `numi-kde-X.Y.Z-x86_64.rpm`, `RPM-GPG-KEY-numi-kde`,
   `install.sh`, `uninstall.sh`, `SHA256SUMS`
4. If the job fails nothing is published: fix via PR, then delete and re-push the tag (`docs/release.md`)

**Never** upload the RPM manually — it races with the CI-built asset and SHA256SUMS.

---

## Self-Update Flow

1. `UpdateChecker::checkAsync()` polls `releases/latest` (hourly timer, 24 h gate).
2. Newer tag → `installUpdate()` runs `pkexec /usr/libexec/numi-kde-install-update <version>`.
   The helper (`resources/numi-kde-install-update.sh`, polkit `allow_active=yes`) downloads the RPM and `SHA256SUMS`
   from the release itself into a root-owned dir, verifies checksum + package name, refuses downgrades, imports
   `/etc/pki/rpm-gpg/RPM-GPG-KEY-numi-kde`, requires `rpmkeys --checksig` → `signatures OK` **and** signer key id
   `411c68b856cec16e`, then `dnf install` (no `--nogpgcheck`). Exit codes: 2 bad version, 3 downgrade, 4 download,
   5 checksum, 6 wrong package, 7 no/invalid signature, 8 foreign key.
   **Never pass a file path to the helper** — that was a local privilege escalation in ≤ 0.1.80.
3. `main.cpp`: window hidden → `restartApp()` immediately (`numi-kde --hidden`); window visible → restart on next hide,
   KNotification with "Restart now". First start after an update shows one "Numi-KDE updated" notification.

### Release signing

- Key `Numi-KDE Release Signing <dev@skorin.online>`, fingerprint `7022A79158931F4131646599411C68B856CEC16E`,
  expires 2029-09-04. Public key committed as `packaging/RPM-GPG-KEY-numi-kde`; private key only in GitHub secrets
  and the owner's password manager. **Never commit it.**
- The helper and `install.sh` hard-code the key id — rotate key file + both `EXPECTED_KEY_ID` constants together,
  and ship the new public key one release before switching the signer.

---

## Test Layers

| Suite / check | What it guards | Where |
|---------------|----------------|-------|
| `qalc` | math, units, dates, currency (fixture rates), completions, error texts | `tests/qalc_test.cpp` |
| `golden` | whole documents line-by-line vs `.expected` (rounding, scientific, complex, dates, unicode vars, units, time, totals, sum/product, edge cases) | `tests/fixtures/golden/` |
| `documentmodel` | model roles, totals, history, autostart + migration — through the real engine process | `tests/qalc_test.cpp` |
| `editor` | QML key handlers (Tab completion, Esc) in a QQuickView with a mock model | `tests/qalc_test.cpp` |
| `kwinrules` | KConfig rule writing in KWin 6 format, legacy cleanup; `WindowMemory` storage | `tests/qalc_test.cpp` |
| `kwinscript` | KWin script in QJSEngine with mocked `workspace`/`callDBus`; D-Bus service/path/interface contract incl. live round trip (needs a session bus) | `tests/qalc_test.cpp` |
| `stress` | in-process QalcBridge: cancel skips remaining lines; snapshot completions + cancel hammered while evaluating; run under TSAN in CI. Leaks its engine and exits via `_Exit` on purpose | `tests/qalc_test.cpp` |
| `engine` | real `numi-kde-engine`: round trip, sync helpers, stall → watchdog → poison → restart → skip → poison lifted on edit, forced crash, rapid supersedes | `tests/qalc_test.cpp` |
| shell | polkit helper with stubbed curl/rpm/rpmkeys/dnf: paths, bad versions, downgrades, missing assets, checksum mismatch, foreign package, unsigned, foreign key never reach dnf | `tests/shell/test-install-helper.sh` |
| packaging | RPM must contain every runtime file (both binaries, key, KWin script, icons…) and dependency | `scripts/check-rpm-contents.sh` |
| e2e (manual) | real KWin: show → move → hide → show → restart, geometry compared | `scripts/e2e-window-position.sh` |

Numbers in tests are locale-pinned to `en_US` (Qt locale and `LC_ALL`, because libqalculate reads the C locale at
`Calculator()` construction); CI containers install `glibc-langpack-en`.

---

## Static Analysis

Pre-commit hook runs `scripts/check-semgrep.sh` on staged source files (C/C++, QML, JS, shell, Python, YAML).
Rules in `.semgrep.yml`: `system()`, `popen()`, `strcpy()`, `sprintf()` are forbidden.
Only the project rules are used (no `--config auto` / online registry) — builds must not depend on network access.
CI additionally runs `shellcheck`, `desktop-file-validate` and `appstreamcli validate` (AppStream reports an
informational `cid-maybe-not-rdns` for the hyphen in the id — accepted).

Activate the hook once per repo clone:
```sh
git config core.hooksPath .githooks
```

---

## Personal Data Policy

- Commit author: `DMYTROSKORIN` / `dev@skorin.online`
- Public contact in source files: `dev@skorin.online`
- No other personal emails, tokens, or private paths anywhere in the repository
