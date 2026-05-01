import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: page

    property string sourceText: ""
    property var settingsWindow: null
    property bool showResultsSeparator: true
    readonly property int editorMinimumWidth: 230
    readonly property int baseResultMinimumWidth: 96
    readonly property int resultMaximumWidth: 720
    readonly property int pageHorizontalMargins: 74
    readonly property int contentSpacing: 16
    readonly property int separatorWidth: showResultsSeparator ? 8 : 0
    readonly property int measuredResultWidth: Math.ceil(Math.max(resultMetrics.implicitWidth, totalMeasure.implicitWidth) + 10)
    readonly property int minimumResultWidth: Math.max(baseResultMinimumWidth, measuredResultWidth)
    readonly property int effectiveResultWidth: Math.max(minimumResultWidth, Math.min(resultMaximumWidth, Window.window ? Window.window.resultWidth : 124))

    Component.onCompleted: documentModel.source = sourceText

    onMinimumResultWidthChanged: enforceWindowMinimum()
    onShowResultsSeparatorChanged: enforceWindowMinimum()

    function enforceWindowMinimum() {
        if (!Window.window) return
        let needed = page.pageHorizontalMargins
                   + page.editorMinimumWidth
                   + page.contentSpacing
                   + page.separatorWidth
                   + page.minimumResultWidth
        Window.window.minimumWidth = Math.max(420, needed)
        if (Window.window.width < Window.window.minimumWidth) {
            Window.window.width = Window.window.minimumWidth
        }
    }

    function totalLabelText() {
        if (!documentModel) return ""
        let places = documentModel.decimalPlaces
        let val = parseFloat(documentModel.total.toFixed(places))
        return "Total: " + val
    }

    Column {
        id: resultMetrics
        x: -10000
        y: -10000
        opacity: 0
        enabled: false

        Repeater {
            model: documentModel
            delegate: Text {
                text: ok ? result : "Error"
                font.family: "Menlo, Monaco, Consolas, monospace"
                font.pixelSize: Window.window ? Window.window.fontSize : 16
            }
        }

        Text {
            id: totalMeasure
            text: page.totalLabelText()
            font.family: "Menlo, Monaco, Consolas, monospace"
            font.pixelSize: Window.window ? Window.window.fontSize - 2 : 14
        }
    }

    Shortcut {
        sequence: "Ctrl+N"
        context: Qt.ApplicationShortcut
        onActivated: {
            page.sourceText = ""
            documentModel.source = ""
        }
    }

    Connections {
        target: documentModel

        function onSourceChanged() {
            if (page.sourceText !== documentModel.source) {
                page.sourceText = documentModel.source
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent
        anchors.leftMargin: 38
        anchors.rightMargin: 36
        anchors.topMargin: 8
        anchors.bottomMargin: 8
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 16

            EditorPane {
                id: editor

                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.preferredWidth: 258
                Layout.minimumWidth: page.editorMinimumWidth
                paletteWindow: Window.window.numiWindow
                textColor: Window.window.numiText
                accentYellow: Window.window.numiYellow
                accentBlue: Window.window.numiBlue
                mutedColor: Window.window.numiMuted
                highlightModel: documentModel
                placeholderText: "/help"
                monoSize: Window.window ? Window.window.fontSize : 16
                text: page.sourceText
                onTextChanged: {
                    page.sourceText = text
                    documentModel.source = text
                }

                Controls.ScrollView {
                    anchors.fill: parent
                    z: 30
                    visible: (page.sourceText || "").trim() === "/help"
                    clip: true

                    background: Rectangle {
                        radius: 6
                        color: "#282b33"
                        border.color: "#3b3f4b"
                        border.width: 1
                    }

                    Column {
                        width: Math.max(0, editor.availableWidth - 2)
                        padding: 16
                        spacing: 14

                        Text {
                            width: parent.width - parent.padding * 2
                            text: "Numi-KDE Help"
                            color: Window.window.numiText
                            font.family: editor.monoFont
                            font.pixelSize: Math.max(18, editor.monoSize + 3)
                            font.weight: Font.DemiBold
                        }

                        Text {
                            width: parent.width - parent.padding * 2
                            text: "Type one expression per line. Results appear on the right, variables can be reused below, and clicking a result copies it."
                            color: Window.window.numiMuted
                            wrapMode: Text.WordWrap
                            lineHeight: 1.15
                            font.family: editor.monoFont
                            font.pixelSize: Math.max(12, editor.monoSize - 2)
                        }

                        Repeater {
                            model: [
                                { category: "Math", examples: "2 + 2\n3^2\nsqrt(16)\nsin(pi/2)" },
                                { category: "Variables", examples: "A = 800 - 200\nB := A * 2\nB + 50" },
                                { category: "Units", examples: "10 m to ft\n1 hour in min\n50 kg to lbs" },
                                { category: "Currency & Crypto", examples: "500 AED to USD\n1 BTC to UAH\n400 USD to ETH" },
                                { category: "Date & Time", examples: "today + 2 weeks\ntoday - 26.08.1983\ntime\nnow" },
                                { category: "Percentages", examples: "20% of 500\n10% from 200" },
                                { category: "Controls", examples: "Ctrl+N clears the document\nTab completes units, functions and variables\nUse Settings for font, decimals and global hotkey" }
                            ]
                            delegate: Row {
                                width: parent.width - parent.padding * 2
                                spacing: 18

                                Text {
                                    width: Math.min(160, parent.width * 0.42)
                                    text: modelData.category
                                    color: Window.window.numiBlue
                                    font.family: editor.monoFont
                                    font.pixelSize: Math.max(12, editor.monoSize - 2)
                                    font.weight: Font.DemiBold
                                    wrapMode: Text.WordWrap
                                }

                                Text {
                                    width: parent.width - x
                                    text: modelData.examples
                                    color: Window.window.numiText
                                    wrapMode: Text.WordWrap
                                    lineHeight: 1.15
                                    font.family: editor.monoFont
                                    font.pixelSize: Math.max(12, editor.monoSize - 2)
                                }
                            }
                        }
                    }
                }
            }

            Item {
                id: splitterContainer
                Layout.preferredWidth: page.showResultsSeparator ? 8 : 0
                Layout.fillHeight: true
                visible: page.showResultsSeparator

                Rectangle {
                    id: separatorLine
                    x: Math.floor((parent.width - width) / 2)
                    anchors.verticalCenter: parent.verticalCenter
                    width: 1
                    height: parent.height - 24
                    color: splitterMouse.pressed ? Window.window.numiBlue : (splitterMouse.containsMouse ? Window.window.numiMuted : "#2a2c34")
                    opacity: splitterMouse.pressed ? 0.8 : (splitterMouse.containsMouse ? 0.5 : 0.2)
                    antialiasing: false
                }

                MouseArea {
                    id: splitterMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.SizeHorCursor

                    property real startPageX: 0
                    property real startWidth: 0

                    onPressed: (mouse) => {
                        startPageX = splitterContainer.mapToItem(page, mouse.x, mouse.y).x
                        startWidth = page.effectiveResultWidth
                    }

                    onPositionChanged: (mouse) => {
                        if (pressed) {
                            let currentPageX = splitterContainer.mapToItem(page, mouse.x, mouse.y).x
                            let delta = currentPageX - startPageX
                            let newWidth = Math.max(page.minimumResultWidth,
                                                    Math.min(page.resultMaximumWidth,
                                                             Math.round(startWidth - delta)))
                            if (Window.window.resultWidth !== newWidth) {
                                Window.window.resultWidth = newWidth
                            }
                        }
                    }
                }
            }

            ResultsPane {
                id: resultsPane
                Layout.preferredWidth: page.effectiveResultWidth
                Layout.fillWidth: false
                Layout.minimumWidth: page.minimumResultWidth
                Layout.maximumWidth: Math.max(page.minimumResultWidth, page.resultMaximumWidth)
                Layout.fillHeight: true
                resultColor: Window.window.numiGreen
                mutedColor: Window.window.numiMuted
                lineH: editor.lineH
                monoSize: Window.window ? Window.window.fontSize : 16
                lines: documentModel
                syncFlickable: editor.flickable
                onCopyRequested: (row) => documentModel.copyResult(row)
            }
        }

        // Total row — shown when there are 2+ numeric results
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            visible: documentModel && documentModel.resultCount >= 2

            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: 1
                color: "#2a2c34"
            }

            Text {
                id: totalText
                anchors.right: parent.right
                anchors.verticalCenter: parent.verticalCenter
                width: resultsPane.width
                text: page.totalLabelText()
                color: totalMouse.containsMouse ? (Window.window ? Window.window.numiYellow : "#ffd35a")
                                                : (Window.window ? Window.window.numiMuted : "#6b6d76")
                font.family: "Menlo, Monaco, Consolas, monospace"
                font.pixelSize: Window.window ? Window.window.fontSize - 2 : 14
                horizontalAlignment: Text.AlignRight
                elide: Text.ElideRight

                MouseArea {
                    id: totalMouse
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (documentModel) {
                            let places = documentModel.decimalPlaces
                            let val = parseFloat(documentModel.total.toFixed(places))
                            documentModel.copyText(String(val))
                        }
                    }
                }
            }
        }
    }

    Controls.Button {
        id: settingsButton

        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 7
        width: 30
        height: 30
        padding: 0
        text: "⚙"
        hoverEnabled: true

        onClicked: { if (settingsWindow) settingsWindow.show() }

        contentItem: Text {
            text: settingsButton.text
            color: Window.window.numiMuted
            opacity: 0.75
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 20
        }

        background: Rectangle {
            radius: 5
            color: settingsButton.down ? Window.window.controlPressed : settingsButton.hovered ? Window.window.controlHover : "transparent"
        }
    }
}
