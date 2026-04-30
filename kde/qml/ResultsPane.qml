import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami

Controls.ScrollView {
    id: root

    property var lines: []
    property Flickable syncFlickable: null

    clip: true

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
            height: Math.max(resultText.implicitHeight, Kirigami.Units.gridUnit)
            spacing: Kirigami.Units.smallSpacing

            Controls.Label {
                text: modelData.line
                color: Kirigami.Theme.disabledTextColor
                font.pointSize: Kirigami.Theme.smallFont.pointSize
                Layout.preferredWidth: Kirigami.Units.gridUnit * 2
            }

            Controls.Label {
                id: resultText
                text: modelData.ok ? modelData.result : ""
                color: modelData.ok ? Kirigami.Theme.textColor : Kirigami.Theme.negativeTextColor
                font.family: Kirigami.Theme.fixedWidthFont.family
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
    }
}
