# Changelog

## 0.1.34 - 2026-05-04

### UX & Interface

- **Rebranded to Numi-KDE**: updated the application name to "Numi-KDE" in the main window title, application menu, and metadata to ensure consistent naming across the system.

### Packaging

- **Lean Ubuntu DEB build**: reduced the Ubuntu container dependency installation with `--no-install-recommends` and non-PTY dpkg output to keep the required DEB release job reliable.

## 0.1.33 - 2026-05-04

### Packaging

- **Restore DEB release support**: the Ubuntu packaging job now runs in an Ubuntu 25.10 container where KF6 development packages are available, and DEB builds are required for publishing a release.
- **Debian runtime dependencies**: CPack now uses `dpkg-shlibdeps` so generated `.deb` packages include runtime library dependencies instead of an empty `Depends` field.

## 0.1.32 - 2026-05-04

### Changes

- **Frankfurter fiat rates**: fiat currency conversion now uses `https://api.frankfurter.dev/v2/rates?base=USD` as the managed rate provider. Rates are converted into a local USD-based map and applied to libqalculate currency units.
- **Unified currency conversion path**: fiat and crypto conversions now share the same USD-rate logic, so expressions such as `500 AED to USD`, `1 BTC to EUR`, and `500 EUR - 100 USD` use consistent conversion behavior.
- **Stable fallback**: libqalculate exchange rates are still loaded as currency unit definitions and fallback data, but live fiat rates are refreshed from Frankfurter on startup and then hourly.

## 0.1.31 - 2026-05-04

### Bug Fixes

- **Global hotkey regression after 0.1.27**: restored the stable 0.1.27 toggle behavior so a visible window hides and a hidden window is shown even when another application has focus. Removed the `KWindowSystem::activateWindow()` toggle path because KWin may reject explicit activation as focus stealing on Wayland.
- **Minimize button requiring two hotkey presses**: the custom `–` button now hides the window to tray instead of using compositor minimize. This avoids unreliable Wayland minimize state detection and makes the next hotkey press show the window immediately.

## 0.1.30 - 2026-05-04

### Bug Fixes

- **Global hotkey did not work when the window was not focused**: fixed a critical Wayland regression where the hotkey did not activate the window when another application had focus. Root cause: `win->requestActivate()` on Wayland requires an XDG activation token, which is not available when triggered through D-Bus (KGlobalAccel). `KWindowSystem::activateWindow()` is now used to handle the Wayland activation protocol.

## 0.1.29 - 2026-05-04

### Bug Fixes

- **Hotkey did not work after minimize (final fix)**: fixed the window toggle logic. Instead of unreliable `windowStates()`/`isExposed()` checks (both incorrect on Wayland), it now uses `isActive()`: the window is hidden only when it is visible and focused. A minimized window is never active, so the hotkey always restores it. Bonus: if the window is open behind another application, the hotkey brings it forward instead of hiding it.

## 0.1.28 - 2026-05-04

### Bug Fixes

- **Minimize + hotkey/tray requires two presses (Wayland fix)**: on Wayland, the compositor owns the minimize state and never sets `Qt::WindowMinimized`, so the previous fix (0.1.27) did not work. `QWindow::isExposed()` is now used because it correctly returns `false` for minimized windows on both X11 and Wayland.

## 0.1.27 - 2026-05-04

### Bug Fixes

- **Minimize + hotkey requires two presses**: Fixed an issue where pressing the global hotkey (or clicking the tray icon) after minimizing the window would hide it instead of restoring it. Root cause: a minimized window reports `isVisible() == true` in Qt, so the toggle logic treated it as "shown" and called `hide()`. The fix checks `windowState()` for `Qt::WindowMinimized` and restores the window in that case.

## 0.1.26 - 2026-05-04

### Bug Fixes

- **Currency Variable Arithmetic**: Fixed a regression where `A = 500 AED to USD` followed by `A + 200` showed the unsimplified expression `USD 136.xxx + 200.000` instead of computing the sum. The variable is now stored as a plain numeric value so that arithmetic with plain numbers works correctly.
- **No Trailing Zeros on Integers**: Fixed the related issue where the integer `200` was displayed as `200.000` in compound expressions involving currency-valued variables.

## 0.1.25 - 2026-05-04

### Bug Fixes

- **Variable Substitution Fix**: Resolved an issue where variables assigned with currency conversions (e.g., `A = 500 AED to USD`) resulted in unsimplified expressions when used in subsequent calculations. Variables now preserve their mathematical structure instead of being treated as plain text.

### UX & Interface

- **Version Display**: Added the application version information to the bottom of the settings drawer for better version tracking.

## 0.1.24 - 2026-05-03

### UX & Interface

- **Shadowing Units for Variables**: Allowed using unit names (like `A`, `B`, `C`, `m`) as variable names. This resolves the "Error" result when assigning conversion results to short variable names, as the engine now correctly shadows the units.
- **Improved Error Reporting**: The interface now displays specific error messages (like "Reserved name") instead of a generic "Error" text.

## 0.1.23 - 2026-05-03

### Bug Fixes

- **Fix QML Loading Crash**: Resolved a "Non-existent attached object" error caused by an unqualified ScrollBar property in SettingsPane.qml.
- **Improved UI Robustness**: Added null checks for Window.window attached properties in HistoryPane and SettingsPane to prevent runtime crashes during engine initialization.

## 0.1.22 - 2026-05-03

### UX & Interface

- **Final Fix for Currency Sums**: Resolved the "Error" result when adding mixed currencies (e.g., `600 AED + 400 USD`). The fix involves improved preprocessing of currency symbols and correct handling of variable assignments for unsimplified results.

## 0.1.21 - 2026-05-03

### UX & Interface

- **Settings Interface Restyling**: Completely restyled the settings panel to match the Numi-KDE custom dark theme. Replaced default Qt Quick Controls 2 delegates with custom-drawn CheckBoxes, Sliders, and TextFields to ensure visual consistency.

## 0.1.20 - 2026-05-03

### UX & Interface

- **New Settings Interface**: Migrated the settings window into a side drawer (similar to the History pane). This completely resolves window positioning issues on Wayland by keeping all interface elements within the main application window.
- **Improved Math Logic**: Fixed a bug where currency sums (e.g., `AED + USD`) resulted in an "Error". The engine now correctly handles and displays multi-term currency results.

## 0.1.19 - 2026-05-03

### Performance & Stability

- **Wayland Position Memory Fix**: Rewrote the window position logic to resolve the "centering every second show" bug on KDE Wayland. The application now uses an idempotent KWin rule injection strategy and disables client-side position restoration on Wayland to prevent conflicts with KWin's native `Remember` mechanism.
- **Settings Dialog Jumping Fix**: Disabled manual dialog centering on Wayland to prevent the settings window from jumping to the top-left corner due to unreliable client-side coordinates.

## 0.1.18 - 2026-05-03

### Performance & Stability

- **Window Position Persistence Fix**: The main window now saves position only after user-initiated movement or resize, so compositor remap placement cannot overwrite the last user position.
- **Settings Dialog Placement Fix**: The settings dialog is now a transient child of the main window and recenters over it when opened.
- **Repository Hygiene**: Added tracked release/authorship rules and a network-independent semgrep pre-commit hook.

## 0.1.17 - 2026-05-03

### Performance & Stability

- **Window Position Persistence Fix**: The main window now saves its position immediately before hiding and disables compositor-placement writes while hidden, preventing the saved position from being overwritten by a centered fallback after the next toggle.
- **Settings Dialog Placement**: The settings dialog now opens centered over the main application window instead of persisting its own position.

## 0.1.16 - 2026-05-03

### Performance & Stability

- **Window Position Startup Fix**: The main window is now shown from C++ after startup preparation, so QML no longer maps it before KWin rules are written. Position persistence is owned by QML on X11 and by KWin's `Remember` rule on Wayland.
- **Settings Window Position Persistence**: The settings dialog now restores and saves its own position.
- **Public Repository Cleanup**: Removed internal notes and stale documentation references.

## 0.1.15 - 2026-05-03

### Performance & Stability

- **Dark-Only UI**: Removed the light theme option and related theme state.
- **Cross-Currency Arithmetic**: Fixed expressions such as `500 EUR - 100 USD` so the result keeps the left-hand currency.
- **Global Shortcut Reliability**: Restored `setShortcut(NoAutoloading)` alongside native global shortcut registration.
- **Window Position Follow-Up**: Improved hide/show position handling; startup restoration is completed in `0.1.16`.

### Calculator

- **Expanded Help**: Added Area, Data, and Speed examples to `/help`.

## 0.1.13 - 2026-05-03

### Performance & Stability

- **Final Fix for Global Shortcuts**: Resolved the persistence issue where custom shortcuts reverted to defaults. The system now uses local configuration as the source of truth and forces `KGlobalAccel` to apply these settings on every launch, bypassing daemon auto-loading conflicts.
- **Robust Window Position Restoration**: Implemented an active position restoration logic in QML (`onVisibleChanged`). The window now forcefully re-applies its saved coordinates whenever it transitions from hidden to visible, countering compositor-level placement resets on Wayland and X11.
- **Improved Mathematical Logic**: Enhanced the calculation engine to reject unsimplified results (e.g., `USD 500 - 600`) as `Error`. This prevents confusing output when mixing incompatible units or using incorrect number formats, ensuring results are always mathematically sound.

### UX & Branding

- **Interactive Update Notifications**: System notifications for new versions are now actionable. Clicking a notification (or the tray menu entry) will immediately open the official GitHub releases page for instructions and downloads.

## 0.1.12 - 2026-05-03

### Performance & Stability

- **Emergency Fix: Window Visibility**: Reverted to using `hide()` instead of `showMinimized()` for window closing. This fixes a critical regression where the frameless window would become unreachable on some Wayland/X11 configurations. Implemented manual position restoration to ensure the window still remembers its last location.
- **Emergency Fix: Global Shortcuts**: Completely rewrote the `ShortcutManager` to fix a regression where shortcuts failed to register or persist. The system now uses a hybrid approach of local `QAction` shortcuts and native `KGlobalAccel` registration, ensuring reliable triggering and persistent user preferences across restarts.

## 0.1.11 - 2026-05-03

### Performance & Stability

- **Strict Input Noise Filter**: Implemented a heuristic to ignore arbitrary text and invalid commands (e.g., junk text or malformed `/help` calls). Only lines that contain digits, mathematical operators, assignments, or known symbols are now evaluated, keeping the result area clean from non-mathematical noise.
- **Improved Currency & Large Number Parsing**: Fixed a critical issue where large numbers with thousands separators were being interpreted as vector lists. Reimplemented group separator removal using a robust backwards-iteration logic that handles multiple separators (e.g., `EUR 7,486,332.414`) correctly.
- **Finalized Shortcut Persistence**: Resolved an issue where custom global shortcuts reset after application restart. The system now uses robust `QSettings` management combined with `KGlobalAccel::NoAutoloading` and explicit desktop file identification, ensuring user preferences are strictly honored by the KDE desktop.

### Calculator

- **Division Operator Alias**: Added support for `:` as a division operator (e.g., `2 : 4` now returns `0.5`), matching common notation in some regions.
- **Vector Rejection**: Evaluated results that produce vectors or matrices (often caused by illogical or comma-heavy input) are now explicitly flagged as `Error` instead of showing confusing bracketed lists.

## 0.1.10 - 2026-05-03

### Performance & Stability

- **Improved Window Position Persistence**: Switched from `hide()` to `showMinimized()` for the main window toggle. This preserves the surface mapping and compositor state, ensuring the window reappears exactly where the user left it on both Wayland and X11.
- **Enhanced Calculation Parsing**: Fixed an issue where numbers with thousands separators (e.g., `22,045.00`) were incorrectly parsed as lists by `libqalculate`. Pre-processing now safely strips locale-aware group separators from numbers before evaluation.
- **Robust Shortcut Persistence**: Refactored `ShortcutManager` to use native `KGlobalAccel` configuration. User-defined global shortcuts are now stored directly in KDE's settings (`kglobalshortcutsrc`), ensuring they are remembered across app restarts.

### UX & Branding

- **Light Theme**: Added a new "Light Theme" option in settings with a palette inspired by old paper/milky white (`#f2e9d0`). All UI components (settings, history, drawer) now dynamically adapt to the selected theme.
- **Update Checker Notifications**: Fixed a `QSettings` organization mismatch that prevented update checks from persisting. Added system tray notifications for manual update checks: "Checking for updates...", "You are using the latest version", and "A new version is available".

## 0.1.9 - 2026-05-03

### Performance & Stability

- **Async Evaluation**: Moved the calculation engine (`libqalculate`) to a background thread using `QtConcurrent`. The UI now remains perfectly responsive even during heavy computation or rapid typing.
- **Throttling/Debouncing**: Added a 50ms delay to evaluations to prevent CPU spikes during rapid text input.
- **Highlighter Optimization**: Implemented classification caching in `SyntaxHighlighter`. Repeated tokens (units, keywords) are now highlighted near-instantly without re-querying the math engine.
- **Variable Persistence Fix**: User-defined variables are now correctly cleared before each full document re-evaluation, preventing deleted lines from affecting future results.
- **ListView Optimization**: Refactored the QML highlight layer to use `ListView` instead of `Repeater`. Only visible lines are rendered, drastically improving performance for large documents.

### UX & Branding

- **Network Status Indicator**: Added a colored status dot in the title bar to show the state of cryptocurrency rate updates (Fetching, Success, Error). Includes detailed ToolTips.
- **Professional Branding**: Updated all application identifiers and metadata to use the formal `numi-kde` organization name.
- **Contact Info**: Updated official contact email to `dev@skorin.online`.
- **Atomic KWin Rules**: Switched to `QSaveFile` for updating `kwinrulesrc`, ensuring configuration safety on KDE Wayland.

### Calculator

- **Variable Validation**: Improved assignment logic to prevent overwriting of system units (like `m`, `kg`, `USD`) or reserved keywords.

## 0.1.8 - 2026-05-02

### Update checker

- Added automatic update checking via the GitHub Releases API (`api.github.com`).
- On startup, the app checks for a newer release once per 24 hours (last-check timestamp stored in `QSettings`).
- The tray menu gains a **Check for Updates** action for manual checks.
- When a newer version is found, the action label changes to **Update available: vX.Y.Z** and clicking it opens the GitHub release page in the browser.
- Repository is now public: `https://github.com/DMYTROSKORIN/numi-kde`.

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
