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
    // The ScrollView's own Flickable: this is what actually scrolls, so the
    // results column syncs against it. `textEdit` is the editable control.
    readonly property Item flickable: root.contentItem
    readonly property alias textEdit: editor

    // lineH must match TextArea's actual internal line height, not fontMetrics.lineSpacing,
    // to prevent accumulated vertical drift between highlighted text and the cursor position.
    readonly property real lineH: editor.cursorRectangle.height > 0
                                  ? editor.cursorRectangle.height
                                  : fontMetrics.lineSpacing
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

        TextEdit {
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
            cursorDelegate: Rectangle {
                width: 2
                height: editor.cursorRectangle.height
                color: root.accentYellow
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
            font.family: root.monoFont
            font.pixelSize: root.monoSize

            property int _wordStart: 0
            property int _segmentStart: 0
            property int _replaceFrom: 0
            property bool _useSegmentStart: false
            property string _originalWord: ""
            property bool _applyingCompletion: false

            // Returns the start of the last word in the current segment
            // (advances past internal spaces, e.g. "30 da" → position of 'd').
            function _getWordStart() {
                let pos = editor.cursorPosition
                let txt = editor.text
                let lineStart = pos
                while (lineStart > 0 && txt[lineStart - 1] !== '\n') lineStart--
                const separators = "+-*/()=^%,;"
                let sepIdx = -1
                for (let i = pos - 1; i >= lineStart; i--) {
                    if (separators.includes(txt[i])) { sepIdx = i; break }
                }
                let start = sepIdx === -1 ? lineStart : sepIdx + 1
                while (start < pos && txt[start] === ' ') start++
                // Advance to the last word when segment contains spaces (e.g. "30 da" → "da")
                for (let i = pos - 1; i > start; i--) {
                    if (txt[i] === ' ') { start = i + 1; break }
                }
                return start
            }

            // Returns the start of the full segment (after last separator, leading spaces stripped)
            // without the last-word advance — used as the replace origin for multi-word completions.
            function _getSegmentStart() {
                let pos = editor.cursorPosition
                let txt = editor.text
                let lineStart = pos
                while (lineStart > 0 && txt[lineStart - 1] !== '\n') lineStart--
                const separators = "+-*/()=^%,;"
                let sepIdx = -1
                for (let i = pos - 1; i >= lineStart; i--) {
                    if (separators.includes(txt[i])) { sepIdx = i; break }
                }
                let start = sepIdx === -1 ? lineStart : sepIdx + 1
                while (start < pos && txt[start] === ' ') start++
                return start
            }

            function _applyItem(index) {
                let item = completionPopup.model[index]
                if (!item) return
                let name = item.split('\t')[0]
                // Multi-word completions replace from segment start; single-word completions
                // replace from last-word start — unless the segment ends with a trailing
                // space (_useSegmentStart), in which case we always use segment start.
                let replaceFrom = (name.includes(' ') || editor._useSegmentStart)
                                  ? editor._segmentStart
                                  : editor._wordStart
                editor._replaceFrom = replaceFrom
                editor._applyingCompletion = true
                editor.remove(replaceFrom, editor.cursorPosition)
                editor.insert(replaceFrom, name)
                editor.cursorPosition = replaceFrom + name.length
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

                let txt = editor.text
                let lineStart = pos
                while (lineStart > 0 && txt[lineStart - 1] !== '\n') lineStart--
                let lineContext = txt.substring(lineStart, pos)
                if (lineContext.trim().length === 0) return

                let completions = documentModel.getCompletions(lineContext)
                if (completions.length === 0) return

                let wordStart = editor._getWordStart()
                let segStart = editor._getSegmentStart()
                editor._wordStart = wordStart
                editor._segmentStart = segStart
                // When wordStart == pos the segment ends with a trailing space:
                // always replace from segStart regardless of completion type.
                editor._useSegmentStart = (wordStart === pos)

                if (completions.length === 1) {
                    let name = completions[0].split('\t')[0]
                    let replaceFrom = (name.includes(' ') || editor._useSegmentStart) ? segStart : wordStart
                    editor._applyingCompletion = true
                    editor.remove(replaceFrom, pos)
                    editor.insert(replaceFrom, name)
                    editor.cursorPosition = replaceFrom + name.length
                    editor._applyingCompletion = false
                    return
                }

                let firstName = completions[0].split('\t')[0]
                let firstReplaceFrom = (firstName.includes(' ') || editor._useSegmentStart) ? segStart : wordStart
                editor._originalWord = txt.substring(firstReplaceFrom, pos)
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
                if (!completionPopup.visible) {
                    event.accepted = false
                    return
                }
                event.accepted = true
                let prev = (completionPopup.currentIndex - 1 + completionPopup.model.length) % completionPopup.model.length
                completionPopup.currentIndex = prev
                editor._applyItem(prev)
            }

            Keys.onDownPressed: (event) => {
                if (!completionPopup.visible) {
                    event.accepted = false
                    return
                }
                event.accepted = true
                let next = (completionPopup.currentIndex + 1) % completionPopup.model.length
                completionPopup.currentIndex = next
                editor._applyItem(next)
            }

            Keys.onReturnPressed: (event) => {
                if (completionPopup.visible) {
                    completionPopup.close()
                    event.accepted = true
                } else {
                    event.accepted = false
                }
            }

            Keys.onEscapePressed: (event) => {
                if (!completionPopup.visible) {
                    // No popup to dismiss: Esc puts the calculator away, like KRunner.
                    let win = Window.window
                    if (win && win.escHidesWindow && typeof win.hideWindow === "function") {
                        win.hideWindow()
                        event.accepted = true
                    } else {
                        event.accepted = false
                    }
                    return
                }
                event.accepted = true
                editor._applyingCompletion = true
                editor.remove(editor._replaceFrom, editor.cursorPosition)
                editor.insert(editor._replaceFrom, editor._originalWord)
                editor.cursorPosition = editor._replaceFrom + editor._originalWord.length
                editor._applyingCompletion = false
                completionPopup.close()
            }

            onTextChanged: {
                if (!editor._applyingCompletion && completionPopup.visible) {
                    completionPopup.close()
                }
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

        width: 300
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

                Row {
                    anchors.verticalCenter: parent.verticalCenter
                    x: 8
                    spacing: 4

                    Text {
                        text: modelData.split('\t')[0]
                        color: index === completionPopup.currentIndex
                               ? (Window.window ? Window.window.numiBlue : "#6fc4e8")
                               : (Window.window ? Window.window.numiText : "#f0f0f3")
                        font.family: root.monoFont
                        font.pixelSize: root.monoSize - 2
                    }

                    Text {
                        property string desc: {
                            let parts = modelData.split('\t')
                            return (parts.length > 1 && parts[1].length > 0) ? "(" + parts[1] + ")" : ""
                        }
                        visible: desc.length > 0
                        text: desc
                        color: "#6b6d76"
                        font.family: root.monoFont
                        font.pixelSize: root.monoSize - 3
                    }
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
