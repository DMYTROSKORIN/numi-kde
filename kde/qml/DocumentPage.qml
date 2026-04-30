import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: page

    property string sourceText: ""

    Component.onCompleted: documentModel.source = sourceText

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 38
        anchors.rightMargin: 36
        anchors.topMargin: 8
        anchors.bottomMargin: 34
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            EditorPane {
                id: editor

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 258
                Layout.minimumWidth: 230
                paletteWindow: Window.window.numiWindow
                textColor: Window.window.numiText
                accentYellow: Window.window.numiYellow
                accentBlue: Window.window.numiBlue
                mutedColor: Window.window.numiMuted
                highlightModel: documentModel
                placeholderText: "/help"
                monoSize: Window.window ? Window.window.fontSize : 16
                text: page.sourceText
                onTextChanged: {
                    page.sourceText = text
                    documentModel.source = text
                }
            }

            ResultsPane {
                Layout.preferredWidth: Window.window ? Window.window.resultWidth : 124
                Layout.fillWidth: false
                Layout.minimumWidth: 80
                Layout.maximumWidth: 300
                Layout.fillHeight: true
                resultColor: Window.window.numiGreen
                mutedColor: Window.window.numiMuted
                lineH: editor.lineH
                monoSize: Window.window ? Window.window.fontSize : 16
                lines: documentModel
                syncFlickable: editor.flickable
                onCopyRequested: (row) => documentModel.copyResult(row)
            }
        }
    }

    Controls.Button {
        id: settingsButton

        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 7
        width: 30
        height: 30
        padding: 0
        text: "⚙"
        hoverEnabled: true

        onClicked: settingsPopup.open()

        contentItem: Text {
            text: settingsButton.text
            color: Window.window.numiMuted
            opacity: 0.75
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 20
        }

        background: Rectangle {
            radius: 5
            color: settingsButton.down ? Window.window.controlPressed : settingsButton.hovered ? Window.window.controlHover : "transparent"
        }
    }

    Controls.Popup {
        id: settingsPopup

        parent: settingsButton
        y: -height - 6
        x: 0
        padding: 14

        background: Rectangle {
            color: "#2a2c34"
            radius: 6
            border.color: "#1a1b20"
            border.width: 1
        }

        Column {
            spacing: 10

            Controls.CheckBox {
                id: alwaysOnTopCheck

                text: qsTr("Always on top")
                checked: Window.window ? !!Window.window.alwaysOnTop : true
                onToggled: {
                    if (Window.window) {
                        Window.window.alwaysOnTop = checked
                    }
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

            Column {
                spacing: 4
                width: parent.width

                Text {
                    text: qsTr("Font size")
                    color: "#f0f0f3"
                    font.pixelSize: 11
                }

                Controls.Slider {
                    width: parent.width
                    from: 11
                    to: 24
                    stepSize: 1
                    value: Window.window ? Window.window.fontSize : 16
                    onMoved: if (Window.window) Window.window.fontSize = value
                }
            }

            Column {
                spacing: 4
                width: parent.width

                Text {
                    text: qsTr("Result width")
                    color: "#f0f0f3"
                    font.pixelSize: 11
                }

                Controls.Slider {
                    width: parent.width
                    from: 80
                    to: 300
                    stepSize: 8
                    value: Window.window ? Window.window.resultWidth : 124
                    onMoved: if (Window.window) Window.window.resultWidth = value
                }
            }

            Column {
                spacing: 4
                width: parent.width

                Text {
                    text: qsTr("Decimal places")
                    color: "#f0f0f3"
                    font.pixelSize: 11
                }

                Controls.Slider {
                    width: parent.width
                    from: 0
                    to: 10
                    stepSize: 1
                    value: Window.window ? Window.window.decimalPlaces : 3
                    onMoved: if (Window.window) Window.window.decimalPlaces = value
                }
            }
        }
    }
}
