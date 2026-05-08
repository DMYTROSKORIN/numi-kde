import QtQuick
import QtQuick.Controls as Controls
import QtCore

Controls.ApplicationWindow {
    id: root

    width: windowSettings.savedWidth
    height: windowSettings.savedHeight
    minimumWidth: 420
    minimumHeight: 320
    visible: false
    title: qsTr("Numi-KDE")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint

    property bool alwaysOnTop: windowSettings.alwaysOnTop
    onAlwaysOnTopChanged: {
        windowSettings.alwaysOnTop = alwaysOnTop
        // We DO NOT change root.flags here. Changing flags causes the window
        // to recreate and jump to the center of the screen.
        // The C++ DocumentModel::setKeepAbove handles the actual window state.
        if (typeof documentModel !== "undefined") {
            Qt.callLater(() => documentModel.setKeepAbove(alwaysOnTop))
        }
    }

    property int fontSize: windowSettings.fontSize
    onFontSizeChanged: windowSettings.fontSize = fontSize

    property int resultWidth: windowSettings.resultWidth
    onResultWidthChanged: windowSettings.resultWidth = resultWidth

    property int decimalPlaces: windowSettings.decimalPlaces
    onDecimalPlacesChanged: {
        windowSettings.decimalPlaces = decimalPlaces
        if (typeof documentModel !== "undefined") {
            documentModel.decimalPlaces = decimalPlaces
        }
    }

    property bool showResultsSeparator: windowSettings.showResultsSeparator
    onShowResultsSeparatorChanged: windowSettings.showResultsSeparator = showResultsSeparator

    // Only save x/y after user-initiated movement or resize. Compositor remap
    // placement must never overwrite the last user position.
    property bool positionSaveEnabled: false

    readonly property color numiWindow: "#22242a"
    readonly property color numiTitle: "#5d5f69"
    readonly property color numiText: "#f0f0f3"
    readonly property color numiMuted: "#6b6d76"
    readonly property color numiYellow: "#ffd35a"
    readonly property color numiBlue: "#6fc4e8"
    readonly property color numiGreen: "#8fd14f"
    readonly property color numiRed: "#ff5f57"
    readonly property color controlHover: "#30333b"
    readonly property color controlPressed: "#3a3d45"

    Settings {
        id: windowSettings
        category: "Window"
        property int savedWidth: 456
        property int savedHeight: 368
        property int savedX: -1
        property int savedY: -1
        property bool alwaysOnTop: true
        property int fontSize: 16
        property int resultWidth: 124
        property int decimalPlaces: 3
        property bool showResultsSeparator: true
    }

    function restoreSavedPosition() {
        if (typeof documentModel !== "undefined" && documentModel.isWayland) {
            return
        }
        if (windowSettings.savedX >= 0) {
            root.x = windowSettings.savedX
        }
        if (windowSettings.savedY >= 0) {
            root.y = windowSettings.savedY
        }
    }

    function hideWindow() {
        restorePositionTimer.stop()
        positionSaveEnabled = false
        root.hide()
    }

    function armPositionSave() {
        if (visible) {
            positionSaveEnabled = true
        }
    }

    Timer {
        id: restorePositionTimer
        interval: 75
        repeat: false
        onTriggered: root.restoreSavedPosition()
    }

    Component.onCompleted: {
        if (typeof documentModel !== "undefined") {
            documentModel.decimalPlaces = decimalPlaces
        }
        restoreSavedPosition()
    }

    // Re-apply skip-taskbar and keep-above whenever the window becomes visible
    // (X11 EWMH states are reset on each unmap/remap).
    onVisibleChanged: {
        if (visible) {
            positionSaveEnabled = false
            restoreSavedPosition()
            restorePositionTimer.restart()
            if (typeof documentModel !== "undefined") {
                Qt.callLater(() => documentModel.setKeepAbove(alwaysOnTop))
            }
        } else {
            restorePositionTimer.stop()
            positionSaveEnabled = false
        }
    }

    onWidthChanged: windowSettings.savedWidth = width
    onHeightChanged: windowSettings.savedHeight = height
    onXChanged: {
        if (typeof documentModel !== "undefined" && documentModel.isWayland) return
        if (positionSaveEnabled && visible) windowSettings.savedX = x
    }
    onYChanged: {
        if (typeof documentModel !== "undefined" && documentModel.isWayland) return
        if (positionSaveEnabled && visible) windowSettings.savedY = y
    }

    onClosing: (close) => {
        close.accepted = false
        hideWindow()
    }

    background: Rectangle {
        color: "transparent"
    }

    Rectangle {
        id: windowSurface

        anchors.fill: parent
        radius: 7
        color: root.numiWindow
        border.color: "#1a1b20"
        border.width: 1

        layer.enabled: true

        Rectangle {
            id: titleBar

            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 45
            radius: windowSurface.radius
            color: root.numiWindow

            MouseArea {
                anchors.fill: parent
                onDoubleClicked: root.visibility = root.visibility === Window.Maximized ? Window.Windowed : Window.Maximized
                onPressed: {
                    root.armPositionSave()
                    root.startSystemMove()
                }
            }

            Controls.Button {
                id: historyButton

                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                width: 30
                height: 30
                padding: 0
                text: "↺"
                font.pixelSize: 20
                hoverEnabled: true

                onClicked: {
                    if (typeof documentModel !== "undefined") {
                        documentModel.saveSession()
                    }
                    historyDrawer.open()
                }

                contentItem: Text {
                    text: historyButton.text
                    color: root.numiMuted
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: historyButton.font
                }

                background: Rectangle {
                    radius: 5
                    color: historyButton.down ? root.controlPressed : historyButton.hovered ? root.controlHover : "transparent"
                }
            }

            Controls.Label {
                anchors.centerIn: parent
                text: "Numi-KDE"
                color: root.numiTitle
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Rectangle {
                id: networkStatusIndicator
                anchors.left: parent.left
                anchors.leftMargin: 56
                anchors.verticalCenter: parent.verticalCenter
                width: 8
                height: 8
                radius: 4
                color: {
                    if (typeof documentModel === "undefined") return "transparent"
                    switch (documentModel.networkStatus) {
                        case 1: return root.numiYellow  // Fetching
                        case 2: return root.numiGreen   // Success
                        case 3: return root.numiRed     // Error
                        default: return "transparent"   // Idle
                    }
                }
                opacity: color === "transparent" ? 0 : 0.8
                
                Controls.ToolTip.visible: statusMouse.containsMouse && documentModel.networkStatus !== 0
                Controls.ToolTip.text: {
                    switch (documentModel.networkStatus) {
                        case 1: return qsTr("Updating rates...")
                        case 2: return qsTr("Rates up to date")
                        case 3: return qsTr("Rate update failed")
                        default: return ""
                    }
                }

                MouseArea {
                    id: statusMouse
                    anchors.fill: parent
                    hoverEnabled: true
                }

                Behavior on color { ColorAnimation { duration: 200 } }
                Behavior on opacity { NumberAnimation { duration: 200 } }
            }

            Row {
                anchors.right: parent.right
                anchors.rightMargin: 12
                anchors.verticalCenter: parent.verticalCenter
                spacing: 4

                WindowButton {
                    text: "–"
                    color: root.numiMuted
                    hoverColor: root.controlHover
                    pressedColor: root.controlPressed
                    onClicked: root.hideWindow()
                }

                WindowButton {
                    text: "□"
                    color: root.numiMuted
                    hoverColor: root.controlHover
                    pressedColor: root.controlPressed
                    onClicked: root.visibility = root.visibility === Window.Maximized ? Window.Windowed : Window.Maximized
                }

                WindowButton {
                    text: "×"
                    color: root.numiMuted
                    hoverColor: "#5c2a2d"
                    pressedColor: "#743236"
                    onClicked: root.hideWindow()
                }
            }
        }

        DocumentPage {
            id: documentPage
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            settingsWindow: settingsWindow
            showResultsSeparator: root.showResultsSeparator
        }

        ResizeHandle {
            edge: Qt.TopEdge
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 7
            cursorShape: Qt.SizeVerCursor
        }

        ResizeHandle {
            edge: Qt.BottomEdge
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 7
            cursorShape: Qt.SizeVerCursor
        }

        ResizeHandle {
            edge: Qt.LeftEdge
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 7
            cursorShape: Qt.SizeHorCursor
        }

        ResizeHandle {
            edge: Qt.RightEdge
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 7
            cursorShape: Qt.SizeHorCursor
        }

        ResizeHandle {
            edge: Qt.TopEdge | Qt.LeftEdge
            anchors.top: parent.top
            anchors.left: parent.left
            width: 14
            height: 14
            cursorShape: Qt.SizeFDiagCursor
        }

        ResizeHandle {
            edge: Qt.TopEdge | Qt.RightEdge
            anchors.top: parent.top
            anchors.right: parent.right
            width: 14
            height: 14
            cursorShape: Qt.SizeBDiagCursor
        }

        ResizeHandle {
            edge: Qt.BottomEdge | Qt.LeftEdge
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            width: 14
            height: 14
            cursorShape: Qt.SizeBDiagCursor
        }

        ResizeHandle {
            edge: Qt.BottomEdge | Qt.RightEdge
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 14
            height: 14
            cursorShape: Qt.SizeFDiagCursor
        }

        component WindowButton: Controls.Button {
            property color color: "#6b6d76"
            property color hoverColor: "#30333b"
            property color pressedColor: "#3a3d45"

            width: 34
            height: 30
            padding: 0
            hoverEnabled: true

            contentItem: Text {
                text: parent.text
                color: parent.color
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 18
                font.weight: Font.DemiBold
            }

            background: Rectangle {
                radius: 5
                color: parent.down ? parent.pressedColor : parent.hovered ? parent.hoverColor : "transparent"
            }
        }

        component ResizeHandle: MouseArea {
            required property int edge

            hoverEnabled: true
            acceptedButtons: Qt.LeftButton
            z: 10
            onPressed: {
                root.armPositionSave()
                root.startSystemResize(edge)
            }
        }
    }

    Controls.Drawer {
        id: settingsDrawer
        width: Math.min(root.width, 380)
        height: root.height
        edge: Qt.LeftEdge
        dragMargin: 0

        background: Rectangle {
            color: root.numiWindow
            border.color: "#3a3d47"
            border.width: 1
        }

        contentItem: SettingsPane {
            id: settingsPane
        }
    }

    Controls.Drawer {
        id: historyDrawer
        width: 260
        height: root.height
        edge: Qt.LeftEdge
        z: 100
        background: Rectangle {
            color: "#2d303a"
            border.color: "#3a3d47"
            border.width: 1
        }

        HistoryPane {
            anchors.fill: parent
            anchors.margins: 10
            onSessionSelected: historyDrawer.close()
        }
    }
}
