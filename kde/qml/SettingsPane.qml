import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kquickcontrols as KQuickControls

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

        Controls.CheckBox {
            text: qsTr("Esc hides the window")
            checked: appWindow ? appWindow.escHidesWindow : true
            onToggled: if (appWindow) appWindow.escHidesWindow = checked
        }

        Controls.CheckBox {
            text: qsTr("Install updates automatically")
            checked: updateChecker ? updateChecker.autoInstallUpdates : true
            onToggled: if (updateChecker) updateChecker.setAutoInstallUpdates(checked)
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
            Layout.fillWidth: true
            spacing: 6

            Controls.Label { text: qsTr("Global hotkey") }

            // Standard KDE recorder: accepts any sequence and asks about
            // conflicts with other global shortcuts before applying.
            KQuickControls.KeySequenceItem {
                id: hotkeyItem
                Layout.fillWidth: true
                showClearButton: false
                keySequence: shortcutManager ? shortcutManager.sequence : "Ctrl+Alt+1"
                onKeySequenceModified: {
                    if (shortcutManager)
                        shortcutManager.applyKeySequence(keySequence)
                }
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
