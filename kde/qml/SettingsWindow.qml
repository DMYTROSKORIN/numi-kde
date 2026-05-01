import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import QtCore

Controls.Window {
    id: root
    title: qsTr("Settings")
    width: 320
    height: 420
    minimumWidth: 280
    minimumHeight: 380
    color: "#22242a"
    visible: false

    Settings {
        id: localSettings
        category: "Window"
        property int fontSize: 16
        property int resultWidth: 124
        property int decimalPlaces: 3
        property bool alwaysOnTop: true
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Text {
            text: qsTr("General Settings")
            color: "#f0f0f3"
            font.pixelSize: 16
            font.bold: true
        }

        Controls.CheckBox {
            id: alwaysOnTopCheck
            text: qsTr("Always on top")
            checked: localSettings.alwaysOnTop
            onToggled: localSettings.alwaysOnTop = checked
            
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

            Controls.ComboBox {
                Layout.fillWidth: true
                model: [9, 10, 11, 12, 13, 14, 16, 18, 20, 22, 24]
                currentIndex: model.indexOf(localSettings.fontSize)
                onActivated: (index) => localSettings.fontSize = model[index]
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
                value: localSettings.resultWidth
                onMoved: localSettings.resultWidth = value
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: qsTr("Decimal places")
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 0
                to: 10
                stepSize: 1
                value: localSettings.decimalPlaces
                onMoved: localSettings.decimalPlaces = value
            }

            Text {
                text: qsTr("Current: %1 (Default 3)").arg(Math.floor(localSettings.decimalPlaces))
                color: "#6b6d76"
                font.pixelSize: 11
            }
        }

        Item { Layout.fillHeight: true }

        Controls.Button {
            text: qsTr("Close")
            Layout.alignment: Qt.AlignRight
            onClicked: root.close()
            
            background: Rectangle {
                radius: 4
                color: parent.down ? "#3a3d45" : parent.hovered ? "#30333b" : "#2a2c34"
            }
            contentItem: Text {
                text: parent.text
                color: "#f0f0f3"
                horizontalAlignment: Text.AlignHCenter
            }
        }
    }
}
