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
        if (!documentModel || !documentModel.hasTotal) return ""
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

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: parent.top
                    anchors.topMargin: editor.lineH + 10
                    z: 30
                    visible: (page.sourceText || "").trim() === "/help"
                    spacing: 3

                    Repeater {
                        model: [
                            { label: "Math", text: "2 <span style='color:#ffd35a'>+</span> 2    3<span style='color:#ffd35a'>^</span>2    sqrt(16)    sin(pi/2)" },
                            { label: "Variables", text: "A <span style='color:#ffd35a'>=</span> 800 <span style='color:#ffd35a'>-</span> 200    B <span style='color:#ffd35a'>:=</span> A <span style='color:#ffd35a'>*</span> 2    B <span style='color:#ffd35a'>+</span> 50" },
                            { label: "Units", text: "10 m <span style='color:#ffd35a'>to</span> ft    1 hour <span style='color:#ffd35a'>in</span> min    50 kg <span style='color:#ffd35a'>to</span> lbs" },
                            { label: "Money", text: "500 AED <span style='color:#ffd35a'>to</span> USD    1 BTC <span style='color:#ffd35a'>to</span> UAH    400 USD <span style='color:#ffd35a'>to</span> ETH" },
                            { label: "Dates", text: "<span style='color:#ffd35a'>today</span> <span style='color:#ffd35a'>+</span> 2 weeks    <span style='color:#ffd35a'>today</span> <span style='color:#ffd35a'>-</span> 26.08.1983    26.08.1983 <span style='color:#ffd35a'>+</span> 42 years" },
                            { label: "Percent", text: "20% <span style='color:#ffd35a'>of</span> 500    10% <span style='color:#ffd35a'>from</span> 200" },
                            { label: "Keys", text: "Tab completes    Ctrl<span style='color:#ffd35a'>+</span>N clears    click result <span style='color:#ffd35a'>to</span> copy" }
                        ]
                        delegate: Row {
                            width: parent.width
                            spacing: 12

                            Text {
                                width: Math.max(86, editor.monoSize * 6)
                                text: modelData.label
                                color: Window.window.numiBlue
                                font.family: editor.monoFont
                                font.pixelSize: editor.monoSize
                                font.weight: Font.DemiBold
                            }

                            Text {
                                width: parent.width - x
                                text: modelData.text
                                textFormat: Text.RichText
                                color: Window.window.numiText
                                wrapMode: Text.WordWrap
                                lineHeight: 1.05
                                font.family: editor.monoFont
                                font.pixelSize: editor.monoSize
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
                    color: splitterMouse.pressed ? Window.window.numiBlue : (splitterMouse.containsMouse ? "#343742" : "#2a2c34")
                    opacity: 1.0
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

        // Total row — shown only for compatible numeric result rows.
        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            visible: documentModel && documentModel.hasTotal

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
