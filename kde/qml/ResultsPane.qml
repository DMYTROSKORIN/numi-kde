import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ScrollView {
    id: root

    property var lines: []
    property Flickable syncFlickable: null
    property color resultColor: "#8fd14f"
    property color mutedColor: "#6b6d76"

    clip: true
    background: Rectangle {
        color: "transparent"
    }

    Connections {
        target: root.syncFlickable
        function onContentYChanged() {
            resultList.contentY = root.syncFlickable.contentY
        }
    }

    ListView {
        id: resultList

        model: root.lines
        boundsBehavior: Flickable.StopAtBounds
        interactive: false

        delegate: RowLayout {
            width: resultList.width
            height: 25
            spacing: Kirigami.Units.smallSpacing

            Controls.Label {
                id: resultText
                text: modelData.ok ? modelData.result : ""
                color: modelData.ok ? root.resultColor : "#ff5f57"
                font.family: "Menlo, Monaco, Consolas, monospace"
                font.pixelSize: 16
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }
}
