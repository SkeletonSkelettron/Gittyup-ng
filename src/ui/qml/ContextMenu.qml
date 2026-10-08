import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// A context menu with the actions of a C++ QMenu, shown by
// QmlSupport::execMenu(). 'items' lists the actions with their 'text' and
// 'shortcut', whether they are 'enabled', 'checkable' and 'checked', and
// either a 'submenu' of items or the 'id' that is 'chosen'.
Controls.Menu {
    id: root

    property var items: []
    readonly property bool hasChecks: items.some((item) => item.checkable === true)
    // The id of the triggered action, also from a submenu.
    property int chosen: -1

    signal activated(int id)

    onActivated: (id) => chosen = id

    modal: true
    dim: false
    padding: 4
    margins: 8
    overlap: 4

    background: Rectangle {
        implicitWidth: 200
        radius: 8
        color: Theme.panel
        border.color: Theme.border
    }

    // The entries of submenus.
    delegate: Entry {
        checkColumn: root.hasChecks
    }

    component Entry: Controls.MenuItem {
        id: entry

        property string shortcut
        property bool checkColumn: false

        implicitHeight: 28
        leftPadding: 10
        rightPadding: 10
        indicator: null
        arrow: null

        readonly property color foreground: !enabled ? Theme.textDisabled
                                            : highlighted ? Theme.selectedText : Theme.text

        contentItem: RowLayout {
            spacing: 10

            Item {
                visible: entry.checkColumn
                Layout.preferredWidth: 14
                Layout.fillHeight: true

                Icon {
                    anchors.centerIn: parent
                    visible: entry.checked
                    name: "check"
                    size: 12
                    color: entry.foreground
                }
            }

            Text {
                Layout.fillWidth: true
                text: entry.text
                elide: Text.ElideRight
                color: entry.foreground
                font.pixelSize: 13
            }

            Text {
                visible: entry.shortcut !== ""
                Layout.leftMargin: 14
                text: entry.shortcut
                color: entry.highlighted && entry.enabled ? Theme.selectedText : Theme.textMuted
                font.pixelSize: 12
            }

            Icon {
                visible: entry.subMenu !== null
                name: "chevron-right"
                size: 12
                color: entry.foreground
            }
        }

        background: Rectangle {
            radius: 5
            color: entry.highlighted && entry.enabled ? Theme.selected : "transparent"
        }
    }

    component Separator: Controls.MenuSeparator {
        topPadding: 4
        bottomPadding: 4
        leftPadding: 6
        rightPadding: 6

        contentItem: Rectangle {
            implicitHeight: 1
            color: Theme.border
        }
    }

    Component {
        id: entryComponent

        Entry {}
    }

    Component {
        id: separatorComponent

        Separator {}
    }

    Component.onCompleted: {
        for (const item of root.items) {
            if (item.separator) {
                root.addItem(separatorComponent.createObject(null))
            } else if (item.submenu) {
                const component = Qt.createComponent("ContextMenu.qml")
                const menu = component.createObject(null, {
                    title: item.text,
                    items: item.submenu
                })
                menu.activated.connect(root.activated)
                root.addMenu(menu)
            } else {
                const entry = entryComponent.createObject(null, {
                    text: item.text,
                    shortcut: item.shortcut,
                    enabled: item.enabled,
                    checkable: item.checkable,
                    checked: item.checked,
                    checkColumn: root.hasChecks
                })
                const id = item.id
                entry.triggered.connect(() => root.activated(id))
                root.addItem(entry)
            }
        }

        // As wide as the widest entry.
        let widest = 0
        for (let i = 0; i < root.count; ++i) {
            const item = root.itemAt(i)
            if (item)
                widest = Math.max(widest, item.implicitWidth)
        }
        root.width = Math.max(200, Math.min(520, widest + root.leftPadding + root.rightPadding))
    }
}
