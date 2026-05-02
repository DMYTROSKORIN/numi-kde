# Native KDE Application

Last updated: 2026-05-02.

The native KDE application lives in `kde/` and is the primary runtime target.

## Stack

- Qt 6 / QML / Qt Quick Controls.
- C++ backend.
- `libqalculate` calculation engine.
- KF6WindowSystem for window integration.
- KF6GlobalAccel for global shortcuts when available.
- Qt DBus for KWin reconfiguration.

## Fedora Dependencies

```sh
sudo dnf install -y \
  cmake gcc-c++ libqalculate-devel \
  qt6-qtbase-devel qt6-qtdeclarative-devel \
  kf6-kwindowsystem-devel kf6-kglobalaccel-devel
```

The CMake target requires Qt 6 Core/DBus/Gui/Widgets/Network/Qml/Quick/QuickControls2,
KF6WindowSystem, optional KF6GlobalAccel, and libqalculate.

## Build, Test, Run

```sh
cmake -S kde -B build/kde
cmake --build build/kde --target numi-kde numi-kde-tests
ctest --test-dir build/kde --output-on-failure
./build/kde/numi-kde --version
./build/kde/numi-kde --probe
./build/kde/numi-kde
```

## Always on Top

**X11:** Uses `KX11Extras` with `NET::KeepAbove | NET::SkipTaskbar | NET::SkipPager`.
QML keeps static window flags to avoid window recreation and re-centering.

**KDE Wayland:** Wayland clients cannot directly control stacking.
`DocumentModel` writes a managed KWin Window Rule to `~/.config/kwinrulesrc`
and calls `org.kde.KWin.reconfigure` via DBus.
The rule is identified by `Description=Numi-KDE keep above (managed)`.
It sets `keepabove`, `skiptaskbar` and `skippager` — all forced.

## Window Behavior

- Closing the window (X button or Alt+F4) hides it; the app stays alive in the system tray.
- To quit, use **Quit Numi-KDE** from the tray context menu.
- Window position is saved to `QSettings` before hiding and restored on show.

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
