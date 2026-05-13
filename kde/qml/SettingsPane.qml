import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 24
            Layout.bottomMargin: 6

            Text {
                text: "Settings"
                color: Window.window ? Window.window.numiMuted : "#6b6d76"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3d47"
            Layout.bottomMargin: 8
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            Controls.ScrollBar.vertical.policy: Controls.ScrollBar.AlwaysOff
            Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff

            ColumnLayout {
                id: settingsColumn
                width: parent.width
                spacing: 12

                // Helper component for CheckBox styling
                component NumiCheckBox: Controls.CheckBox {
                    id: cb
                    indicator: Rectangle {
                        implicitWidth: 18
                        implicitHeight: 18
                        radius: 4
                        border.color: cb.checked ? (Window.window ? Window.window.numiBlue : "#6fc4e8") : "#4a4d56"
                        border.width: 1
                        color: cb.checked ? (Window.window ? Window.window.numiBlue : "#6fc4e8") : "transparent"
                        Text {
                            anchors.centerIn: parent
                            text: "✓"
                            color: Window.window ? Window.window.numiWindow : "#22242a"
                            font.pixelSize: 12
                            font.weight: Font.Bold
                            visible: cb.checked
                        }
                    }
                    contentItem: Text {
                        leftPadding: cb.indicator.width + cb.spacing
                        text: cb.text
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                        verticalAlignment: Text.AlignVCenter
                    }
                }

                // Helper component for compact numeric input
                component NumiSpinField: Controls.TextField {
                    id: spinField
                    property int minVal: 0
                    property int maxVal: 100
                    Layout.preferredWidth: 52
                    Layout.preferredHeight: 28
                    horizontalAlignment: Text.AlignHCenter
                    validator: IntValidator { bottom: spinField.minVal; top: spinField.maxVal }
                    color: Window.window ? Window.window.numiText : "#f0f0f3"
                    selectedTextColor: "#22242a"
                    selectionColor: "#6fc4e8"
                    font.pixelSize: 13
                    background: Rectangle {
                        radius: 4
                        color: "#1d1f25"
                        border.color: spinField.activeFocus
                                      ? (Window.window ? Window.window.numiBlue : "#6fc4e8")
                                      : "#343742"
                        border.width: 1
                    }
                }

                NumiCheckBox {
                    text: "Always on top"
                    checked: Window.window ? Window.window.alwaysOnTop : true
                    onToggled: if (Window.window) Window.window.alwaysOnTop = checked
                }

                NumiCheckBox {
                    text: "Launch at login"
                    checked: documentModel ? documentModel.autostart : false
                    onToggled: if (documentModel) documentModel.autostart = checked
                }

                NumiCheckBox {
                    text: "Show result separator"
                    checked: Window.window ? Window.window.showResultsSeparator : true
                    onToggled: if (Window.window) Window.window.showResultsSeparator = checked
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Font size"
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    NumiSpinField {
                        minVal: 11; maxVal: 24
                        Layout.preferredWidth: 84
                        text: Window.window ? Window.window.fontSize.toString() : "16"
                        onEditingFinished: {
                            let v = Math.max(minVal, Math.min(maxVal, parseInt(text) || minVal))
                            if (Window.window) Window.window.fontSize = v
                            text = v.toString()
                        }
                    }
                    Text {
                        text: "px  (11 – 24)"
                        width: 84
                        color: Window.window ? Window.window.numiMuted : "#6b6d76"
                        font.pixelSize: 11
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Result width"
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    NumiSpinField {
                        minVal: 80; maxVal: 720
                        Layout.preferredWidth: 84
                        text: Window.window ? Window.window.resultWidth.toString() : "124"
                        onEditingFinished: {
                            let v = Math.max(minVal, Math.min(maxVal, parseInt(text) || minVal))
                            if (Window.window) Window.window.resultWidth = v
                            text = v.toString()
                        }
                    }
                    Text {
                        text: "px  (80 – 720)"
                        width: 84
                        color: Window.window ? Window.window.numiMuted : "#6b6d76"
                        font.pixelSize: 11
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Decimal places"
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    NumiSpinField {
                        minVal: 0; maxVal: 10
                        Layout.preferredWidth: 84
                        text: Window.window ? Window.window.decimalPlaces.toString() : "3"
                        onEditingFinished: {
                            let v = Math.max(minVal, Math.min(maxVal, parseInt(text) || 0))
                            if (Window.window) Window.window.decimalPlaces = v
                            text = v.toString()
                        }
                    }
                    Text {
                        text: "(0 – 10)"
                        width: 84
                        color: Window.window ? Window.window.numiMuted : "#6b6d76"
                        font.pixelSize: 11
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6
                    Text {
                        text: "Default currency"
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    Controls.TextField {
                        id: defaultCurrencyField
                        Layout.preferredWidth: 72
                        Layout.preferredHeight: 28
                        text: documentModel ? documentModel.defaultCurrency : "USD"
                        maximumLength: 5
                        validator: RegularExpressionValidator { regularExpression: /[A-Za-z]{0,5}/ }
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        selectedTextColor: "#22242a"
                        selectionColor: "#6fc4e8"
                        font.pixelSize: 13
                        placeholderText: "USD"
                        placeholderTextColor: Window.window ? Window.window.numiMuted : "#6b6d76"
                        onEditingFinished: {
                            if (documentModel)
                                documentModel.defaultCurrency = text.toUpperCase()
                            text = documentModel ? documentModel.defaultCurrency : "USD"
                        }
                        background: Rectangle {
                            radius: 4
                            color: "#1d1f25"
                            border.color: defaultCurrencyField.activeFocus
                                          ? (Window.window ? Window.window.numiBlue : "#6fc4e8")
                                          : "#343742"
                            border.width: 1
                        }
                    }
                    Text {
                        text: "output for mixed crypto"
                        width: 72
                        color: Window.window ? Window.window.numiMuted : "#6b6d76"
                        font.pixelSize: 11
                        wrapMode: Text.WordWrap
                    }
                }

                ColumnLayout {
                    id: hotkeyColumn
                    Layout.fillWidth: true
                    spacing: 8
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
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }

                    Controls.TextField {
                        id: hotkeyField
                        Layout.preferredWidth: 140
                        Layout.preferredHeight: 28
                        text: hotkeyColumn.recordingHotkey ? "Press shortcut..." : (shortcutManager ? shortcutManager.sequence : "Ctrl+Alt+1")
                        readOnly: true
                        focus: hotkeyColumn.recordingHotkey
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
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
                            border.color: hotkeyField.activeFocus ? (Window.window ? Window.window.numiBlue : "#6fc4e8") : "#343742"
                            border.width: 1
                        }
                    }

                    Text {
                        text: hotkeyColumn.recordingHotkey ? "press combo, Esc to cancel" : "click to record"
                        width: 140
                        color: Window.window ? Window.window.numiMuted : "#6b6d76"
                        font.pixelSize: 11
                    }

                    Text {
                        Layout.preferredWidth: 140
                        visible: shortcutManager && shortcutManager.status.length > 0
                        text: shortcutManager ? shortcutManager.status : ""
                        color: text === "Shortcut saved" ? (Window.window ? Window.window.numiGreen : "#8fd14f") : (Window.window ? Window.window.numiRed : "#ff5f57")
                        font.pixelSize: 11
                        elide: Text.ElideRight
                    }
                }
            }
        }

        // Version info at the bottom
        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3d47"
            Layout.topMargin: 8
            Layout.bottomMargin: 6
        }

        Text {
            Layout.fillWidth: true
            Layout.bottomMargin: 6
            text: "Numi-KDE v" + (documentModel ? documentModel.version : "unknown")
            color: aboutHover.hovered
                   ? (Window.window ? Window.window.numiBlue : "#6fc4e8")
                   : (Window.window ? Window.window.numiMuted : "#6b6d76")
            font.pixelSize: 11
            font.underline: aboutHover.hovered
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.8

            HoverHandler { id: aboutHover; cursorShape: Qt.PointingHandCursor }
            TapHandler {
                onTapped: Qt.openUrlExternally("https://github.com/DMYTROSKORIN/numi-kde")
            }
        }
    }
}
