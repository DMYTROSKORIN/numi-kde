import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Controls.ApplicationWindow {
    id: root
    title: qsTr("Settings")
    width: 320
    height: 380
    minimumWidth: 280
    minimumHeight: 340
    color: "#22242a"

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 20

        Text {
            text: qsTr("General Settings")
            color: "#f0f0f3"
            font.pixelSize: 16
            font.bold: true
        }

        Controls.CheckBox {
            id: alwaysOnTopCheck
            text: qsTr("Always on top")
            checked: Window.window ? !!Window.window.alwaysOnTop : true
            onToggled: {
                if (Window.window) {
                    Window.window.alwaysOnTop = checked
                }
            }
            contentItem: Text {
                leftPadding: alwaysOnTopCheck.indicator.width + alwaysOnTopCheck.spacing
                text: alwaysOnTopCheck.text
                color: "#f0f0f3"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: qsTr("Font size")
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 11
                to: 24
                stepSize: 1
                value: Window.window ? Window.window.fontSize : 16
                onMoved: if (Window.window) Window.window.fontSize = value
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: qsTr("Result width")
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 80
                to: 300
                stepSize: 8
                value: Window.window ? Window.window.resultWidth : 124
                onMoved: if (Window.window) Window.window.resultWidth = value
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: qsTr("Number of decimal places")
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 0
                to: 10
                stepSize: 1
                value: Window.window ? Window.window.decimalPlaces : 3
                onMoved: if (Window.window) Window.window.decimalPlaces = value
            }

            Text {
                text: qsTr("Default is 3")
                color: "#6b6d76"
                font.pixelSize: 11
            }
        }

        Item { Layout.fillHeight: true }

        Controls.Button {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignRight
            onClicked: root.close()
        }
    }
}
