import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Kirigami.Page {
    id: page

    title: "Document"
    padding: 0

    property string sourceText: "x = 10\ny = x * 2\n20 inches in cm\n2026-04-30 + 2 weeks"
    property var resultLines: [
        { "line": 1, "result": "10", "ok": true },
        { "line": 2, "result": "20", "ok": true },
        { "line": 3, "result": "50.8 cm", "ok": true },
        { "line": 4, "result": "2026-05-14", "ok": true }
    ]

    actions: [
        Kirigami.Action {
            text: "Copy Results"
            icon.name: "edit-copy"
        }
    ]

    Controls.SplitView {
        anchors.fill: parent
        orientation: Qt.Horizontal

        EditorPane {
            id: editor
            SplitView.fillWidth: true
            text: page.sourceText
            onTextChanged: page.sourceText = text
        }

        ResultsPane {
            SplitView.preferredWidth: Math.max(260, page.width * 0.34)
            SplitView.minimumWidth: 220
            lines: page.resultLines
            syncFlickable: editor.flickable
        }
    }
}
