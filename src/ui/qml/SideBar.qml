import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// Repository sidebar. 'sidebar' is the C++ SideBar that owns this view.
Rectangle {
    id: root

    // Keep in sync with ItemKind in SideBar.cpp.
    readonly property int kindHeader: 0
    readonly property int kindOpen: 1
    readonly property int kindRecent: 2
    readonly property int kindAccount: 3
    readonly property int kindAddAccount: 4
    readonly property int kindRemoteRepo: 5
    readonly property int kindError: 6
    readonly property int kindProgress: 7
    readonly property int kindEmpty: 8

    color: Theme.sidebar

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Item {
            Layout.fillWidth: true
            implicitHeight: 40

            Text {
                anchors.left: parent.left
                anchors.leftMargin: 14
                anchors.right: buttons.left
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("Repositories")
                color: Theme.text
                font.pixelSize: 13
                font.bold: true
                elide: Text.ElideRight
            }

            Row {
                id: buttons

                anchors.right: parent.right
                anchors.rightMargin: 6
                anchors.verticalCenter: parent.verticalCenter
                spacing: 2

                ActionButton {
                    compact: true
                    icon: "plus"
                    hasMenu: true
                    menuOnly: true
                    tip: qsTr("Clone, open, or add an account")
                    onMenuRequested: (x, y) => sidebar.showAddMenu(x, y)
                }

                ActionButton {
                    compact: true
                    icon: "more"
                    hasMenu: true
                    menuOnly: true
                    tip: qsTr("Options")
                    onMenuRequested: (x, y) => sidebar.showOptionsMenu(x, y)
                }
            }
        }

        TreeView {
            id: tree

            // Row of the last clicked item that isn't an open repository.
            property int selectedRow: -1

            function restoreExpansion() {
                const model = sidebar.model
                for (let i = 0; i < model.rowCount(); ++i) {
                    const section = model.index(i, 0)
                    const sectionRow = tree.rowAtIndex(section)
                    if (sectionRow < 0)
                        continue

                    if (!sidebar.isExpanded(section)) {
                        tree.collapse(sectionRow)
                        continue
                    }

                    tree.expand(sectionRow)
                    for (let j = 0; j < model.rowCount(section); ++j) {
                        const child = model.index(j, 0, section)
                        if (model.rowCount(child) === 0)
                            continue

                        const row = tree.rowAtIndex(child)
                        if (row < 0)
                            continue

                        if (sidebar.isExpanded(child))
                            tree.expand(row)
                        else
                            tree.collapse(row)
                    }
                }
            }

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            model: sidebar.model
            boundsBehavior: Flickable.StopAtBounds
            columnWidthProvider: function () { return tree.width }
            onWidthChanged: forceLayout()

            Component.onCompleted: restoreExpansion()

            Connections {
                target: sidebar.model

                function onModelReset() {
                    tree.selectedRow = -1
                    Qt.callLater(tree.restoreExpansion)
                }
            }

            Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

            delegate: Item {
                id: delegateItem

                required property TreeView treeView
                required property bool isTreeNode
                required property bool expanded
                required property bool hasChildren
                required property int depth
                required property int row
                required property int column
                required property var display
                required property var toolTip
                required property var kind
                required property var iconName
                required property var isCurrent
                required property var count
                required property var removable

                readonly property bool isHeader: kind === root.kindHeader
                readonly property bool highlighted: isCurrent === true
                readonly property bool selected: !highlighted && tree.selectedRow === row
                readonly property var modelIndex: treeView.index(row, column)

                function toggle() {
                    treeView.toggleExpanded(row)
                    sidebar.setExpanded(modelIndex, treeView.isExpanded(row))
                }

                implicitWidth: treeView.width
                implicitHeight: isHeader ? 32 : 28

                Rectangle {
                    anchors.fill: parent
                    anchors.leftMargin: 6
                    anchors.rightMargin: 6
                    radius: 5
                    visible: !delegateItem.isHeader
                    color: delegateItem.highlighted ? Theme.selected
                                            : mouse.containsMouse ? Theme.hover
                                            : delegateItem.selected ? Theme.pressed
                                                            : "transparent"

                    Rectangle {
                        visible: delegateItem.highlighted
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: 3
                        height: parent.height - 10
                        radius: 1.5
                        color: Theme.accent
                    }
                }

                MouseArea {
                    id: mouse

                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton

                    onClicked: (event) => {
                        if (event.button === Qt.RightButton) {
                            const p = mapToItem(null, event.x, event.y)
                            sidebar.showContextMenu(delegateItem.modelIndex, p.x, p.y)
                            return
                        }

                        if (delegateItem.hasChildren && (delegateItem.isHeader || delegateItem.kind === root.kindAccount)) {
                            delegateItem.toggle()
                            return
                        }

                        tree.selectedRow = delegateItem.kind === root.kindOpen ? -1 : delegateItem.row
                        sidebar.activate(delegateItem.modelIndex)
                    }
                    onDoubleClicked: (event) => {
                        if (event.button === Qt.LeftButton && !delegateItem.isHeader)
                            sidebar.open(delegateItem.modelIndex)
                    }
                    onExited: host.hideToolTip()
                }

                Timer {
                    interval: 700
                    running: mouse.containsMouse && !mouse.pressed && delegateItem.toolTip
                    onTriggered: {
                        const p = delegateItem.mapToItem(null, 0, 0)
                        host.showToolTip(delegateItem.toolTip, p.x, p.y, delegateItem.width, delegateItem.height)
                    }
                }

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: delegateItem.isHeader ? 10 : 16 + (delegateItem.depth - 1) * 16
                    anchors.rightMargin: 12
                    spacing: 6

                    // Expand chevron for headers and accounts.
                    Icon {
                        visible: delegateItem.isHeader || delegateItem.kind === root.kindAccount
                        opacity: delegateItem.hasChildren ? 1 : 0
                        name: "chevron-right"
                        size: delegateItem.isHeader ? 12 : 11
                        color: Theme.textMuted
                        rotation: delegateItem.expanded ? 90 : 0
                        Behavior on rotation { NumberAnimation { duration: 120 } }
                    }

                    Icon {
                        id: icon

                        visible: !delegateItem.isHeader && delegateItem.iconName !== ""
                        name: delegateItem.iconName
                        size: 16
                        color: delegateItem.kind === root.kindError ? Theme.badge
                               : delegateItem.highlighted ? Theme.accent : Theme.textMuted

                        RotationAnimator on rotation {
                            running: delegateItem.kind === root.kindProgress
                            loops: Animation.Infinite
                            from: 0
                            to: 360
                            duration: 900
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: delegateItem.display !== undefined ? delegateItem.display : ""
                        elide: Text.ElideMiddle
                        color: delegateItem.isHeader ? Theme.textMuted
                               : delegateItem.highlighted ? Theme.selectedText
                               : delegateItem.kind === root.kindEmpty ? Theme.textDisabled
                                                              : Theme.text
                        font.pixelSize: delegateItem.isHeader ? 11 : 13
                        font.bold: delegateItem.isHeader || delegateItem.highlighted
                        font.italic: delegateItem.kind === root.kindEmpty || delegateItem.kind === root.kindProgress
                        font.capitalization: delegateItem.isHeader ? Font.AllUppercase : Font.MixedCase
                        font.letterSpacing: delegateItem.isHeader ? 0.8 : 0
                    }

                    // Section item count.
                    Rectangle {
                        visible: delegateItem.isHeader && delegateItem.count > 0
                        implicitWidth: Math.max(implicitHeight, countLabel.implicitWidth + 10)
                        implicitHeight: 16
                        radius: implicitHeight / 2
                        color: Theme.hover

                        Text {
                            id: countLabel

                            anchors.centerIn: parent
                            text: delegateItem.count
                            color: Theme.textMuted
                            font.pixelSize: 10
                            font.bold: true
                        }
                    }

                    // Quick action shown on hover.
                    Icon {
                        visible: delegateItem.removable === true || delegateItem.kind === root.kindAddAccount
                        opacity: mouse.containsMouse || actionMouse.containsMouse ? 1 : 0
                        name: delegateItem.kind === root.kindAddAccount ? "plus" : "close"
                        size: 14
                        color: actionMouse.containsMouse ? Theme.text : Theme.textMuted

                        MouseArea {
                            id: actionMouse

                            anchors.fill: parent
                            anchors.margins: -4
                            hoverEnabled: true
                            onClicked: {
                                if (delegateItem.kind === root.kindAddAccount)
                                    sidebar.open(delegateItem.modelIndex)
                                else
                                    sidebar.remove(delegateItem.modelIndex)
                            }
                        }
                    }
                }
            }
        }
    }

    // Separate the sidebar from the repository view.
    Rectangle {
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        width: 1
        color: Theme.border
    }
}
