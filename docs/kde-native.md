# Native KDE Prototype

The native KDE prototype lives in `kde/`.

It is the starting point for the final Qt 6/KF6/Kirigami application. The current skeleton already
defines:

- `kde/CMakeLists.txt`
- `kde/src/main.cpp`
- `kde/qml/Main.qml`
- `kde/qml/DocumentPage.qml`
- `kde/qml/EditorPane.qml`
- `kde/qml/ResultsPane.qml`
- desktop and AppStream metadata.

The QML shell mirrors the tested web GUI prototype:

- source editor on the left;
- result column on the right;
- SplitView layout;
- monospace editor/result typography;
- KDE theme colors;
- placeholder actions for New/Open/Save/Preferences.

## Fedora Dependencies

On Fedora KDE, install the missing development packages:

```sh
sudo dnf install -y qt6-qtbase-devel qt6-qtdeclarative-devel kf6-kirigami-devel
```

These provide the missing CMake packages:

- `Qt6`
- `Qt6Qml`
- `Qt6Quick`
- `Qt6QuickControls2`
- `KF6Kirigami`

In the current workspace, package discovery confirmed:

- `qt6-qtbase-devel` provides `cmake(Qt6)`;
- `qt6-qtdeclarative-devel` provides `cmake(Qt6Qml)` and `cmake(Qt6Quick)`;
- `kf6-kirigami-devel` provides `cmake(KF6Kirigami)`.

Automatic installation was not completed because `sudo` requires the user's password.

## Build

After installing dependencies:

```sh
cmake -S kde -B build/kde
cmake --build build/kde
```

Run:

```sh
./build/kde/numi-kde
```

## Current Limitations

The native skeleton does not yet call the JS core. It is a UI shell scaffold. The next step is to
connect it to the shared core through a local adapter process or native backend bridge.

The web prototype remains the runnable GUI for immediate UX testing:

```sh
npm run gui
```

Then open:

```text
http://127.0.0.1:15156
```

## Next Work

- Wire native `DocumentPage` to the shared evaluation backend.
- Replace placeholder results with live results.
- Add semantic token highlighting using `parseDocument()`.
- Add font size and result column width settings.
- Add native screenshot checks once the binary builds.
