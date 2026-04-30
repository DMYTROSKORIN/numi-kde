import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollView {
    id: root

    property alias text: editor.text
    property var highlightModel: []
    property string placeholderText: ""
    property color paletteWindow: "#22242a"
    property color textColor: "#f0f0f3"
    property color accentYellow: "#ffd35a"
    property color accentBlue: "#6fc4e8"
    property color mutedColor: "#6b6d76"
    readonly property alias flickable: editor

    // lineH comes from font metrics so overlay rows match TextArea line height
    readonly property real lineH: fontMetrics.height
    readonly property string monoFont: "Menlo, Monaco, Consolas, monospace"
    property int monoSize: 16

    clip: true

    FontMetrics {
        id: fontMetrics
        font.family: root.monoFont
        font.pixelSize: root.monoSize
    }
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
                    height: root.lineH
                    text: highlightedHtml
                    textFormat: Text.RichText
                    color: root.textColor
                    font.family: root.monoFont
                    font.pixelSize: root.monoSize
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
            cursorDelegate: Rectangle {
                width: 2
                color: root.textColor
                visible: editor.cursorVisible
            }
            font.family: root.monoFont
            font.pixelSize: root.monoSize
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            placeholderText: root.placeholderText
            placeholderTextColor: "#2a2c34"

            background: Rectangle {
                color: "transparent"
            }

        }
    }
}
