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
- compact frameless Numi-like window;
- traffic-light title controls and centered title;
- monospace editor/result typography;
- dark Numi palette with yellow labels, blue units/keywords and green results.

Visual reference:

```text
https://camo.githubusercontent.com/49d6223fe0ad7af2d1991e9eb4ef9ea32ef3d20bd4c4801ba33481bcf4dcce43/68747470733a2f2f6e756d692e6170702f696d616765732f6e756d692d73637265656e73686f742d79656c6c6f772e706e67
```

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

Current local status:

- `cmake -S kde -B build/kde` passes;
- `cmake --build build/kde` passes;
- `QT_QPA_PLATFORM=offscreen ./build/kde/numi-kde` starts without QML runtime errors.

Fedora currently emits non-fatal CMake warnings about Kirigami QML plugin link targets during
configure. The binary still builds and uses runtime QML imports. This should be revisited before
release packaging.

## Current Limitations

The native skeleton does not yet call the JS core. It is a UI shell scaffold with static sample
content styled against the Numi visual reference. The next step is to connect it to the shared core
through a local adapter process or native backend bridge.

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
