import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Item {
    id: page

    property string sourceText: "Price: $10\nFee: 4 GBP in Euro\nsum in USD - 4%\n\nnext friday + 2 weeks\n20 ml in tea spoons\n20% of what is 30 cm"
    property var resultLines: [
        { "line": 1, "result": "$10", "ok": true },
        { "line": 2, "result": "5.65 EUR", "ok": true },
        { "line": 3, "result": "$15.54", "ok": true },
        { "line": 4, "result": "", "ok": true },
        { "line": 5, "result": "8/14/15", "ok": true },
        { "line": 6, "result": "4.06 tsp.", "ok": true },
        { "line": 7, "result": "150 cm", "ok": true }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 38
        anchors.rightMargin: 36
        anchors.topMargin: 8
        anchors.bottomMargin: 14
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
                sampleMode: true
                sampleLines: [
                    "<span style='color:#ffd35a'>Price:</span> $10",
                    "<span style='color:#ffd35a'>Fee:</span> 4 GBP <span style='color:#6fc4e8'>in</span> Euro",
                    "<span style='color:#6fc4e8'>sum in USD</span> - 4%",
                    "",
                    "next friday + 2 weeks",
                    "20 <span style='color:#6fc4e8'>ml in tea spoons</span>",
                    "20% <span style='color:#6fc4e8'>of what is</span> 30 cm"
                ]
                onTextChanged: page.sourceText = text
            }

            ResultsPane {
                Layout.preferredWidth: 124
                Layout.minimumWidth: 108
                Layout.fillHeight: true
                resultColor: Window.window.numiGreen
                mutedColor: Window.window.numiMuted
                lines: page.resultLines
                syncFlickable: editor.flickable
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 34
            spacing: 0

            Controls.Label {
                text: "⌘"
                color: Window.window.numiMuted
                opacity: 0.65
                font.pixelSize: 18
                Layout.alignment: Qt.AlignLeft | Qt.AlignVCenter
            }

            Item {
                Layout.fillWidth: true
            }

            Controls.Label {
                text: "→"
                color: Window.window.numiMuted
                opacity: 0.65
                font.pixelSize: 22
                Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            }
        }
    }
}
