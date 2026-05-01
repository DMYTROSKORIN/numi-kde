# numi-kde

KDE/Linux-native clone of Numi: a compact text-document calculator with expressions on the left and results on the right.

## Current Status

- Runtime backend: C++/Qt 6 with `libqalculate`.
- Desktop integration: tray icon/menu, global shortcut, clipboard, saved window geometry, launch-at-login, KDE/KWin keep-above integration.
- Current local verification: native CMake build and native CTest pass.

## Features

- Math: `2 + 2 * 3^2`, `sqrt(256)`, `sin(pi/2)`.
- Units: `10 meters in feet`, `50 kg to lbs`, `1 km to m`.
- Currency and crypto: case-insensitive fiat units where libqalculate supports them, plus live CoinGecko-backed top-crypto conversion such as `400 USD to ETH`.
- Dates/time: `today + 2 weeks`, `today - 26.08.1983`, `time`, `now`.
- Variables: `A = 800 - 200`, `B := A * 2`.
- Percentages: `20% of 300`, `20% from 300`; incomplete input like `20% from` stays quiet until finished.
- `/help`: clear in-app usage guide with examples for math, variables, units, currency, dates and controls.
- Total footer: sums numeric result rows, including converted unit/currency/crypto result values, and can copy the total.
- History drawer: saves/restores recent sessions and can clear history.
- Settings: always on top, launch at login, result separator, font size, result column width, decimal places, global hotkey.
- Result column: auto-expands for long results and prevents the separator from covering visible result text.
- Keyboard: `Ctrl+Alt+1` toggles the app globally by default; `Ctrl+N` clears the current document; `Tab` completes units/functions/variables.

## KDE Notes

On KDE Wayland, clients cannot directly force stacking above other windows. `numi-kde` therefore writes a managed KWin window rule when "Always on top" is enabled and asks KWin to reconfigure. On X11 it also uses `NET::KeepAbove`.

The app and tray icons in `kde/resources/` are transparent PNGs. They are embedded into the Qt resource binary during build.

## Build and Run

### Fedora Dependencies

```sh
sudo dnf install -y \
  cmake gcc-c++ libqalculate-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kwindowsystem-devel kf6-kglobalaccel-devel
```

Package names can vary by distro; the CMake requirements are Qt 6 Core/DBus/Gui/Widgets/Network/Qml/Quick/QuickControls2, KF6WindowSystem, optional KF6GlobalAccel, and libqalculate.

### Commands

```sh
cmake -S kde -B build/kde
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde
```

## Documentation

- `docs/AI_HANDOFF.md`: current handoff for the next agent.
- `docs/handoff.md`: concise operational project handoff.
- `docs/kde-native.md`: native KDE build/runtime notes.
- `docs/implementation-plan.md`: roadmap and current phase status.
- `docs/architecture.md`: current layer boundaries.
- `CHANGELOG.md`: release notes.
