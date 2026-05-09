import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: root

    signal exampleSelected(string text)

    readonly property var examples: [
        { label: "Math",      items: ["2 + 2", "3^2", "sqrt(16)", "sin(pi/2)"] },
        { label: "Variables", items: ["A = 800 - 200", "B := A * 2", "B + 50"] },
        { label: "Units",     items: ["10 m to ft", "1 hour in min", "50 kg to lbs"] },
        { label: "Area",      items: ["10 m^2 to ft^2", "5 ha to acre"] },
        { label: "Data",      items: ["1 GB to MB", "5 GiB to MiB"] },
        { label: "Speed",     items: ["60 km/h to mph", "30 m/s to km/h"] },
        { label: "Money",     items: ["500 EUR - 100 USD", "1 BTC to UAH", "400 USD to ETH"] },
        { label: "Dates",     items: ["today + 2 weeks", "today - 01.01.2000", "01.01.2000 + 25 years"] },
        { label: "Percent",   items: ["20% of 500", "10% from 200"] },
        { label: "Keys",      items: ["Tab — autocomplete", "Ctrl+N — clear", "Click result — copy"], isInfo: true }
    ]

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 16
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.preferredHeight: 36
            Layout.bottomMargin: 10

            Text {
                text: "Help"
                color: Window.window ? Window.window.numiMuted : "#6b6d76"
                font.pixelSize: 13
                font.weight: Font.DemiBold
                Layout.fillWidth: true
                verticalAlignment: Text.AlignVCenter
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: "#3a3d47"
            Layout.bottomMargin: 14
        }

        Controls.ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff

            ColumnLayout {
                width: parent.width
                spacing: 14

                Repeater {
                    model: root.examples

                    delegate: ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 3

                        Text {
                            text: modelData.label
                            color: Window.window ? Window.window.numiBlue : "#6fc4e8"
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }

                        Repeater {
                            model: modelData.items

                            delegate: Rectangle {
                                Layout.fillWidth: true
                                height: 22
                                radius: 3
                                color: !modelData.isInfo && itemHover.hovered
                                       ? (Window.window ? Window.window.controlHover : "#30333b")
                                       : "transparent"

                                Text {
                                    anchors.verticalCenter: parent.verticalCenter
                                    x: 6
                                    text: modelData
                                    color: Window.window ? Window.window.numiText : "#f0f0f3"
                                    font.pixelSize: 12
                                    font.family: "Menlo, Monaco, Consolas, monospace"
                                    elide: Text.ElideRight
                                    width: parent.width - 12
                                }

                                HoverHandler {
                                    id: itemHover
                                    enabled: !modelData.isInfo
                                    cursorShape: Qt.PointingHandCursor
                                }

                                TapHandler {
                                    enabled: !modelData.isInfo
                                    onTapped: {
                                        if (documentModel) documentModel.saveSession()
                                        root.exampleSelected(modelData)
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
