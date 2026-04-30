import QtQuick
import QtQuick.Controls as Controls

Controls.ApplicationWindow {
    id: root

    width: 456
    height: 368
    minimumWidth: 420
    minimumHeight: 320
    visible: true
    title: "Numi"
    color: "transparent"
    flags: Qt.Window | Qt.FramelessWindowHint | Qt.WindowStaysOnTopHint

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
    }
}
