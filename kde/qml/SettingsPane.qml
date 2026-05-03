import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 15

        Text {
            text: "Settings"
            color: Window.window.numiMuted
            font.pixelSize: 13
            font.weight: Font.DemiBold
            Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3d47"
        }

        Flickable {
            Layout.fillWidth: true
            Layout.fillHeight: true
            contentHeight: settingsColumn.height
            clip: true

            ColumnLayout {
                id: settingsColumn
                width: parent.width
                spacing: 15
                Layout.topMargin: 5

                Controls.CheckBox {
                    id: alwaysOnTopCheck
                    text: "Always on top"
                    checked: Window.window.alwaysOnTop
                    onToggled: Window.window.alwaysOnTop = checked

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 3
                        border.color: alwaysOnTopCheck.checked ? Window.window.numiBlue : "#6b6d76"
                        border.width: 1
                        color: alwaysOnTopCheck.checked ? Window.window.numiBlue : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: Window.window.numiWindow
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            visible: alwaysOnTopCheck.checked
                        }
                    }

                    contentItem: Text {
                        leftPadding: alwaysOnTopCheck.indicator.width + alwaysOnTopCheck.spacing
                        text: alwaysOnTopCheck.text
                        color: Window.window.numiText
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Controls.CheckBox {
                    id: autostartCheck
                    text: "Launch at login"
                    checked: documentModel ? documentModel.autostart : false
                    onToggled: if (documentModel) documentModel.autostart = checked

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 3
                        border.color: autostartCheck.checked ? Window.window.numiBlue : "#6b6d76"
                        border.width: 1
                        color: autostartCheck.checked ? Window.window.numiBlue : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: Window.window.numiWindow
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            visible: autostartCheck.checked
                        }
                    }

                    contentItem: Text {
                        leftPadding: autostartCheck.indicator.width + autostartCheck.spacing
                        text: autostartCheck.text
                        color: Window.window.numiText
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                Controls.CheckBox {
                    id: resultsSeparatorCheck
                    text: "Show result separator"
                    checked: Window.window.showResultsSeparator
                    onToggled: Window.window.showResultsSeparator = checked

                    indicator: Rectangle {
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 3
                        border.color: resultsSeparatorCheck.checked ? Window.window.numiBlue : "#6b6d76"
                        border.width: 1
                        color: resultsSeparatorCheck.checked ? Window.window.numiBlue : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: Window.window.numiWindow
                            font.pixelSize: 11
                            font.weight: Font.Bold
                            visible: resultsSeparatorCheck.checked
                        }
                    }

                    contentItem: Text {
                        leftPadding: resultsSeparatorCheck.indicator.width + resultsSeparatorCheck.spacing
                        text: resultsSeparatorCheck.text
                        color: Window.window.numiText
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: "Font size: " + Window.window.fontSize + " px"
                        color: Window.window.numiText
                        font.pixelSize: 13
                    }
                    Controls.Slider {
                        Layout.fillWidth: true
                        from: 11; to: 24; stepSize: 1
                        value: Window.window.fontSize
                        onMoved: Window.window.fontSize = Math.round(value)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: "Result column width: " + Window.window.resultWidth + " px"
                        color: Window.window.numiText
                        font.pixelSize: 13
                    }
                    Controls.Slider {
                        Layout.fillWidth: true
                        from: 96; to: 720; stepSize: 8
                        value: Window.window.resultWidth
                        onMoved: Window.window.resultWidth = Math.round(value)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 5
                    Text {
                        text: "Decimal places: " + Math.round(Window.window.decimalPlaces)
                        color: Window.window.numiText
                        font.pixelSize: 13
                    }
                    Controls.Slider {
                        Layout.fillWidth: true
                        from: 0; to: 10; stepSize: 1
                        value: Window.window.decimalPlaces
                        onMoved: Window.window.decimalPlaces = Math.round(value)
                    }
                }

                ColumnLayout {
                    id: hotkeyColumn
                    Layout.fillWidth: true
                    spacing: 5
                    property bool recordingHotkey: false

                    function keyName(key, text) {
                        if (key >= Qt.Key_0 && key <= Qt.Key_9) return String.fromCharCode("0".charCodeAt(0) + key - Qt.Key_0)
                        if (key >= Qt.Key_A && key <= Qt.Key_Z) return String.fromCharCode("A".charCodeAt(0) + key - Qt.Key_A)
                        if (key >= Qt.Key_F1 && key <= Qt.Key_F12) return "F" + (key - Qt.Key_F1 + 1)
                        if (key === Qt.Key_Space) return "Space"
                        if (text && text.length > 0) return text.toUpperCase()
                        return ""
                    }

                    Text {
                        text: "Global hotkey"
                        color: Window.window.numiText
                        font.pixelSize: 13
                    }

                    Controls.TextField {
                        id: hotkeyField
                        Layout.fillWidth: true
                        text: hotkeyColumn.recordingHotkey ? "Press shortcut..." : (shortcutManager ? shortcutManager.sequence : "Ctrl+Alt+1")
                        readOnly: true
                        focus: hotkeyColumn.recordingHotkey
                        color: Window.window.numiText
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
                            color: "#1d1f25"
                            border.color: "#343742"
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
            }
        }
    }
}
