import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Flickable {
    id: root

    property string text: ""
    property color paletteWindow: "#22242a"
    property color textColor: "#f0f0f3"
    property color accentYellow: "#ffd35a"
    property color accentBlue: "#6fc4e8"
    property color mutedColor: "#6b6d76"
    property bool sampleMode: false
    property var sampleLines: []
    readonly property alias flickable: root

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    contentWidth: editorColumn.implicitWidth
    contentHeight: editorColumn.implicitHeight

    Column {
        id: editorColumn

        width: root.width
        spacing: 0

        Repeater {
            model: root.sampleLines

            Text {
                width: editorColumn.width
                height: 25
                text: modelData
                textFormat: Text.RichText
                color: root.textColor
                font.family: "Menlo, Monaco, Consolas, monospace"
                font.pixelSize: 16
            }
        }
    }
}
