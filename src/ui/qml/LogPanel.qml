import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The activity log at the bottom of the repository page. 'logPanel' is the
// C++ LogPanel.
Rectangle {
    id: root

    // Keep in sync with LogEntry::Kind.
    readonly property int entryKind: 0
    readonly property int fileKind: 1
    readonly property int hintKind: 2
    readonly property int warningKind: 3
    readonly property int errorKind: 4

    property int currentRow: -1
    property bool showHeader: true

    color: Theme.panel

    Keys.onEscapePressed: logPanel.close()
    // Take the copy shortcut from the Edit menu.
    Keys.onShortcutOverride: (event) => {
        if (event.matches(StandardKey.Copy) && root.currentRow >= 0)
            event.accepted = true
    }
    Keys.onPressed: (event) => {
        if (event.matches(StandardKey.Copy) && root.currentRow >= 0) {
            logPanel.copy(tree.index(root.currentRow, 0))
            event.accepted = true
        }
    }

    // Expand new entries, collapse the previous one unless it reports a
    // problem and keep the newest in view, like a terminal.
    Connections {
        target: logPanel.model

        function onRowsInserted(parent, first, last) {
            const topLevel = !parent.valid
            Qt.callLater(() => {
                if (topLevel) {
                    const model = logPanel.model
                    const previousIndex = model.index(first - 1, 0)
                    if (logPanel.collapseEnabled && first > 0
                            && !logPanel.hasProblems(previousIndex)) {
                        const previous = tree.rowAtIndex(previousIndex)
                        if (previous >= 0)
                            tree.collapse(previous)
                    }

                    const row = tree.rowAtIndex(model.index(last, 0))
                    if (row >= 0)
                        tree.expand(row)
                }

                tree.positionViewAtRow(tree.rows - 1, TableView.AlignBottom)
            })
        }
    }

    Connections {
        target: logPanel

        function onExpandRequested(index, expanded) {
            Qt.callLater(() => {
                const row = tree.rowAtIndex(index)
                if (row < 0)
                    return
                if (expanded)
                    tree.expand(row)
                else
                    tree.collapse(row)
            })
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Header.
        Rectangle {
            Layout.fillWidth: true
            visible: root.showHeader
            implicitHeight: 32
            color: Theme.panel

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: Theme.border
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.border
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 12
                anchors.rightMargin: 6
                spacing: 8

                Icon {
                    name: "log"
                    size: 14
                    color: Theme.textMuted
                }

                Text {
                    Layout.fillWidth: true
                    text: qsTr("Activity")
                    color: Theme.text
                    font.pixelSize: 12
                    font.weight: Font.DemiBold
                }

                ActionButton {
                    compact: true
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: "copy"
                    tip: qsTr("Copy the log")
                    onClicked: logPanel.copyAll()
                }

                ActionButton {
                    compact: true
                    implicitWidth: 24
                    implicitHeight: 24
                    icon: "chevron-down"
                    tip: qsTr("Hide the log (Esc)")
                    onClicked: logPanel.close()
                }
            }
        }

        TreeView {
            id: tree

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: logPanel.model
            boundsBehavior: Flickable.StopAtBounds
            columnWidthProvider: function () { return tree.width }
            onWidthChanged: forceLayout()

            Controls.ScrollBar.vertical: ThinScrollBar {}

            delegate: Item {
                id: entry

                required property TreeView treeView
                required property bool expanded
                required property bool hasChildren
                required property int depth
                required property int row
                required property string display
                required property int kind
                required property int progress
                required property string status

                readonly property bool busy: kind === root.entryKind && progress >= 0

                implicitWidth: treeView.width
                implicitHeight: Math.max(24, label.implicitHeight + 8)

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 4
                    anchors.rightMargin: 4
                    radius: 4
                    color: entry.row === root.currentRow ? Theme.selected
                                                         : entryMouse.containsMouse ? Theme.hover
                                                                                    : "transparent"
                }

                MouseArea {
                    id: entryMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onPressed: root.forceActiveFocus()
                    onClicked: (event) => {
                        root.currentRow = entry.row
                        if (event.button === Qt.RightButton) {
                            const p = mapToItem(null, event.x, event.y)
                            logPanel.showMenu(entry.treeView.index(entry.row, 0), p.x, p.y)
                        }
                    }
                    onDoubleClicked: {
                        if (entry.hasChildren)
                            entry.treeView.toggleExpanded(entry.row)
                    }
                }

                // Disclosure indicator.
                Item {
                    id: indicator

                    x: 8 + entry.depth * 16
                    y: 4
                    width: 16
                    height: 16

                    Icon {
                        visible: entry.hasChildren
                        anchors.centerIn: parent
                        name: "chevron-right"
                        size: 11
                        color: Theme.textMuted
                        rotation: entry.expanded ? 90 : 0
                    }

                    MouseArea {
                        anchors.fill: parent
                        enabled: entry.hasChildren
                        cursorShape: Qt.PointingHandCursor
                        onClicked: entry.treeView.toggleExpanded(entry.row)
                    }
                }

                // Kind of the entry, or the progress of an operation.
                Item {
                    id: decoration

                    anchors.left: indicator.right
                    anchors.leftMargin: 2
                    y: 4
                    width: visible ? 16 : 0
                    height: 16
                    visible: entry.busy || entry.kind !== root.entryKind

                    Spinner {
                        anchors.centerIn: parent
                        visible: entry.busy && !cancelMouse.containsMouse
                        size: 14
                        color: Theme.accent
                    }

                    Icon {
                        anchors.centerIn: parent
                        visible: entry.busy && cancelMouse.containsMouse
                        name: "close"
                        size: 12
                        color: Theme.deleted
                    }

                    MouseArea {
                        id: cancelMouse

                        anchors.fill: parent
                        enabled: entry.busy
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: logPanel.cancel(entry.treeView.index(entry.row, 0))
                    }

                    StatusBadge {
                        anchors.centerIn: parent
                        visible: entry.kind === root.fileKind
                        status: entry.status
                    }

                    Icon {
                        anchors.centerIn: parent
                        visible: entry.kind === root.hintKind
                        name: "info"
                        size: 14
                        color: Theme.accent
                    }

                    Icon {
                        anchors.centerIn: parent
                        visible: entry.kind === root.warningKind
                        name: "warning"
                        size: 14
                        color: Theme.modified
                    }

                    Icon {
                        anchors.centerIn: parent
                        visible: entry.kind === root.errorKind
                        name: "alert"
                        size: 14
                        color: Theme.deleted
                    }
                }

                Text {
                    id: label

                    anchors.left: decoration.right
                    anchors.leftMargin: 6
                    anchors.right: parent.right
                    anchors.rightMargin: 12
                    y: 4
                    textFormat: Text.RichText
                    wrapMode: Text.Wrap
                    text: "<style>a { color: " + Theme.accent + "; text-decoration: none; }</style>"
                          + entry.display
                    color: entry.kind === root.errorKind ? Theme.deleted : Theme.text
                    font.pixelSize: 12
                    onLinkActivated: (link) => {
                        if (link === "expand")
                            entry.treeView.toggleExpanded(entry.row)
                        else
                            logPanel.activateLink(link)
                    }

                    HoverHandler {
                        cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                    }
                }
            }
        }
    }
}
