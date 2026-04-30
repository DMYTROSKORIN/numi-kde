import QtQuick
import QtQuick.Controls as Controls
import QtCore

Controls.ApplicationWindow {
    id: root

    width: windowSettings.savedWidth
    height: windowSettings.savedHeight
    minimumWidth: 420
    minimumHeight: 320
    visible: true
    title: qsTr("Numi")
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | (alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)

    property bool alwaysOnTop: windowSettings.alwaysOnTop
    onAlwaysOnTopChanged: {
        windowSettings.alwaysOnTop = alwaysOnTop
        root.flags = Qt.Window | Qt.FramelessWindowHint | (alwaysOnTop ? Qt.WindowStaysOnTopHint : 0)
    }

    property int fontSize: windowSettings.fontSize
    onFontSizeChanged: windowSettings.fontSize = fontSize

    property int resultWidth: windowSettings.resultWidth
    onResultWidthChanged: windowSettings.resultWidth = resultWidth

    property int decimalPlaces: windowSettings.decimalPlaces
    onDecimalPlacesChanged: windowSettings.decimalPlaces = decimalPlaces

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
    }

    Component.onCompleted: {
        if (windowSettings.savedX >= 0 && windowSettings.savedY >= 0) {
            root.x = windowSettings.savedX
            root.y = windowSettings.savedY
        }
    }

    onWidthChanged: windowSettings.savedWidth = width
    onHeightChanged: windowSettings.savedHeight = height
    onXChanged: windowSettings.savedX = x
    onYChanged: windowSettings.savedY = y

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
                onPressed: root.startSystemMove()
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
                text: "Numi"
                color: root.numiTitle
                font.pixelSize: 14
                font.weight: Font.DemiBold
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
                    onClicked: root.showMinimized()
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
                    onClicked: Qt.quit()
                }
            }
        }

        DocumentPage {
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
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
            onPressed: root.startSystemResize(edge)
        }
    }
}
