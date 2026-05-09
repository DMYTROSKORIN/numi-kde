import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

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

            property int _wordStart: 0
            property string _originalWord: ""
            property bool _applyingCompletion: false

            function _getWordStart() {
                let pos = editor.cursorPosition
                let txt = editor.text
                let start = pos
                while (start > 0 && /[A-Za-z0-9_π]/.test(txt[start - 1])) start--
                return start
            }

            function _applyItem(index) {
                let item = completionPopup.model[index]
                if (!item) return
                editor._applyingCompletion = true
                editor.remove(editor._wordStart, editor.cursorPosition)
                editor.insert(editor._wordStart, item)
                editor.cursorPosition = editor._wordStart + item.length
                editor._applyingCompletion = false
            }

            Keys.onTabPressed: (event) => {
                event.accepted = true
                if (completionPopup.visible) {
                    let next = (completionPopup.currentIndex + 1) % completionPopup.model.length
                    completionPopup.currentIndex = next
                    editor._applyItem(next)
                    return
                }
                let pos = editor.cursorPosition
                if (pos === 0) return
                let start = editor._getWordStart()
                let word = editor.text.substring(start, pos)
                if (word.length === 0) return
                let completions = documentModel.getCompletions(word)
                if (completions.length === 0) return
                if (completions.length === 1) {
                    editor._applyingCompletion = true
                    editor.remove(start, pos)
                    editor.insert(start, completions[0])
                    editor.cursorPosition = start + completions[0].length
                    editor._applyingCompletion = false
                    return
                }
                editor._wordStart = start
                editor._originalWord = word
                completionPopup.model = completions
                completionPopup.currentIndex = 0
                editor._applyItem(0)
                let cursorRect = editor.positionToRectangle(editor.cursorPosition)
                let mapped = editor.mapToItem(root, cursorRect.x, cursorRect.y + cursorRect.height + 2)
                completionPopup.x = Math.min(mapped.x, root.width - completionPopup.width - 4)
                completionPopup.y = mapped.y
                completionPopup.open()
            }

            Keys.onUpPressed: (event) => {
                if (!completionPopup.visible) return
                event.accepted = true
                let prev = (completionPopup.currentIndex - 1 + completionPopup.model.length) % completionPopup.model.length
                completionPopup.currentIndex = prev
                editor._applyItem(prev)
            }

            Keys.onDownPressed: (event) => {
                if (!completionPopup.visible) return
                event.accepted = true
                let next = (completionPopup.currentIndex + 1) % completionPopup.model.length
                completionPopup.currentIndex = next
                editor._applyItem(next)
            }

            Keys.onReturnPressed: (event) => {
                if (completionPopup.visible) completionPopup.close()
            }

            Keys.onEscapePressed: (event) => {
                if (!completionPopup.visible) return
                event.accepted = true
                editor._applyingCompletion = true
                editor.remove(editor._wordStart, editor.cursorPosition)
                editor.insert(editor._wordStart, editor._originalWord)
                editor.cursorPosition = editor._wordStart + editor._originalWord.length
                editor._applyingCompletion = false
                completionPopup.close()
            }

            onTextChanged: {
                if (!editor._applyingCompletion && completionPopup.visible) {
                    completionPopup.close()
                }
            }

            background: Rectangle {
                color: "transparent"
            }

        }
    }

    Controls.Popup {
        id: completionPopup

        property var model: []
        property int currentIndex: 0

        padding: 4
        closePolicy: Controls.Popup.NoAutoClose
        parent: root

        background: Rectangle {
            color: "#2a2d36"
            border.color: "#3a3d47"
            border.width: 1
            radius: 4
        }

        width: 180
        height: Math.min(model.length, 8) * 24 + 8

        ListView {
            anchors.fill: parent
            model: completionPopup.model
            interactive: false
            clip: true

            delegate: Rectangle {
                width: completionPopup.width - 8
                height: 24
                radius: 3
                color: index === completionPopup.currentIndex
                       ? (Window.window ? Window.window.controlHover : "#30333b")
                       : "transparent"

                Text {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 8
                    text: modelData
                    color: index === completionPopup.currentIndex
                           ? (Window.window ? Window.window.numiBlue : "#6fc4e8")
                           : (Window.window ? Window.window.numiText : "#f0f0f3")
                    font.family: root.monoFont
                    font.pixelSize: root.monoSize - 2
                    elide: Text.ElideRight
                    width: parent.width - 16
                }

                TapHandler {
                    onTapped: {
                        completionPopup.currentIndex = index
                        editor._applyItem(index)
                        completionPopup.close()
                        editor.forceActiveFocus()
                    }
                }
            }
        }
    }
}
