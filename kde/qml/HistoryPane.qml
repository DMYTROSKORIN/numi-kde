import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root
    
    signal sessionSelected(int index)

    ListView {
        id: historyList
        anchors.fill: parent
        model: documentModel.history
        spacing: 2
        clip: true

        delegate: Rectangle {
            width: historyList.width
            height: 60
            color: itemMouse.hovered ? "#30333b" : "transparent"
            radius: 4

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 2

                Text {
                    text: modelData.text.split('\n')[0]
                    color: "#f0f0f3"
                    font.pixelSize: 13
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: modelData.timestamp
                    color: "#6b6d76"
                    font.pixelSize: 10
                    Layout.fillWidth: true
                }
            }

            HoverHandler {
                id: itemMouse
            }

            TapHandler {
                onTapped: {
                    documentModel.restoreSession(index)
                    root.sessionSelected(index)
                }
            }
        }

        footer: Controls.Button {
            width: historyList.width
            text: qsTr("Clear History")
            visible: historyList.count > 0
            onClicked: documentModel.clearHistory()
            
            contentItem: Text {
                text: parent.text
                color: "#ff5f57"
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 12
            }
            
            background: Rectangle {
                color: "transparent"
            }
        }
    }
}
