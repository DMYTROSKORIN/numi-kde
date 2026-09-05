import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollView {
    id: root

    property var lines: []
    property Item syncFlickable: null
    property real lineH: 25
    property color resultColor: "#8fd14f"
    property color mutedColor: "#6b6d76"
    signal copyRequested(int row)

    readonly property string monoFont: "Menlo, Monaco, Consolas, monospace"
    property int monoSize: 16

    clip: true
    background: Rectangle {
        color: "transparent"
    }

    ListView {
        id: resultList

        width: root.availableWidth
        height: root.availableHeight
        model: root.lines
        boundsBehavior: Flickable.StopAtBounds
        interactive: false

        delegate: Item {
            width: resultList.width
            height: root.lineH

            Controls.Label {
                id: resultText
                text: ok ? result : "Error"
                color: {
                    if (!ok) return "#ff5f57"
                    if (resultMouse.containsMouse && text.length > 0) return "#ffd35a"
                    return root.resultColor
                }
                font.family: root.monoFont
                font.pixelSize: root.monoSize
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter

                // Why did this line fail? The engine's explanation on hover.
                Controls.ToolTip.visible: !ok && resultMouse.containsMouse && diagnostic.length > 0
                Controls.ToolTip.text: diagnostic
                Controls.ToolTip.delay: 250
            }

            MouseArea {
                id: resultMouse

                anchors.fill: parent
                hoverEnabled: true
                cursorShape: resultText.text.length > 0 ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: {
                    if (resultText.text.length > 0) {
                        root.copyRequested(index)
                    }
                }
            }
        }
    }

    Binding {
        target: resultList
        property: "contentY"
        value: root.syncFlickable ? root.syncFlickable.contentY : 0
        restoreMode: Binding.RestoreNone
    }
}
