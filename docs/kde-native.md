# Native KDE Application

Last updated: 2026-05-01.

The native KDE application lives in `kde/` and is the primary runtime target.

## Stack

- Qt 6 / QML / Qt Quick Controls.
- C++ backend.
- `libqalculate` calculation engine.
- KF6WindowSystem for window integration.
- KF6GlobalAccel for global shortcuts when available.
- Qt DBus for KWin reconfiguration.

The repository is scoped to the native KDE application. It does not include the old JavaScript/web prototype.

## Fedora Dependencies

```sh
sudo dnf install -y \
  cmake gcc-c++ libqalculate-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kwindowsystem-devel kf6-kglobalaccel-devel
```

The CMake target requires Qt 6 Core/DBus/Gui/Widgets/Network/Qml/Quick/QuickControls2, KF6WindowSystem, optional KF6GlobalAccel, and libqalculate.

## Build, Test, Run

```sh
cmake -S kde -B build/kde
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde
```

Latest local status:

- configure: passes;
- native build: passes;
- native CTest: 2/2 passes;
- GUI dev binary starts without QML warnings.

## Current Behavior

- Compact frameless dark window.
- Editable source document on the left.
- Result column on the right, with automatic expansion for long result text.
- Live C++ evaluation while typing.
- C++ syntax highlighting.
- Result hover/click behavior.
- Total footer for numeric rows, including converted unit/currency/crypto values.
- `/help` opens a readable in-app guide.
- `today - DD.MM.YYYY` returns a year/day date span.
- History drawer with persisted sessions.
- Settings window for display, behavior, precision, autostart and hotkey.
- Tray icon and tray menu.
- Global shortcut: `Ctrl+Alt+1` by default.
- `Ctrl+N`: clear current document.
- `Tab`: completion.
- Persistent window geometry.
- Transparent app/tray PNG resources.

## Always on Top

X11:

- Uses `KX11Extras` with `NET::KeepAbove`.
- QML keeps static window flags to avoid window recreation and re-centering.

KDE Wayland:

- Wayland clients cannot directly control stacking.
- When enabled, `DocumentModel` writes a managed KWin Window Rule to `~/.config/kwinrulesrc` and calls `org.kde.KWin.reconfigure`.
- The managed rule is identified by `Description=Numi-KDE keep above (managed)`.
- The rule matches `wmclass` substring `numi-kde` and title `Numi`, then sets `keepabove=true`.

Manual validation on a real KDE Wayland session is still required because automated tests cannot verify compositor stacking.

## Runtime Files

```text
kde/src/main.cpp
kde/src/documentmodel.{h,cpp}
kde/src/qalcbridge.{h,cpp}
kde/src/shortcutmanager.{h,cpp}
kde/src/syntaxhighlighter.{h,cpp}
kde/qml/*.qml
kde/resources/*.png
kde/tests/qalc_test.cpp
```

## Current Limitations

- No packaged-install smoke test yet.
- KWin keep-above rule writer is not unit-tested with an injected config path.
- Wayland keep-above depends on KWin accepting/reloading the generated rule.
