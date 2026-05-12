import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

Item {
    id: page

    property string sourceText: ""
    property var settingsWindow: null
    property bool showResultsSeparator: true
    property bool _showHelp: false
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
        return "Total: " + documentModel.total.toLocaleString(Qt.locale(), 'f', places)
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
            if (documentModel) documentModel.saveSession()
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
                placeholderText: ""
                monoSize: Window.window ? Window.window.fontSize : 16
                text: page.sourceText
                onTextChanged: {
                    page.sourceText = text
                    documentModel.source = text
                }

                Rectangle {
                    anchors.fill: parent
                    z: 30
                    visible: page._showHelp
                    color: Window.window ? Window.window.numiWindow : "#22242a"

                    Controls.ScrollView {
                        anchors.fill: parent
                        anchors.margins: 8
                        Controls.ScrollBar.horizontal.policy: Controls.ScrollBar.AlwaysOff
                        clip: true

                        Column {
                            width: parent.width
                            spacing: 7

                            Repeater {
                                model: [
                                    { label: "Math",      text: "2 + 2    3^2    sqrt(16)    sin(pi/2)    abs(-5)" },
                                    { label: "Variables", text: "price = 1200    tax = price * 0.2    price + tax" },
                                    { label: "Units",     text: "10 m to ft    1 hour in min    50 kg to lbs    100 °C to F" },
                                    { label: "Area",      text: "10 m^2 to ft^2    5 ha to acre    1 km^2 to m^2" },
                                    { label: "Data",      text: "1 GB to MB    5 GiB to MiB    100 Mb to Gb" },
                                    { label: "Speed",     text: "60 km/h to mph    30 m/s to km/h" },
                                    { label: "Money",     text: "500 EUR - 100 USD    1 BTC to UAH    400 USD to ETH" },
                                    { label: "Dates",     text: "today + 2 weeks    today - 01.01.2000    01.01.2000 + 25 years" },
                                    { label: "Percent",   text: "20% of 500    10% from 200    500 + 20%" },
                                    { label: "Keys",      text: "Tab — autocomplete    Ctrl+N — clear    Click result — copy" }
                                ]

                                delegate: Row {
                                    width: parent.width
                                    spacing: 12

                                    Text {
                                        width: Math.max(80, editor.monoSize * 5.5)
                                        text: modelData.label
                                        color: Window.window ? Window.window.numiBlue : "#6fc4e8"
                                        font.family: editor.monoFont
                                        font.pixelSize: editor.monoSize - 1
                                        font.weight: Font.DemiBold
                                    }

                                    Text {
                                        width: parent.width - x
                                        text: documentModel ? documentModel.highlightExample(modelData.text) : modelData.text
                                        textFormat: Text.RichText
                                        color: Window.window ? Window.window.numiText : "#f0f0f3"
                                        wrapMode: Text.WordWrap
                                        lineHeight: 1.1
                                        font.family: editor.monoFont
                                        font.pixelSize: editor.monoSize - 1
                                    }
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
                            documentModel.copyText(
                                documentModel.total.toLocaleString(Qt.locale(), 'f', places))
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

        onClicked: { if (settingsDrawer) settingsDrawer.open() }

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

    Controls.Button {
        id: helpButton

        anchors.left: settingsButton.right
        anchors.leftMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 9
        width: 26
        height: 26
        padding: 0
        text: "?"
        hoverEnabled: true

        onClicked: page._showHelp = !page._showHelp

        contentItem: Text {
            text: helpButton.text
            color: page._showHelp ? (Window.window ? Window.window.numiBlue : "#6fc4e8")
                                  : (Window.window ? Window.window.numiMuted : "#6b6d76")
            opacity: 0.85
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.pixelSize: 15
            font.weight: Font.DemiBold
        }

        background: Rectangle {
            radius: 5
            color: helpButton.down ? Window.window.controlPressed : helpButton.hovered ? Window.window.controlHover : "transparent"
        }
    }
}
