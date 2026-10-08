import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The commit graph. 'commitList' is the C++ CommitList, 'repoView' the
// RepoView that owns this page.
Rectangle {
    id: root

    readonly property int rowHeight: commitList.compact ? 26 : 32
    // The header keeps only icons when the view is narrow.
    readonly property bool narrowHeader: width < 700
    readonly property int laneWidth: commitList.compact ? 16 : 20
    readonly property int refsWidth: Math.min(180, width * 0.22)
    // The message keeps at least this much space. The optional columns and
    // then the graph give way when the view is narrow.
    readonly property int messageMinWidth: 220
    readonly property int available: width - 20 - refsWidth
    // Wide enough for the column title.
    readonly property int graphWidth: commitList.laneCount > 0
        ? Math.max(64, Math.min(commitList.laneCount * laneWidth + 6,
                                Math.max(laneWidth * 3, available * 0.28))) : 0
    readonly property int spare: available - graphWidth - messageMinWidth
    readonly property int authorWidth: commitList.showAuthor && spare >= 140 ? 140 : 0
    readonly property int dateWidth: commitList.showDate && spare - authorWidth >= 96 ? 96 : 0
    readonly property int idWidth:
        commitList.showId && spare - authorWidth - dateWidth >= 68 ? 68 : 0

    color: Theme.base

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // Filter and display options.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 40
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                spacing: 4

                // The branches shown alone, with a button to show all again.
                Rectangle {
                    id: soloChip

                    visible: !commitList.filtered && commitList.solo.length > 0
                    implicitWidth: soloRow.implicitWidth + 20
                    implicitHeight: 26
                    radius: 13
                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14)
                    border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.7)

                    HoverHandler {
                        id: soloHover
                    }

                    HoverTip {
                        target: soloChip
                        text: qsTr("Only the soloed branches are shown:") + "<br>"
                              + commitList.solo.map((name) => name.replace(/^refs\/(heads|remotes)\//, ""))
                                                .join("<br>")
                        hovered: soloHover.hovered && !unsoloMouse.containsMouse
                    }

                    Row {
                        id: soloRow

                        anchors.centerIn: parent
                        spacing: 6

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "solo"
                            size: 13
                            color: Theme.accent
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: qsTr("Solo: %1").arg(commitList.soloText)
                            color: Theme.accent
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Rectangle {
                            id: unsolo

                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18
                            radius: 9
                            color: unsoloMouse.containsMouse
                                   ? Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.25)
                                   : "transparent"

                            Icon {
                                anchors.centerIn: parent
                                name: "close"
                                size: 10
                                color: Theme.accent
                            }

                            MouseArea {
                                id: unsoloMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: commitList.unsoloAll()
                                onExited: host.hideToolTip()
                            }

                            HoverTip {
                                target: unsolo
                                text: qsTr("Unsolo all branches")
                                hovered: unsoloMouse.containsMouse
                            }
                        }
                    }
                }

                // The hidden branches, with a button to show them again.
                Rectangle {
                    id: hiddenChip

                    visible: !commitList.filtered && commitList.solo.length === 0
                             && commitList.hidden.length > 0
                    implicitWidth: hiddenRow.implicitWidth + 20
                    implicitHeight: 26
                    radius: 13
                    color: Theme.hover
                    border.color: Theme.border

                    HoverHandler {
                        id: hiddenHover
                    }

                    HoverTip {
                        target: hiddenChip
                        text: qsTr("Hidden in the graph:") + "<br>"
                              + commitList.hidden.map((name) => name.replace(/^refs\/(heads|remotes)\//, "")
                                                                   .replace(/\/$/, "/*"))
                                                 .join("<br>")
                        hovered: hiddenHover.hovered && !showAllMouse.containsMouse
                    }

                    Row {
                        id: hiddenRow

                        anchors.centerIn: parent
                        spacing: 6

                        Icon {
                            anchors.verticalCenter: parent.verticalCenter
                            name: "eye-off"
                            size: 13
                            color: Theme.textMuted
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: root.narrowHeader ? commitList.hidden.length
                                                    : qsTr("Hidden: %1").arg(commitList.hiddenText)
                            color: Theme.text
                            font.pixelSize: 12
                            font.bold: true
                        }

                        Rectangle {
                            id: showAll

                            anchors.verticalCenter: parent.verticalCenter
                            width: 18
                            height: 18
                            radius: 9
                            color: showAllMouse.containsMouse ? Theme.pressed : "transparent"

                            Icon {
                                anchors.centerIn: parent
                                name: "close"
                                size: 10
                                color: Theme.textMuted
                            }

                            MouseArea {
                                id: showAllMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: commitList.showAll()
                                onExited: host.hideToolTip()
                            }

                            HoverTip {
                                target: showAll
                                text: qsTr("Show all hidden branches")
                                hovered: showAllMouse.containsMouse
                            }
                        }
                    }
                }

                MenuButton {
                    visible: !commitList.filtered && commitList.solo.length === 0
                    icon: "branch"
                    text: root.narrowHeader ? "" : commitList.refsFilterName
                    tip: qsTr("Which branches to show")
                    onMenuRequested: (x, y) => commitList.showRefsFilterMenu(x, y)
                }

                MenuButton {
                    visible: !commitList.filtered
                    icon: "sort"
                    text: root.narrowHeader ? "" : commitList.sortName
                    tip: qsTr("Commit order")
                    onMenuRequested: (x, y) => commitList.showSortMenu(x, y)
                }

                Text {
                    visible: commitList.filtered
                    Layout.leftMargin: 6
                    text: qsTr("Search results")
                    color: Theme.accent
                    font.pixelSize: 12
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Spinner {
                    running: commitList.loading
                    size: 16
                }

                FilterField {
                    id: pathField

                    // Shrink to keep the buttons in narrow views.
                    Layout.fillWidth: true
                    Layout.minimumWidth: 90
                    Layout.maximumWidth: Math.min(260, root.width * 0.35)
                    icon: "filter"
                    placeholder: qsTr("Filter by path")
                    text: repoView.pathspec
                    onEdited: pathTimer.restart()
                    onAccepted: {
                        pathTimer.stop()
                        repoView.setPathspec(text)
                    }

                    Timer {
                        id: pathTimer

                        interval: 400
                        onTriggered: repoView.setPathspec(pathField.text)
                    }
                }

                ActionButton {
                    compact: true
                    icon: "folder"
                    tip: qsTr("Choose a path")
                    checked: pathPopup.opened
                    onClicked: pathPopup.opened ? pathPopup.close() : pathPopup.open()
                }

                ActionButton {
                    compact: true
                    icon: "settings"
                    hasMenu: true
                    menuOnly: true
                    tip: qsTr("Graph options")
                    onMenuRequested: (x, y) => commitList.showSettingsMenu(x, y)
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.border
            }
        }

        // Column titles.
        Item {
            Layout.fillWidth: true
            implicitHeight: 24

            Row {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 12

                Repeater {
                    model: [
                        { title: qsTr("Branch / Tag"), width: root.refsWidth },
                        { title: qsTr("Graph"), width: root.graphWidth },
                        { title: qsTr("Commit message"),
                          width: root.width - 20 - root.refsWidth - root.graphWidth
                                 - root.authorWidth - root.dateWidth - root.idWidth },
                        { title: qsTr("Author"), width: root.authorWidth },
                        { title: qsTr("Date"), width: root.dateWidth },
                        { title: qsTr("SHA"), width: root.idWidth }
                    ]

                    Text {
                        required property var modelData

                        visible: modelData.width > 0
                        width: modelData.width
                        height: parent.height
                        leftPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        text: modelData.title
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.pixelSize: 10
                        font.bold: true
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 0.6
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.border
            }
        }

        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            boundsBehavior: Flickable.StopAtBounds
            model: commitList.model
            reuseItems: true
            cacheBuffer: 400

            onAtYEndChanged: {
                if (atYEnd)
                    commitList.fetchMore()
            }

            Keys.onUpPressed: commitList.selectCommitRelative(-1)
            Keys.onDownPressed: commitList.selectCommitRelative(1)

            Connections {
                target: commitList

                function onScrollRequested(row) {
                    listView.positionViewAtIndex(row, ListView.Contain)
                }
            }

            Controls.ScrollBar.vertical: ThinScrollBar {}

            delegate: Item {
                id: row

                required property int index
                required property var display
                required property var graph
                required property var graphColors
                required property var isStatus
                required property var summary
                required property var author
                required property var initials
                required property var date
                required property var shortId
                required property var starred
                required property var refs
                required property var nodeColor
                required property var isMerge
                required property var busy
                required property var wip

                readonly property bool selected:
                    commitList.selectionRevision >= 0 && commitList.isSelected(index)
                readonly property bool hovered: mouse.containsMouse || starMouse.containsMouse
                readonly property color laneColor: nodeColor ? nodeColor : Theme.accent
                // Selected, and not the target of a dragged branch.
                readonly property bool highlighted: selected && !drop.containsDrag
                readonly property color foreground: highlighted ? Theme.selectedText : Theme.text
                // The branch that references dragged onto this row are dropped onto.
                readonly property string dropRef: {
                    const refs = row.refs || []
                    const local = refs.find((ref) => ref.local && ref.qualified)
                    if (local)
                        return local.qualified
                    const remote = refs.find((ref) => ref.remote && ref.qualified)
                    return remote ? remote.qualified : ""
                }

                width: ListView.view.width
                height: root.rowHeight

                Rectangle {
                    anchors.fill: parent
                    color: drop.containsDrag ? Qt.rgba(Theme.accent.r, Theme.accent.g,
                                                       Theme.accent.b, 0.18)
                           : row.selected ? Theme.selected
                           : row.hovered ? Theme.hover : "transparent"
                    border.width: drop.containsDrag ? 1 : 0
                    border.color: Theme.accent
                }

                // Drop a branch onto the branch of this commit.
                DropArea {
                    id: drop

                    anchors.fill: parent
                    enabled: row.dropRef !== ""
                    keys: ["gittyup/ref"]
                    onEntered: (drag) => drag.accepted = refDrop.accepts(row.dropRef)
                    onDropped: (drop) => {
                        const p = mapToItem(null, drop.x, drop.y)
                        refDrop.drop(row.dropRef, p.x, p.y)
                    }
                }

                MouseArea {
                    id: mouse

                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onPressed: listView.forceActiveFocus()
                    onClicked: (event) => {
                        if (event.button === Qt.RightButton) {
                            const p = mapToItem(null, event.x, event.y)
                            commitList.showContextMenu(row.index, p.x, p.y)
                        } else {
                            commitList.click(row.index, event.modifiers)
                        }
                    }
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 12

                    // References pointing at this commit.
                    Item {
                        width: root.refsWidth
                        height: parent.height
                        clip: true

                        Row {
                            anchors.right: parent.right
                            anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 3

                            RefBadge {
                                id: badge

                                visible: (row.refs || []).length > 0
                                ref: (row.refs || []).length > 0 ? row.refs[0] : ({})
                                laneColor: row.laneColor
                                maxWidth: root.refsWidth - (more.visible ? more.width + 12 : 8)

                                // Drag the branch onto another, like in GitKraken.
                                RefDragArea {
                                    anchors.fill: parent
                                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                                    refName: badge.ref.qualified || ""
                                    cursorShape: refName !== "" ? Qt.OpenHandCursor
                                                                : Qt.ArrowCursor
                                    onPressed: listView.forceActiveFocus()
                                    onClicked: (event) => {
                                        if (dragged)
                                            return
                                        if (event.button === Qt.RightButton) {
                                            const p = mapToItem(null, event.x, event.y)
                                            commitList.showContextMenu(row.index, p.x, p.y)
                                        } else {
                                            commitList.click(row.index, event.modifiers)
                                        }
                                    }
                                }
                            }

                            Rectangle {
                                id: more

                                visible: (row.refs || []).length > 1
                                implicitWidth: moreLabel.implicitWidth + 10
                                implicitHeight: 20
                                radius: 4
                                color: Qt.rgba(row.laneColor.r, row.laneColor.g,
                                               row.laneColor.b, Theme.dark ? 0.3 : 0.18)

                                Text {
                                    id: moreLabel

                                    anchors.centerIn: parent
                                    text: "+" + (row.refs ? row.refs.length - 1 : 0)
                                    color: Theme.text
                                    font.pixelSize: 11
                                    font.bold: true
                                }

                                MouseArea {
                                    id: moreMouse

                                    anchors.fill: parent
                                    hoverEnabled: true
                                    acceptedButtons: Qt.NoButton
                                }

                                HoverTip {
                                    target: more
                                    hovered: moreMouse.containsMouse
                                    text: row.refs ? row.refs.map(r => r.name).join("\n") : ""
                                }
                            }
                        }
                    }

                    CommitGraph {
                        visible: root.graphWidth > 0
                        width: root.graphWidth
                        height: parent.height
                        clip: true
                        columns: row.graph || []
                        colors: row.graphColors || []
                        laneWidth: root.laneWidth
                        initials: row.initials || ""
                        merge: row.isMerge || false
                        status: row.isStatus || false
                        statusColor: Theme.textMuted
                        textColor: "#FFFFFF"
                    }

                    // Message or uncommitted changes.
                    Item {
                        width: parent.width - root.refsWidth - root.graphWidth
                               - root.authorWidth - root.dateWidth - root.idWidth
                        height: parent.height

                        Row {
                            visible: row.isStatus === true
                            anchors.left: parent.left
                            anchors.leftMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 10

                            Rectangle {
                                anchors.verticalCenter: parent.verticalCenter
                                implicitWidth: wipLabel.implicitWidth + 12
                                implicitHeight: 20
                                radius: 4
                                color: "transparent"
                                border.color: Theme.textMuted

                                Text {
                                    id: wipLabel

                                    anchors.centerIn: parent
                                    text: "// WIP"
                                    color: row.foreground
                                    font.pixelSize: 11
                                    font.bold: true
                                }
                            }

                            Spinner {
                                anchors.verticalCenter: parent.verticalCenter
                                running: row.busy === true
                                size: 14
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: row.busy === true
                                text: row.display || ""
                                color: Theme.textMuted
                                font.pixelSize: 12
                                font.italic: true
                            }

                            Repeater {
                                model: row.busy === true || !row.wip ? [] : [
                                    { icon: "pencil", count: row.wip.modified || 0,
                                      color: Theme.modified },
                                    { icon: "plus", count: row.wip.added || 0,
                                      color: Theme.added },
                                    { icon: "minus", count: row.wip.deleted || 0,
                                      color: Theme.deleted }
                                ]

                                Row {
                                    required property var modelData

                                    visible: modelData.count > 0
                                    anchors.verticalCenter: parent.verticalCenter
                                    spacing: 3

                                    Icon {
                                        anchors.verticalCenter: parent.verticalCenter
                                        name: modelData.icon
                                        size: 13
                                        color: modelData.color
                                    }

                                    Text {
                                        anchors.verticalCenter: parent.verticalCenter
                                        text: modelData.count
                                        color: modelData.color
                                        font.pixelSize: 12
                                        font.bold: true
                                    }
                                }
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                visible: row.busy !== true && !!row.wip
                                         && !(row.wip.modified || row.wip.added || row.wip.deleted)
                                text: qsTr("No changes")
                                color: Theme.textMuted
                                font.pixelSize: 12
                                font.italic: true
                            }
                        }

                        Text {
                            visible: row.isStatus !== true
                            anchors.left: parent.left
                            anchors.leftMargin: 6
                            anchors.right: star.left
                            anchors.rightMargin: 6
                            anchors.verticalCenter: parent.verticalCenter
                            text: row.summary || ""
                            elide: Text.ElideRight
                            color: row.foreground
                            font.pixelSize: 13
                        }

                        Icon {
                            id: star

                            visible: row.isStatus !== true && (row.starred === true || row.hovered)
                            anchors.right: parent.right
                            anchors.rightMargin: 4
                            anchors.verticalCenter: parent.verticalCenter
                            name: row.starred ? "star-filled" : "star"
                            size: 14
                            color: row.starred ? Theme.star : Theme.textMuted

                            MouseArea {
                                id: starMouse

                                anchors.fill: parent
                                anchors.margins: -4
                                hoverEnabled: true
                                onClicked: commitList.toggleStar(row.index)
                            }
                        }
                    }

                    Text {
                        visible: root.authorWidth > 0
                        width: root.authorWidth
                        height: parent.height
                        leftPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        text: row.isStatus === true ? "" : (row.author || "")
                        elide: Text.ElideRight
                        color: row.highlighted ? Theme.selectedText : Theme.textMuted
                        font.pixelSize: 12
                    }

                    Text {
                        visible: root.dateWidth > 0
                        width: root.dateWidth
                        height: parent.height
                        leftPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        text: row.isStatus === true ? "" : (row.date || "")
                        elide: Text.ElideRight
                        color: row.highlighted ? Theme.selectedText : Theme.textMuted
                        font.pixelSize: 12
                    }

                    Text {
                        visible: root.idWidth > 0
                        width: root.idWidth
                        height: parent.height
                        leftPadding: 6
                        verticalAlignment: Text.AlignVCenter
                        text: row.isStatus === true ? "" : (row.shortId || "")
                        color: row.highlighted ? Theme.selectedText : Theme.textMuted
                        font.pixelSize: 12
                        font.family: "monospace"
                    }
                }
            }

            // Shown while the first page of commits loads.
            Spinner {
                anchors.centerIn: parent
                running: commitList.loading && listView.count === 0
                size: 28
            }
        }
    }

    // File tree of the repository to choose the path filter from.
    Controls.Popup {
        id: pathPopup

        x: root.width - width - 8
        y: 40
        width: Math.min(360, root.width - 16)
        height: Math.min(420, root.height - 60)
        padding: 1

        background: Rectangle {
            color: Theme.panel
            radius: 8
            border.color: Theme.border
        }

        contentItem: TreeView {
            id: pathTree

            clip: true
            model: repoView.pathModel
            boundsBehavior: Flickable.StopAtBounds
            columnWidthProvider: function () { return pathTree.width }
            onWidthChanged: forceLayout()

            delegate: Item {
                id: node

                required property TreeView treeView
                required property bool expanded
                required property bool hasChildren
                required property int depth
                required property int row
                required property int column
                required property string display
                required property string edit

                implicitWidth: treeView.width
                implicitHeight: 26

                Rectangle {
                    anchors.fill: parent
                    anchors.margins: 2
                    radius: 4
                    color: nodeMouse.containsMouse ? Theme.hover : "transparent"
                }

                MouseArea {
                    id: nodeMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    onClicked: (event) => {
                        if (event.button === Qt.RightButton) {
                            const p = mapToItem(null, event.x, event.y)
                            repoView.showPathContextMenu(node.edit, p.x, p.y)
                            return
                        }

                        repoView.setPathspec(node.edit)
                        if (!node.hasChildren)
                            pathPopup.close()
                    }
                    onDoubleClicked: {
                        if (node.hasChildren)
                            node.treeView.toggleExpanded(node.row)
                    }
                }

                Row {
                    anchors.left: parent.left
                    anchors.leftMargin: 8 + node.depth * 14
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 6

                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        opacity: node.hasChildren ? 1 : 0
                        name: "chevron-right"
                        size: 11
                        color: Theme.textMuted
                        rotation: node.expanded ? 90 : 0

                        MouseArea {
                            anchors.fill: parent
                            anchors.margins: -4
                            enabled: node.hasChildren
                            onClicked: node.treeView.toggleExpanded(node.row)
                        }
                    }

                    Icon {
                        anchors.verticalCenter: parent.verticalCenter
                        name: node.hasChildren ? "folder" : "file"
                        size: 14
                        color: node.hasChildren ? Theme.accent : Theme.textMuted
                    }

                    Text {
                        anchors.verticalCenter: parent.verticalCenter
                        text: node.display
                        color: node.edit === repoView.pathspec ? Theme.accent : Theme.text
                        font.pixelSize: 12
                    }
                }
            }
        }
    }
}
