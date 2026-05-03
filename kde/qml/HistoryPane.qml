import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    signal sessionSelected(int index)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header
        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.leftMargin: 6
            Layout.rightMargin: 6

            Text {
                text: "History"
                color: Window.window ? Window.window.numiMuted : "#6b6d76"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }

            Controls.Button {
                id: clearBtn
                visible: documentModel ? documentModel.history.length > 0 : false
                width: 28
                height: 28
                padding: 0
                hoverEnabled: true
                Controls.ToolTip.visible: hovered
                Controls.ToolTip.text: "Clear history"
                Controls.ToolTip.delay: 600

                contentItem: Text {
                    text: "🗑"
                    color: Window.window ? Window.window.numiRed : "#ff5f57"
                    font.pixelSize: 14
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                }

                background: Rectangle {
                    radius: 4
                    color: clearBtn.down ? (Window.window ? Window.window.controlPressed : "#3a3d45") : clearBtn.hovered ? (Window.window ? Window.window.controlHover : "#30333b") : "transparent"
                }

                onClicked: documentModel.clearHistory()
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3d47"
        }

        ListView {
            id: historyList
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            spacing: 2
            topMargin: 6
            model: documentModel ? documentModel.history : []

            delegate: Rectangle {
                width: historyList.width
                height: 58
                color: itemHover.hovered ? (Window.window ? Window.window.controlHover : "#30333b") : "transparent"
                radius: 5

                ColumnLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    anchors.topMargin: 7
                    anchors.bottomMargin: 7
                    spacing: 3

                    Text {
                        text: modelData.text.split('\n')[0]
                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                        font.pixelSize: 12
                        font.family: "Menlo, Monaco, Consolas, monospace"
                        elide: Text.ElideRight
                        Layout.fillWidth: true
                    }

                    Text {
                        text: modelData.timestamp
                        color: Window.window ? Window.window.numiMuted : "#6b6d76"
                        font.pixelSize: 10
                        Layout.fillWidth: true
                    }
                }

                HoverHandler { id: itemHover }

                TapHandler {
                    onTapped: {
                        documentModel.restoreSession(index)
                        root.sessionSelected(index)
                    }
                }
            }

            Controls.Label {
                anchors.centerIn: parent
                visible: historyList.count === 0
                text: "No history yet"
                color: Window.window ? Window.window.numiMuted : "#6b6d76"
                font.pixelSize: 12
            }
        }
    }
}
