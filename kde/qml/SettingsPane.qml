import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.bottomMargin: 10

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
            Layout.bottomMargin: 15
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            Controls.ScrollBar.vertical.policy: Controls.ScrollBar.AsNeeded
            Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff

            ColumnLayout {
                id: settingsColumn
                width: parent.width
                spacing: 20

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

                // Helper component for Slider styling
                component NumiSlider: Controls.Slider {
                    id: slider
                    Layout.fillWidth: true
                    handle: Rectangle {
                        x: slider.leftPadding + slider.visualPosition * (slider.availableWidth - width)
                        y: slider.topPadding + slider.availableHeight / 2 - height / 2
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 8
                        color: slider.pressed ? (Window.window ? Window.window.numiBlue : "#6fc4e8") : "#f0f0f3"
                        border.color: Window.window ? Window.window.numiWindow : "#22242a"
                        border.width: 1
                    }
                    background: Rectangle {
                        x: slider.leftPadding
                        y: slider.topPadding + slider.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 4
                        width: slider.availableWidth
                        height: implicitHeight
                        radius: 2
                        color: "#343742"
                        Rectangle {
                            width: slider.visualPosition * parent.width
                            height: parent.height
                            color: Window.window ? Window.window.numiBlue : "#6fc4e8"
                            radius: 2
                        }
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
                    spacing: 8
                    Text {
                        text: "Font size: " + (Window.window ? Window.window.fontSize : 16) + " px"
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    NumiSlider {
                        from: 11; to: 24; stepSize: 1
                        value: Window.window ? Window.window.fontSize : 16
                        onMoved: if (Window.window) Window.window.fontSize = Math.round(value)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: "Result width: " + (Window.window ? Window.window.resultWidth : 124) + " px"
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    NumiSlider {
                        from: 96; to: 720; stepSize: 8
                        value: Window.window ? Window.window.resultWidth : 124
                        onMoved: if (Window.window) Window.window.resultWidth = Math.round(value)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: "Decimal places: " + (Window.window ? Math.round(Window.window.decimalPlaces) : 3)
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 13
                    }
                    NumiSlider {
                        from: 0; to: 10; stepSize: 1
                        value: Window.window ? Window.window.decimalPlaces : 3
                        onMoved: if (Window.window) Window.window.decimalPlaces = Math.round(value)
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    RowLayout {
                        spacing: 6
                        Text {
                            text: "Default currency"
                            color: Window.window ? Window.window.numiText : "#f0f0f3"
                            font.pixelSize: 13
                            verticalAlignment: Text.AlignVCenter
                        }
                        Item {
                            width: 16
                            height: 16
                            Rectangle {
                                anchors.centerIn: parent
                                width: 14; height: 14; radius: 7
                                color: "transparent"
                                border.color: Window.window ? Window.window.numiMuted : "#6b6d76"
                                border.width: 1
                            }
                            Text {
                                anchors.centerIn: parent
                                text: "?"
                                color: Window.window ? Window.window.numiMuted : "#6b6d76"
                                font.pixelSize: 10
                                font.weight: Font.DemiBold
                            }
                            HoverHandler { id: currencyHelpHover }
                            Controls.ToolTip {
                                visible: currencyHelpHover.hovered
                                text: "Used when mixing cryptocurrencies\n(e.g. 1 BTC + 1 ETH → " +
                                      (documentModel ? documentModel.defaultCurrency : "USD") + ")"
                                delay: 200
                            }
                        }
                    }

                    Controls.TextField {
                        id: defaultCurrencyField
                        Layout.fillWidth: true
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
                        Layout.fillWidth: true
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
                        Layout.fillWidth: true
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
            Layout.topMargin: 15
            Layout.bottomMargin: 10
        }

        Text {
            Layout.fillWidth: true
            Layout.bottomMargin: 10
            text: "Numi-KDE v" + (documentModel ? documentModel.version : "unknown")
            color: Window.window ? Window.window.numiMuted : "#6b6d76"
            font.pixelSize: 11
            horizontalAlignment: Text.AlignHCenter
            opacity: 0.8
        }
    }
}
