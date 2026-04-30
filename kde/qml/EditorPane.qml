import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Controls.ScrollView {
    id: root

    property alias text: editor.text
    readonly property alias flickable: editor

    clip: true

    Controls.TextArea {
        id: editor

        selectByMouse: true
        wrapMode: TextEdit.NoWrap
        persistentSelection: true
        textFormat: TextEdit.PlainText
        font.family: Kirigami.Theme.fixedWidthFont.family
        font.pointSize: Kirigami.Theme.defaultFont.pointSize
        color: Kirigami.Theme.textColor
        selectedTextColor: Kirigami.Theme.highlightedTextColor
        selectionColor: Kirigami.Theme.highlightColor
        background: Rectangle {
            color: Kirigami.Theme.backgroundColor
        }
    }
}
