import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Window {
    id: root

    property Window mainWindow: null

    title: "Настройки"
    width: 320
    height: 400
    minimumWidth: 280
    minimumHeight: 360
    color: "#22242a"
    flags: Qt.Dialog

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Text {
            text: "Numi-KDE — Настройки"
            color: "#f0f0f3"
            font.pixelSize: 14
            font.bold: true
        }

        Controls.CheckBox {
            id: alwaysOnTopCheck

            text: "Всегда поверх окон"
            checked: mainWindow ? mainWindow.alwaysOnTop : true
            onToggled: {
                if (mainWindow) mainWindow.alwaysOnTop = checked
            }

            indicator: Rectangle {
                implicitWidth: 16
                implicitHeight: 16
                x: alwaysOnTopCheck.leftPadding
                y: (alwaysOnTopCheck.height - height) / 2
                radius: 3
                border.color: alwaysOnTopCheck.checked ? "#6fc4e8" : "#6b6d76"
                border.width: 1
                color: alwaysOnTopCheck.checked ? "#6fc4e8" : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: "#22242a"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    visible: alwaysOnTopCheck.checked
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
                text: "Размер шрифта: " + (mainWindow ? mainWindow.fontSize : 16) + " px"
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 11
                to: 24
                stepSize: 1
                value: mainWindow ? mainWindow.fontSize : 16
                onMoved: {
                    if (mainWindow) mainWindow.fontSize = Math.round(value)
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: "Ширина колонки результатов: " + (mainWindow ? mainWindow.resultWidth : 124) + " px"
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 80
                to: 300
                stepSize: 8
                value: mainWindow ? mainWindow.resultWidth : 124
                onMoved: {
                    if (mainWindow) mainWindow.resultWidth = Math.round(value)
                }
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: "Знаков после запятой: " + (mainWindow ? Math.round(mainWindow.decimalPlaces) : 3)
                color: "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 0
                to: 10
                stepSize: 1
                value: mainWindow ? mainWindow.decimalPlaces : 3
                onMoved: {
                    if (mainWindow) mainWindow.decimalPlaces = Math.round(value)
                }
            }
        }

        Item { Layout.fillHeight: true }

        Controls.Button {
            text: "Закрыть"
            Layout.alignment: Qt.AlignRight

            onClicked: root.close()

            background: Rectangle {
                radius: 4
                color: parent.down ? "#3a3d45" : parent.hovered ? "#30333b" : "#2a2c34"
                border.color: "#1a1b20"
                border.width: 1
            }

            contentItem: Text {
                text: parent.text
                color: "#f0f0f3"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
        }
    }
}
