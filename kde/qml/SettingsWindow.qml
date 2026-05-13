import QtQuick
import QtQuick.Controls as Controls
import QtCore

Controls.ApplicationWindow {
    id: settingsWin
    title: qsTr("Numi-KDE Settings")
    flags: Qt.Window

    required property var appWindow

    // Minimum = content natural size + margins; user can make it larger but not smaller
    minimumWidth:  pane.implicitWidth  + 2 * contentMargin
    minimumHeight: pane.implicitHeight + 2 * contentMargin

    readonly property int contentMargin: 12

    Settings {
        id: winState
        category: "SettingsWindow"
        property int savedX: -1
        property int savedY: -1
    }

    Component.onCompleted: {
        // Always open at content size — no saved dimensions
        width  = minimumWidth
        height = minimumHeight
        if (winState.savedX >= 0) x = winState.savedX
        if (winState.savedY >= 0) y = winState.savedY
    }

    onXChanged: if (visible) winState.savedX = x
    onYChanged: if (visible) winState.savedY = y

    onClosing: (close) => { close.accepted = false; hide() }

    function open() { show(); raise(); requestActivate() }

    SettingsPane {
        id: pane
        anchors.fill: parent
        anchors.margins: settingsWin.contentMargin
        appWindow: settingsWin.appWindow
    }
}
