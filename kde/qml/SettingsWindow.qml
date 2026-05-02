import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Window {
    id: root

    property Window mainWindow: null

    title: "Settings"
    width: 320
    height: 500
    minimumWidth: 280
    minimumHeight: 460
    color: mainWindow ? mainWindow.numiWindow : "#22242a"
    flags: Qt.Dialog

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 20
        spacing: 15

        Text {
            text: "Numi-KDE — Settings"
            color: mainWindow ? mainWindow.numiText : "#f0f0f3"
            font.pixelSize: 14
            font.bold: true
        }

        Controls.CheckBox {
            id: lightThemeCheck

            text: "Light theme"
            checked: mainWindow ? mainWindow.lightTheme : false
            onToggled: {
                if (mainWindow) mainWindow.lightTheme = checked
            }

            indicator: Rectangle {
                implicitWidth: 16
                implicitHeight: 16
                x: lightThemeCheck.leftPadding
                y: (lightThemeCheck.height - height) / 2
                radius: 3
                border.color: lightThemeCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "#6b6d76"
                border.width: 1
                color: lightThemeCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: mainWindow ? mainWindow.numiWindow : "#22242a"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    visible: lightThemeCheck.checked
                }
            }

            contentItem: Text {
                leftPadding: lightThemeCheck.indicator.width + lightThemeCheck.spacing
                text: lightThemeCheck.text
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
        }

        Controls.CheckBox {
            id: alwaysOnTopCheck

            text: "Always on top"
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
                border.color: alwaysOnTopCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "#6b6d76"
                border.width: 1
                color: alwaysOnTopCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: mainWindow ? mainWindow.numiWindow : "#22242a"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    visible: alwaysOnTopCheck.checked
                }
            }

            contentItem: Text {
                leftPadding: alwaysOnTopCheck.indicator.width + alwaysOnTopCheck.spacing
                text: alwaysOnTopCheck.text
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
        }

        Controls.CheckBox {
            id: autostartCheck

            text: "Launch at login"
            checked: documentModel ? documentModel.autostart : false
            onToggled: {
                if (documentModel) documentModel.autostart = checked
            }

            indicator: Rectangle {
                implicitWidth: 16
                implicitHeight: 16
                x: autostartCheck.leftPadding
                y: (autostartCheck.height - height) / 2
                radius: 3
                border.color: autostartCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "#6b6d76"
                border.width: 1
                color: autostartCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: mainWindow ? mainWindow.numiWindow : "#22242a"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    visible: autostartCheck.checked
                }
            }

            contentItem: Text {
                leftPadding: autostartCheck.indicator.width + autostartCheck.spacing
                text: autostartCheck.text
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
        }

        Controls.CheckBox {
            id: resultsSeparatorCheck

            text: "Show result separator"
            checked: mainWindow ? mainWindow.showResultsSeparator : true
            onToggled: {
                if (mainWindow) mainWindow.showResultsSeparator = checked
            }

            indicator: Rectangle {
                implicitWidth: 16
                implicitHeight: 16
                x: resultsSeparatorCheck.leftPadding
                y: (resultsSeparatorCheck.height - height) / 2
                radius: 3
                border.color: resultsSeparatorCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "#6b6d76"
                border.width: 1
                color: resultsSeparatorCheck.checked ? (mainWindow ? mainWindow.numiBlue : "#6fc4e8") : "transparent"

                Text {
                    anchors.centerIn: parent
                    text: "✓"
                    color: mainWindow ? mainWindow.numiWindow : "#22242a"
                    font.pixelSize: 11
                    font.weight: Font.Bold
                    visible: resultsSeparatorCheck.checked
                }
            }

            contentItem: Text {
                leftPadding: resultsSeparatorCheck.indicator.width + resultsSeparatorCheck.spacing
                text: resultsSeparatorCheck.text
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                font.pixelSize: 13
                verticalAlignment: Text.AlignVCenter
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 5

            Text {
                text: "Font size: " + (mainWindow ? mainWindow.fontSize : 16) + " px"
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
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
                text: "Result column width: " + (mainWindow ? mainWindow.resultWidth : 124) + " px"
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.Slider {
                Layout.fillWidth: true
                from: 96
                to: 720
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
                text: "Decimal places: " + (mainWindow ? Math.round(mainWindow.decimalPlaces) : 3)
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
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

        ColumnLayout {
            id: hotkeyColumn

            Layout.fillWidth: true
            spacing: 5

            property bool recordingHotkey: false

            function keyName(key, text) {
                if (key >= Qt.Key_0 && key <= Qt.Key_9) {
                    return String.fromCharCode("0".charCodeAt(0) + key - Qt.Key_0)
                }
                if (key >= Qt.Key_A && key <= Qt.Key_Z) {
                    return String.fromCharCode("A".charCodeAt(0) + key - Qt.Key_A)
                }
                if (key >= Qt.Key_F1 && key <= Qt.Key_F12) {
                    return "F" + (key - Qt.Key_F1 + 1)
                }
                if (key === Qt.Key_Space) return "Space"
                if (text && text.length > 0) return text.toUpperCase()
                return ""
            }

            Text {
                text: "Global hotkey"
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                font.pixelSize: 13
            }

            Controls.TextField {
                id: hotkeyField

                Layout.fillWidth: true
                text: hotkeyColumn.recordingHotkey ? "Press shortcut..." : (shortcutManager ? shortcutManager.sequence : "Ctrl+Alt+1")
                readOnly: true
                focus: hotkeyColumn.recordingHotkey
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                selectedTextColor: "#22242a"
                selectionColor: "#6fc4e8"
                font.pixelSize: 13
                activeFocusOnPress: true

                Keys.onPressed: (event) => {
                    if (!hotkeyColumn.recordingHotkey) return
                    if (event.key === Qt.Key_Escape) {
                        hotkeyColumn.recordingHotkey = false
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Control || event.key === Qt.Key_Alt ||
                        event.key === Qt.Key_Shift || event.key === Qt.Key_Meta) {
                        event.accepted = true
                        return
                    }

                    let parts = []
                    if (event.modifiers & Qt.ControlModifier) parts.push("Ctrl")
                    if (event.modifiers & Qt.AltModifier) parts.push("Alt")
                    if (event.modifiers & Qt.ShiftModifier) parts.push("Shift")
                    if (event.modifiers & Qt.MetaModifier) parts.push("Meta")

                    let key = hotkeyColumn.keyName(event.key, event.text)
                    if (key.length > 0 && parts.length > 0 && shortcutManager) {
                        parts.push(key)
                        shortcutManager.sequence = parts.join("+")
                        hotkeyColumn.recordingHotkey = false
                    }
                    event.accepted = true
                }

                TapHandler {
                    onTapped: {
                        hotkeyColumn.recordingHotkey = true
                        hotkeyField.forceActiveFocus()
                    }
                }

                background: Rectangle {
                    radius: 4
                    color: mainWindow ? (mainWindow.lightTheme ? "#e4d9b6" : "#1d1f25") : "#1d1f25"
                    border.color: mainWindow ? (mainWindow.lightTheme ? "#d1c499" : "#343742") : "#343742"
                    border.width: 1
                }
            }

            Text {
                Layout.fillWidth: true
                visible: shortcutManager && shortcutManager.status.length > 0
                text: shortcutManager ? shortcutManager.status : ""
                color: text === "Shortcut saved" ? "#8fd14f" : "#ff5f57"
                font.pixelSize: 11
                elide: Text.ElideRight
            }
        }

        Item { Layout.fillHeight: true }

        Controls.Button {
            text: "Close"
            Layout.alignment: Qt.AlignRight

            onClicked: root.close()

            background: Rectangle {
                radius: 4
                color: parent.down ? (mainWindow ? mainWindow.controlPressed : "#3a3d45") : parent.hovered ? (mainWindow ? mainWindow.controlHover : "#30333b") : (mainWindow ? mainWindow.numiWindow : "#2a2c34")
                border.color: mainWindow ? (mainWindow.lightTheme ? "#d1c499" : "#1a1b20") : "#1a1b20"
                border.width: 1
            }

            contentItem: Text {
                text: parent.text
                color: mainWindow ? mainWindow.numiText : "#f0f0f3"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 13
            }
        }
    }
}
