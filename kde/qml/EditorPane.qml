import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollView {
    id: root

    property alias text: editor.text
    property color paletteWindow: "#22242a"
    property color textColor: "#f0f0f3"
    property color accentYellow: "#ffd35a"
    property color accentBlue: "#6fc4e8"
    property color mutedColor: "#6b6d76"
    readonly property alias flickable: editor

    clip: true
    background: Rectangle {
        color: "transparent"
    }

    Controls.TextArea {
        id: editor

        selectByMouse: true
        wrapMode: TextEdit.NoWrap
        persistentSelection: true
        textFormat: TextEdit.PlainText
        color: root.textColor
        selectedTextColor: "#ffffff"
        selectionColor: "#43505b"
        font.family: "Menlo, Monaco, Consolas, monospace"
        font.pixelSize: 16
        leftPadding: 0
        rightPadding: 0
        topPadding: 0
        bottomPadding: 0

        background: Rectangle {
            color: "transparent"
        }
    }
}
