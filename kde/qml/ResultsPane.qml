import QtQuick
import QtQuick.Controls as Controls

Controls.ScrollView {
    id: root

    property var lines: []
    property Item syncFlickable: null
    property color resultColor: "#8fd14f"
    property color mutedColor: "#6b6d76"
    signal copyRequested(int row)

    clip: true
    background: Rectangle {
        color: "transparent"
    }

    ListView {
        id: resultList

        model: root.lines
        boundsBehavior: Flickable.StopAtBounds
        interactive: false

        delegate: Item {
            width: resultList.width
            height: 25

            Rectangle {
                anchors.fill: parent
                radius: 4
                color: resultMouse.containsMouse && resultText.text.length > 0 ? "#30333b" : "transparent"
                z: -1
            }

            Controls.Label {
                id: resultText
                text: ok ? result : ""
                color: ok ? root.resultColor : "#ff5f57"
                font.family: "Menlo, Monaco, Consolas, monospace"
                font.pixelSize: 16
                horizontalAlignment: Text.AlignRight
                verticalAlignment: Text.AlignVCenter
                elide: Text.ElideRight
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
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
}
