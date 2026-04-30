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
    flags: Qt.Window | Qt.FramelessWindowHint

    readonly property color numiWindow: "#22242a"
    readonly property color numiTitle: "#5d5f69"
    readonly property color numiText: "#f0f0f3"
    readonly property color numiMuted: "#6b6d76"
    readonly property color numiYellow: "#ffd35a"
    readonly property color numiBlue: "#6fc4e8"
    readonly property color numiGreen: "#8fd14f"
    readonly property color numiRed: "#ff5f57"

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

            Row {
                anchors.left: parent.left
                anchors.leftMargin: 16
                anchors.verticalCenter: parent.verticalCenter
                spacing: 8

                Repeater {
                    model: ["#ff5f57", "#febc2e", "#28c840"]

                    Rectangle {
                        width: 13
                        height: 13
                        radius: 6.5
                        color: modelData
                    }
                }
            }

            Controls.Label {
                anchors.centerIn: parent
                text: "Numi"
                color: root.numiTitle
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }
        }

        DocumentPage {
            anchors.top: titleBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }
}
