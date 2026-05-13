import QtQuick
import QtQuick.Controls as Controls
import QtCore

Controls.ApplicationWindow {
    id: settingsWin
    title: qsTr("Numi-KDE Settings")
    minimumWidth: 280
    minimumHeight: 360
    flags: Qt.Window

    required property var appWindow

    Settings {
        id: winState
        category: "SettingsWindow"
        property int savedWidth: 300
        property int savedHeight: 520
        property int savedX: -1
        property int savedY: -1
    }

    Component.onCompleted: {
        width  = winState.savedWidth
        height = winState.savedHeight
        if (winState.savedX >= 0) x = winState.savedX
        if (winState.savedY >= 0) y = winState.savedY
    }

    onWidthChanged:  if (visible) winState.savedWidth  = width
    onHeightChanged: if (visible) winState.savedHeight = height
    onXChanged:      if (visible) winState.savedX = x
    onYChanged:      if (visible) winState.savedY = y

    onClosing: (close) => { close.accepted = false; hide() }

    function open() { show(); raise(); requestActivate() }

    SettingsPane {
        anchors.fill: parent
        anchors.margins: 12
        appWindow: settingsWin.appWindow
    }
}
