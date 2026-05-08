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
    readonly property real lineH: fontMetrics.lineSpacing
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

        ListView {
            id: highlightLayer

            anchors.fill: parent
            model: root.highlightModel
            interactive: false
            boundsBehavior: Flickable.StopAtBounds
            contentY: editor.contentY
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

        Controls.TextArea {
            id: editor

            anchors.fill: parent
            focus: true
            selectByMouse: true
            wrapMode: TextEdit.NoWrap
            persistentSelection: true
            textFormat: TextEdit.PlainText
            color: "transparent"
            selectedTextColor: "transparent"
            selectionColor: "#6643505b"
            cursorDelegate: Item {
                width: 2
                height: root.lineH

                Rectangle {
                    width: 2
                    height: fontMetrics.ascent + fontMetrics.descent
                    color: root.accentYellow
                    anchors.verticalCenter: parent.verticalCenter
                    visible: editor.cursorVisible

                    SequentialAnimation on opacity {
                        running: editor.activeFocus
                        loops: Animation.Infinite
                        onStopped: parent.opacity = 1.0
                        PauseAnimation  { duration: 500 }
                        NumberAnimation { to: 0; duration: 150 }
                        PauseAnimation  { duration: 350 }
                        NumberAnimation { to: 1; duration: 150 }
                    }
                }
            }
            font.family: root.monoFont
            font.pixelSize: root.monoSize
            leftPadding: 0
            rightPadding: 0
            topPadding: 0
            bottomPadding: 0
            placeholderText: root.placeholderText
            placeholderTextColor: "#1a1c22"
            opacity: editor.text === "" ? 0.15 : 1.0

            Keys.onTabPressed: (event) => {
                let pos = editor.cursorPosition
                let text = editor.text
                if (pos === 0) return
                
                // Match word characters backward from cursor (simplified regex for speed and compatibility)
                let start = pos
                while (start > 0 && /[A-Za-z0-9_π]/.test(text[start - 1])) {
                    start--
                }
                
                let word = text.substring(start, pos)
                if (word.length > 0) {
                    let completion = documentModel.completeWord(word)
                    if (completion && completion.toLowerCase() !== word.toLowerCase()) {
                        editor.remove(start, pos)
                        editor.insert(start, completion)
                        editor.cursorPosition = start + completion.length
                    }
                }
                event.accepted = true
            }

            background: Rectangle {
                color: "transparent"
            }

        }
    }
}
