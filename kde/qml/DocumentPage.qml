import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: page

    property string sourceText: "Price: $10\nFee: 4 GBP in Euro\nsum in USD - 4%\n\nnext friday + 2 weeks\n20 ml in tea spoons\n20% of what is 30 cm"

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
                text: page.sourceText
                onTextChanged: {
                    page.sourceText = text
                    documentModel.source = text
                }
            }

            ResultsPane {
                Layout.preferredWidth: 124
                Layout.minimumWidth: 108
                Layout.fillHeight: true
                resultColor: Window.window.numiGreen
                mutedColor: Window.window.numiMuted
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
}
