import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The diff of the selected file. 'detailView.diff' is the C++ DiffModel.
Rectangle {
    id: root

    readonly property var diff: detailView.diff

    // Keep in sync with DiffModel::Kind and git::Index::StagedState.
    readonly property int hunkRow: 0
    readonly property int unstaged: 0
    readonly property int partiallyStaged: 1
    readonly property int staged: 2

    // Fit the code font of the editor settings.
    readonly property int lineHeight: Math.max(20, Math.ceil(metrics.height) + 4)
    readonly property int numberWidth: charWidth * diff.lineNumberWidth + 12
    readonly property int gutterWidth: numberWidth * 2 + (diff.editable ? 22 : 0) + 34
    readonly property real charWidth: metrics.advanceWidth("0")

    // Row of the last line that was clicked, to stage ranges with Shift.
    property int anchorRow: -1

    // The code selected with the mouse, from where it was pressed to where
    // it is, in rows and columns of the lines as they're shown.
    property int selectionAnchorRow: -1
    property int selectionAnchorColumn: 0
    property int selectionRow: -1
    property int selectionColumn: 0

    readonly property bool hasSelection: selectionAnchorRow >= 0
        && (selectionAnchorRow !== selectionRow || selectionAnchorColumn !== selectionColumn)
    readonly property bool selectionForward: selectionAnchorRow < selectionRow
        || (selectionAnchorRow === selectionRow && selectionAnchorColumn <= selectionColumn)
    readonly property int selectionFirstRow: selectionForward ? selectionAnchorRow : selectionRow
    readonly property int selectionFirstColumn: selectionForward ? selectionAnchorColumn
                                                                 : selectionColumn
    readonly property int selectionLastRow: selectionForward ? selectionRow : selectionAnchorRow
    readonly property int selectionLastColumn: selectionForward ? selectionColumn
                                                                : selectionAnchorColumn

    function copySelection() {
        if (hasSelection)
            diff.copy(selectionFirstRow, selectionFirstColumn, selectionLastRow, selectionLastColumn)
    }

    function selectAll() {
        selectionAnchorRow = 0
        selectionAnchorColumn = 0
        selectionRow = listView.count - 1
        selectionColumn = diff.maxLineLength
    }

    color: Theme.base
    focus: visible

    Keys.onEscapePressed: detailView.closeFile()

    // Take copy and select all from the menu bar while the diff has the focus.
    Keys.onShortcutOverride: (event) => {
        event.accepted = (event.matches(StandardKey.Copy) && root.hasSelection)
                         || event.matches(StandardKey.SelectAll)
    }

    Keys.onPressed: (event) => {
        if (event.matches(StandardKey.Copy)) {
            root.copySelection()
            event.accepted = true
        } else if (event.matches(StandardKey.SelectAll)) {
            root.selectAll()
            event.accepted = true
        }
    }

    Connections {
        target: root.diff

        function onDiffChanged() {
            root.selectionAnchorRow = -1
        }
    }

    FontMetrics {
        id: metrics

        font.family: Theme.codeFont
        font.pointSize: Theme.codeFontSize
    }

    // Scroll to the current match of the find bar.
    Connections {
        target: detailView.finder
        enabled: root.visible

        function onCurrentChanged(row, start, length) {
            listView.positionViewAtIndex(row, ListView.Contain)
            const x = root.gutterWidth + start * root.charWidth
            const right = x + length * root.charWidth
            if (right > listView.contentX + listView.width)
                listView.contentX = Math.min(right - listView.width + 40, listView.contentWidth - listView.width)
            else if (x < listView.contentX + root.gutterWidth)
                listView.contentX = Math.max(0, x - root.gutterWidth - 40)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // File header.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 10
                spacing: 8

                ActionButton {
                    compact: true
                    icon: "close"
                    tip: qsTr("Close the diff (Esc)")
                    onClicked: detailView.closeFile()
                }

                StatusBadge {
                    status: root.diff.status
                }

                Text {
                    Layout.fillWidth: true
                    textFormat: Text.StyledText
                    text: {
                        const path = root.diff.path
                        const slash = path.lastIndexOf("/")
                        const name = "<b>" + path.substring(slash + 1) + "</b>"
                        const dir = slash < 0 ? "" : path.substring(0, slash + 1)
                        const old = root.diff.oldPath !== path && root.diff.oldPath !== ""
                                    ? root.diff.oldPath + " → " : ""
                        return old + dir + name
                    }
                    elide: Text.ElideLeft
                    color: Theme.text
                    font.pixelSize: 13
                }

                Text {
                    visible: root.diff.additions > 0
                    text: "+" + root.diff.additions
                    color: Theme.added
                    font.pixelSize: 12
                    font.bold: true
                }

                Text {
                    visible: root.diff.deletions > 0
                    text: "−" + root.diff.deletions
                    color: Theme.deleted
                    font.pixelSize: 12
                    font.bold: true
                }

                MergeModeSwitch {}

                PushButton {
                    visible: root.diff.editable
                    implicitHeight: 26
                    text: qsTr("Discard File")
                    danger: true
                    onClicked: detailView.discardFile(root.diff.path)
                }

                PushButton {
                    visible: root.diff.editable
                    implicitHeight: 26
                    primary: root.diff.stageState !== root.staged
                    text: root.diff.stageState === root.staged ? qsTr("Unstage File")
                                                               : qsTr("Stage File")
                    onClicked: root.diff.setFileStaged(root.diff.stageState !== root.staged)
                }

                ActionButton {
                    compact: true
                    icon: "pencil"
                    hasMenu: true
                    menuOnly: true
                    tip: qsTr("Edit the file")
                    onMenuRequested: (x, y) => root.diff.showEditMenu(-1, x, y)
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

        FindBar {
            Layout.fillWidth: true
            visible: finder.visible
            finder: detailView.finder
        }

        // Binary files, large diffs and the like.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.diff.notice !== ""
                     && root.diff.oldImage === "" && root.diff.newImage === ""

            Column {
                anchors.centerIn: parent
                spacing: 12

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: root.diff.notice
                    color: Theme.textMuted
                    font.pixelSize: 13
                }

                PushButton {
                    anchors.horizontalCenter: parent.horizontalCenter
                    visible: root.diff.canLoadAnyway
                    text: qsTr("Load Anyway")
                    onClicked: root.diff.loadAnyway()
                }
            }
        }

        // Images before and after the change.
        Item {
            id: images

            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.diff.oldImage !== "" || root.diff.newImage !== ""

            readonly property bool both: root.diff.oldImage !== "" && root.diff.newImage !== ""
            readonly property real cardWidth: both ? (width - 120) / 2 : Math.min(width - 48, 640)

            component ImageCard: ColumnLayout {
                id: card

                property string label
                property string source
                property string info
                property color accent

                width: images.cardWidth
                spacing: 8

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Rectangle {
                        implicitWidth: labelText.implicitWidth + 16
                        implicitHeight: 22
                        radius: 11
                        color: Qt.rgba(card.accent.r, card.accent.g, card.accent.b, 0.16)

                        Text {
                            id: labelText

                            anchors.centerIn: parent
                            text: card.label
                            color: card.accent
                            font.pixelSize: 11
                            font.weight: Font.DemiBold
                        }
                    }

                    Text {
                        Layout.fillWidth: true
                        text: card.info
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }
                }

                // The image on a checkerboard for transparency.
                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: Math.min(images.height - 100,
                                                     width * Math.max(0.2, picture.implicitHeight
                                                                      / Math.max(1, picture.implicitWidth)))
                    radius: 8
                    color: Theme.base
                    border.color: Theme.border
                    clip: true

                    Grid {
                        anchors.fill: parent
                        anchors.margins: 1
                        columns: Math.ceil(width / 12)
                        opacity: 0.35

                        Repeater {
                            model: Math.ceil(parent.width / 12) * Math.ceil(parent.height / 12)

                            delegate: Rectangle {
                                required property int index

                                readonly property int columns: Math.ceil(parent.width / 12)

                                width: 12
                                height: 12
                                color: (Math.floor(index / columns) + index % columns) % 2
                                       ? Theme.hover : "transparent"
                            }
                        }
                    }

                    Image {
                        id: picture

                        anchors.fill: parent
                        anchors.margins: 12
                        source: card.source
                        fillMode: Image.PreserveAspectFit
                        smooth: true
                        mipmap: true
                    }
                }
            }

            Row {
                anchors.centerIn: parent
                spacing: 24

                ImageCard {
                    visible: root.diff.oldImage !== ""
                    label: images.both ? qsTr("Before") : qsTr("Deleted")
                    accent: images.both ? Theme.textMuted : Theme.deleted
                    source: root.diff.oldImage
                    info: root.diff.oldImageInfo
                }

                Icon {
                    visible: images.both
                    anchors.verticalCenter: parent.verticalCenter
                    name: "arrow-right"
                    size: 24
                    color: Theme.textMuted
                }

                ImageCard {
                    visible: root.diff.newImage !== ""
                    label: images.both ? qsTr("After") : qsTr("Added")
                    accent: Theme.added
                    source: root.diff.newImage
                    info: root.diff.newImageInfo
                }
            }
        }

        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.diff.notice === ""
            clip: true
            model: root.diff
            reuseItems: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.AutoFlickIfNeeded
            contentWidth: Math.max(width, root.gutterWidth
                                          + root.diff.maxLineLength * root.charWidth + 40)

            Controls.ScrollBar.vertical: ThinScrollBar {
                id: verticalBar
            }

            Controls.ScrollBar.horizontal: ThinScrollBar {
                id: horizontalBar
            }

            delegate: Item {
                id: row

                required property int index
                required property int kind
                required property int hunk
                required property string origin
                required property int oldLine
                required property int newLine
                required property string html
                required property bool staged
                required property bool stageable
                required property string header
                required property int hunkState
                required property int resolution
                required property bool chosen
                required property var diagnostics
                required property var matches

                readonly property bool isHunk: kind === root.hunkRow
                readonly property color background: {
                    switch (origin) {
                    case "+": return Theme.diffAddition
                    case "-": return Theme.diffDeletion
                    case "O": return Theme.diffOurs
                    case "T": return Theme.diffTheirs
                    }
                    return "transparent"
                }

                width: listView.contentWidth
                height: isHunk ? 34 : root.lineHeight

                // Hunk header with the hunk actions.
                Rectangle {
                    visible: row.isHunk
                    anchors.fill: parent
                    anchors.topMargin: row.index > 0 ? 6 : 0
                    color: Theme.panel

                    Rectangle {
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.top: parent.top
                        height: 1
                        color: Theme.border
                    }

                    Text {
                        anchors.left: parent.left
                        anchors.leftMargin: 12
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, listView.width - hunkActions.width - 40)
                        text: row.header
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.family: Theme.codeFont
                        font.pointSize: Math.max(7, Theme.codeFontSize - 1)
                    }

                    Row {
                        id: hunkActions

                        // Stay visible while scrolling horizontally.
                        x: listView.contentX + listView.width - width - 12
                        anchors.verticalCenter: parent.verticalCenter
                        spacing: 6

                        // Conflict resolution.
                        PushButton {
                            visible: root.diff.conflicted
                            implicitHeight: 22
                            text: qsTr("Use Ours")
                            primary: row.resolution === 1
                            onClicked: root.diff.chooseConflict(row.hunk, row.resolution === 1 ? 0 : 1)
                        }

                        PushButton {
                            visible: root.diff.conflicted
                            implicitHeight: 22
                            text: qsTr("Use Theirs")
                            primary: row.resolution === 2
                            onClicked: root.diff.chooseConflict(row.hunk, row.resolution === 2 ? 0 : 2)
                        }

                        PushButton {
                            visible: root.diff.conflicted && row.resolution !== 0
                            implicitHeight: 22
                            text: qsTr("Save")
                            onClicked: root.diff.saveConflict(row.hunk)
                        }

                        PushButton {
                            visible: root.diff.editable && !root.diff.conflicted
                            implicitHeight: 22
                            danger: true
                            text: qsTr("Discard Hunk")
                            onClicked: root.diff.discardHunk(row.hunk)
                        }

                        PushButton {
                            visible: root.diff.editable && !root.diff.conflicted
                            implicitHeight: 22
                            primary: row.hunkState !== root.staged
                            text: row.hunkState === root.staged ? qsTr("Unstage Hunk")
                                                                : qsTr("Stage Hunk")
                            onClicked: root.diff.setHunkStaged(row.hunk,
                                                               row.hunkState !== root.staged)
                        }

                        ActionButton {
                            compact: true
                            icon: "pencil"
                            implicitWidth: 24
                            implicitHeight: 22
                            hasMenu: true
                            menuOnly: true
                            tip: qsTr("Edit the hunk")
                            onMenuRequested: (x, y) => root.diff.showEditMenu(row.hunk, x, y)
                        }
                    }
                }

                // Diff line.
                Rectangle {
                    visible: !row.isHunk
                    anchors.fill: parent
                    color: row.background
                    opacity: row.chosen ? 1 : 0.35

                    // The code can be selected.
                    Item {
                        x: root.gutterWidth
                        width: parent.width - x
                        height: parent.height

                        HoverHandler {
                            cursorShape: Qt.IBeamCursor
                        }
                    }

                    Row {
                        anchors.fill: parent

                        Text {
                            width: root.numberWidth
                            height: parent.height
                            rightPadding: 8
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            text: row.oldLine > 0 ? row.oldLine : ""
                            color: Theme.textMuted
                            font.family: Theme.codeFont
                            font.pointSize: Math.max(7, Theme.codeFontSize - 1)
                        }

                        Text {
                            width: root.numberWidth
                            height: parent.height
                            rightPadding: 8
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            text: row.newLine > 0 ? row.newLine : ""
                            color: Theme.textMuted
                            font.family: Theme.codeFont
                            font.pointSize: Math.max(7, Theme.codeFontSize - 1)
                        }

                        // Stage state of the line.
                        Item {
                            visible: root.diff.editable
                            width: 22
                            height: parent.height

                            Rectangle {
                                visible: row.stageable
                                anchors.centerIn: parent
                                width: 12
                                height: 12
                                radius: 3
                                color: row.staged ? Theme.accent : "transparent"
                                border.color: row.staged ? Theme.accent
                                              : lineMouse.containsMouse ? Theme.text
                                                                        : Theme.textMuted

                                Icon {
                                    visible: row.staged
                                    anchors.centerIn: parent
                                    name: "check"
                                    size: 10
                                    color: Theme.accentText
                                }
                            }

                            MouseArea {
                                id: lineMouse

                                anchors.fill: parent
                                enabled: row.stageable
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: (event) => {
                                    if ((event.modifiers & Qt.ShiftModifier) && root.anchorRow >= 0) {
                                        root.diff.setLinesStaged(Math.min(root.anchorRow, row.index),
                                                                 Math.max(root.anchorRow, row.index),
                                                                 !row.staged)
                                    } else {
                                        root.diff.toggleLine(row.index)
                                    }
                                    root.anchorRow = row.index
                                }
                            }
                        }

                        // The diagnostics of plugins.
                        Item {
                            id: diagnosticItem

                            readonly property int worst: {
                                let kind = -1
                                for (const diag of row.diagnostics)
                                    kind = Math.max(kind, diag.kind)
                                return kind
                            }

                            width: 18
                            height: parent.height

                            Icon {
                                visible: diagnosticItem.worst >= 0
                                anchors.centerIn: parent
                                name: diagnosticItem.worst === 0 ? "info" : "warning"
                                size: 13
                                color: diagnosticItem.worst === 2 ? Theme.deleted
                                       : diagnosticItem.worst === 1 ? Theme.modified : Theme.accent
                            }

                            MouseArea {
                                id: diagnosticMouse

                                anchors.fill: parent
                                enabled: diagnosticItem.worst >= 0
                                hoverEnabled: true
                            }

                            HoverTip {
                                target: diagnosticItem
                                text: row.diagnostics.map(diag => diag.message
                                    + (diag.description ? " - " + diag.description : "")).join("\n")
                                hovered: diagnosticMouse.containsMouse
                            }
                        }

                        Text {
                            width: 16
                            height: parent.height
                            horizontalAlignment: Text.AlignHCenter
                            verticalAlignment: Text.AlignVCenter
                            text: row.origin === "+" || row.origin === "-" ? row.origin : ""
                            color: row.origin === "+" ? Theme.added : Theme.deleted
                            font.family: Theme.codeFont
                            font.pointSize: Theme.codeFontSize
                            font.bold: true
                        }

                        Item {
                            width: codeText.implicitWidth
                            height: parent.height

                            // The selected part of the line.
                            Rectangle {
                                readonly property int first: row.index === root.selectionFirstRow
                                                             ? root.selectionFirstColumn : 0

                                visible: root.hasSelection && row.index >= root.selectionFirstRow
                                         && row.index <= root.selectionLastRow
                                x: first * root.charWidth
                                width: row.index === root.selectionLastRow
                                       ? (root.selectionLastColumn - first) * root.charWidth
                                       : Math.max(0, codeText.implicitWidth - x) + root.charWidth
                                height: parent.height
                                color: Theme.accent
                                opacity: 0.3
                            }

                            FindMatches {
                                matches: row.matches
                                charWidth: root.charWidth
                            }

                            Text {
                                id: codeText

                                height: parent.height
                                verticalAlignment: Text.AlignVCenter
                                textFormat: Text.RichText
                                text: row.html
                                color: Theme.text
                                font.family: Theme.codeFont
                                font.pointSize: Theme.codeFontSize
                                font.strikeout: !row.chosen
                            }
                        }
                    }
                }
            }
        }
    }

    // Select the code with the mouse. It lies over the code of the list, so
    // the gutter, the hunk headers and the scroll bars keep their own mouse.
    MouseArea {
        id: selectionMouse

        // Where the mouse is, to extend the selection while scrolling.
        property point last

        // The row and column at 'x', 'y'. Rows above and below the list are
        // the first and last visible ones until it scrolls to them.
        function position(x, y) {
            const p = mapToItem(listView.contentItem, x, Math.max(0, Math.min(y, height - 1)))
            let row = listView.indexAt(0, p.y)
            if (row < 0)
                row = listView.count - 1
            const column = Math.max(0, Math.round((p.x - root.gutterWidth) / root.charWidth))
            return { row: row, column: column }
        }

        function extend() {
            const p = position(last.x, last.y)
            root.selectionRow = p.row
            root.selectionColumn = p.column
        }

        x: listView.x + Math.max(0, root.gutterWidth - listView.contentX)
        y: listView.y
        width: listView.x + listView.width - x - verticalBar.width
        height: listView.height - horizontalBar.height
        visible: listView.visible

        onPressed: (mouse) => {
            const p = position(mouse.x, mouse.y)
            const item = listView.itemAtIndex(p.row)
            if (!item || item.isHunk) {
                mouse.accepted = false
                return
            }

            root.forceActiveFocus()
            if (!(mouse.modifiers & Qt.ShiftModifier) || root.selectionAnchorRow < 0) {
                root.selectionAnchorRow = p.row
                root.selectionAnchorColumn = p.column
            }

            last = Qt.point(mouse.x, mouse.y)
            extend()
        }

        onPositionChanged: (mouse) => {
            last = Qt.point(mouse.x, mouse.y)
            extend()
            autoScroll.running = mouse.y < 0 || mouse.y > height
        }

        onReleased: autoScroll.stop()
        onCanceled: autoScroll.stop()

        // Scroll while the mouse is above or below the list.
        Timer {
            id: autoScroll

            interval: 30
            repeat: true
            onTriggered: {
                const y = selectionMouse.last.y
                const step = (y < 0 ? y : y - selectionMouse.height) / 2
                const bottom = listView.originY + listView.contentHeight - listView.height
                listView.contentY = Math.max(listView.originY,
                                             Math.min(listView.contentY + step, bottom))
                selectionMouse.extend()
            }
        }
    }
}
