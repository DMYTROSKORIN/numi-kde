# Native KDE Prototype

The native KDE prototype lives in `kde/`.

It is the starting point for the final Qt 6/KF6 application. The current skeleton already
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
- Linux/KDE title controls and centered title;
- monospace editor/result typography;
- dark Numi palette with yellow labels, blue units/keywords and green results.

Visual reference:

```text
https://camo.githubusercontent.com/49d6223fe0ad7af2d1991e9eb4ef9ea32ef3d20bd4c4801ba33481bcf4dcce43/68747470733a2f2f6e756d692e6170702f696d616765732f6e756d692d73637265656e73686f742d79656c6c6f772e706e67
```

## Fedora Dependencies

On Fedora KDE, install the missing development packages:

```sh
sudo dnf install -y qt6-qtbase-devel qt6-qtdeclarative-devel
```

These provide the missing CMake packages:

- `Qt6`
- `Qt6Qml`
- `Qt6Quick`
- `Qt6QuickControls2`

In the current workspace, package discovery confirmed:

- `qt6-qtbase-devel` provides `cmake(Qt6)`;
- `qt6-qtdeclarative-devel` provides `cmake(Qt6Qml)` and `cmake(Qt6Quick)`;

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
- `./build/kde/numi-kde` starts in the graphical session without stderr output;

Current native behavior:

- the left editor is active and editable;
- the right result column is driven by the shared evaluator;
- editing text triggers live recalculation;
- result rows highlight on pointer hover;
- clicking a result copies its formatted value to the clipboard;
- syntax highlighting marks units/currency-like tokens, `%` and natural operators;
- the window is resizable from all edges and corners;
- resized geometry is persisted for the next launch;
- the window stays above other windows by default;
- top-left action is reserved for history;
- top-right actions are minimize, maximize/restore and close;
- bottom-left action is reserved for settings;
- bottom-right arrow is intentionally removed.

The first runnable native prototype deliberately avoids direct Kirigami imports to keep configure
and launch clean on Fedora. It still uses Qt Quick Controls and requests the KDE desktop style in
`main.cpp`. Kirigami components can be reintroduced when settings pages, drawers or navigation need
them.

## Current Limitations

The native prototype currently calls the JS core through a local Node worker process. This is good
for preserving one tested evaluator while the UI takes shape, but the final Linux package must
either embed that runtime cleanly or replace the bridge with a native backend.

The syntax highlighting layer is functional but still early. It highlights semantic token classes;
the next editor pass must tighten caret alignment, line-height parity, selection behavior and
large-document performance.

## Next Work

- Polish editor rendering and caret/highlight alignment.
- Add font size and result column width settings.
- Add history and settings panels.
- Add native screenshot checks once the binary builds.
