import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollView {
    id: root

    property alias text: editor.text
    property var highlightModel: []
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

    Item {
        id: editorLayer

        width: Math.max(root.availableWidth, editor.contentWidth)
        height: Math.max(root.availableHeight, editor.contentHeight)

        Column {
            id: highlightLayer

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top

            Repeater {
                model: root.highlightModel

                delegate: Text {
                    width: highlightLayer.width
                    height: 25
                    text: highlightedHtml
                    textFormat: Text.RichText
                    color: root.textColor
                    font.family: "Menlo, Monaco, Consolas, monospace"
                    font.pixelSize: 16
                    verticalAlignment: Text.AlignVCenter
                }
            }
        }

        Controls.TextArea {
            id: editor

            anchors.fill: parent
            focus: true
            selectByMouse: true
            wrapMode: TextEdit.NoWrap
            persistentSelection: true
            textFormat: TextEdit.PlainText
            color: "transparent"
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
}
