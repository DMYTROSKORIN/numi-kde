import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    property var appWindow: null

    // Propagate layout's natural size so SettingsWindow can size itself to content
    implicitWidth:  col.implicitWidth
    implicitHeight: col.implicitHeight

    ColumnLayout {
        id: col
        anchors.fill: parent
        spacing: 4

        // ── Toggles ──────────────────────────────────────────────────
        Controls.CheckBox {
            text: qsTr("Always on top")
            checked: appWindow ? appWindow.alwaysOnTop : true
            onToggled: if (appWindow) appWindow.alwaysOnTop = checked
        }
        Controls.CheckBox {
            text: qsTr("Launch at login")
            checked: documentModel ? documentModel.autostart : false
            onToggled: if (documentModel) documentModel.autostart = checked
        }
        Controls.CheckBox {
            text: qsTr("Show result separator")
            checked: appWindow ? appWindow.showResultsSeparator : true
            onToggled: if (appWindow) appWindow.showResultsSeparator = checked
        }

        Controls.MenuSeparator { Layout.fillWidth: true }

        // ── Numeric settings ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Controls.Label { text: qsTr("Font size"); Layout.fillWidth: true }
            Controls.SpinBox {
                from: 11; to: 24
                value: appWindow ? appWindow.fontSize : 16
                onValueModified: if (appWindow) appWindow.fontSize = value
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Controls.Label { text: qsTr("Result width"); Layout.fillWidth: true }
            Controls.SpinBox {
                from: 80; to: 720
                stepSize: 4
                value: appWindow ? appWindow.resultWidth : 124
                onValueModified: if (appWindow) appWindow.resultWidth = value
            }
        }
        RowLayout {
            Layout.fillWidth: true
            Controls.Label { text: qsTr("Decimal places"); Layout.fillWidth: true }
            Controls.SpinBox {
                from: 0; to: 10
                value: appWindow ? appWindow.decimalPlaces : 3
                onValueModified: if (appWindow) appWindow.decimalPlaces = value
            }
        }

        Controls.MenuSeparator { Layout.fillWidth: true }

        // ── Default currency ─────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            Controls.Label { text: qsTr("Default currency"); Layout.fillWidth: true }
            Controls.TextField {
                implicitWidth: 72
                text: documentModel ? documentModel.defaultCurrency : "USD"
                maximumLength: 5
                validator: RegularExpressionValidator { regularExpression: /[A-Za-z]{0,5}/ }
                placeholderText: "USD"
                onEditingFinished: {
                    if (documentModel)
                        documentModel.defaultCurrency = text.toUpperCase()
                    text = documentModel ? documentModel.defaultCurrency : "USD"
                }
            }
        }

        Controls.MenuSeparator { Layout.fillWidth: true }

        // ── Global hotkey ─────────────────────────────────────────────
        ColumnLayout {
            id: hotkeyColumn
            Layout.fillWidth: true
            spacing: 6
            property bool recordingHotkey: false

            function keyName(key, text) {
                if (key >= Qt.Key_0 && key <= Qt.Key_9) return String.fromCharCode("0".charCodeAt(0) + key - Qt.Key_0)
                if (key >= Qt.Key_A && key <= Qt.Key_Z) return String.fromCharCode("A".charCodeAt(0) + key - Qt.Key_A)
                if (key >= Qt.Key_F1 && key <= Qt.Key_F12) return "F" + (key - Qt.Key_F1 + 1)
                if (key === Qt.Key_Space) return "Space"
                if (text && text.length > 0) return text.toUpperCase()
                return ""
            }

            Controls.Label { text: qsTr("Global hotkey") }

            Controls.TextField {
                id: hotkeyField
                Layout.fillWidth: true
                text: hotkeyColumn.recordingHotkey
                      ? qsTr("Press shortcut…")
                      : (shortcutManager ? shortcutManager.sequence : "Ctrl+Alt+1")
                readOnly: true
                activeFocusOnPress: true
                placeholderText: "Ctrl+Alt+1"

                Keys.onPressed: (event) => {
                    if (!hotkeyColumn.recordingHotkey) return
                    if (event.key === Qt.Key_Escape) {
                        hotkeyColumn.recordingHotkey = false
                        event.accepted = true
                        return
                    }
                    if (event.key === Qt.Key_Control || event.key === Qt.Key_Alt ||
                        event.key === Qt.Key_Shift   || event.key === Qt.Key_Meta) {
                        event.accepted = true
                        return
                    }
                    let parts = []
                    if (event.modifiers & Qt.ControlModifier) parts.push("Ctrl")
                    if (event.modifiers & Qt.AltModifier)     parts.push("Alt")
                    if (event.modifiers & Qt.ShiftModifier)   parts.push("Shift")
                    if (event.modifiers & Qt.MetaModifier)    parts.push("Meta")
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
            }

            Controls.Label {
                Layout.fillWidth: true
                text: hotkeyColumn.recordingHotkey
                      ? qsTr("Press a combination, Esc to cancel")
                      : qsTr("Click to record a shortcut")
                font.italic: true
                opacity: 0.7
                wrapMode: Text.WordWrap
            }

            Controls.Label {
                Layout.fillWidth: true
                visible: shortcutManager && shortcutManager.status.length > 0
                text: shortcutManager ? shortcutManager.status : ""
                color: text === "Shortcut saved" ? "#4CAF50" : "#f44336"
                font.bold: true
            }
        }

        Controls.MenuSeparator { Layout.fillWidth: true }

        Controls.Label {
            Layout.fillWidth: true
            Layout.bottomMargin: 4
            text: "Numi-KDE v" + (documentModel ? documentModel.version : "unknown")
            opacity: 0.6
            font.italic: true
            horizontalAlignment: Text.AlignHCenter

            HoverHandler { id: aboutHover; cursorShape: Qt.PointingHandCursor }
            TapHandler {
                onTapped: Qt.openUrlExternally("https://github.com/DMYTROSKORIN/numi-kde")
            }
        }
    }
}
