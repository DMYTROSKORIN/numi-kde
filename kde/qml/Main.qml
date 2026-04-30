import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

Kirigami.ApplicationWindow {
    id: root

    width: 980
    height: 640
    minimumWidth: 720
    minimumHeight: 420
    title: "numi-kde"

    pageStack.initialPage: DocumentPage {}

    globalDrawer: Kirigami.GlobalDrawer {
        title: "numi-kde"
        titleIcon: "accessories-calculator"
        isMenu: true

        actions: [
            Kirigami.Action {
                text: "New"
                icon.name: "document-new"
                shortcut: StandardKey.New
            },
            Kirigami.Action {
                text: "Open"
                icon.name: "document-open"
                shortcut: StandardKey.Open
            },
            Kirigami.Action {
                text: "Save"
                icon.name: "document-save"
                shortcut: StandardKey.Save
            },
            Kirigami.Action {
                text: "Preferences"
                icon.name: "settings-configure"
                shortcut: StandardKey.Preferences
            }
        ]
    }
}
