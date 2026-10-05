import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The conflicts of a file resolved like in GitKraken: the lines of ours (A)
// and theirs (B) are taken into the output below, which can also be edited.
// 'detailView.merge' is the C++ MergeModel.
Rectangle {
    id: root

    objectName: "mergePanel"

    readonly property var merge: detailView.merge
    readonly property real charWidth: metrics.advanceWidth("0")
    readonly property int lineHeight: Math.max(20, Math.ceil(metrics.height) + 4)
    readonly property int numberWidth: charWidth * merge.lineNumberWidth + 12
    readonly property color sideColor0: Theme.diffOurs
    readonly property color sideColor1: Theme.diffTheirs
    // The bars beside the lines of the conflicts, in the color of their side.
    readonly property color sideBar0: Theme.dark ? "#4C8DFF" : "#2F6FE0"
    readonly property color sideBar1: Theme.dark ? "#B57BFF" : "#9B3FD6"
    readonly property var sideLabels: [merge.oursLabel, merge.theirsLabel]

    // The conflict that the arrows move between.
    property int current: 0

    // The parts of the file, and their tops and ends in the panes of the
    // sides. Every row of a side has the same height.
    readonly property var layout: merge.layout
    readonly property var sideBounds0: bounds(0)
    readonly property var sideBounds1: bounds(1)

    // Scrolling one pane scrolls the others. The pane that the others follow
    // is the one under the mouse, or the output while typing, so that the
    // output following its text cursor doesn't move the sides.
    property bool syncing: false
    property int driver: -1

    // The first conflict is shown when the panes have their size.
    property bool positioned: false

    function sideColor(side) {
        return side === 0 ? root.sideColor0 : root.sideColor1
    }

    function sideBar(side) {
        return side === 0 ? root.sideBar0 : root.sideBar1
    }

    // The background of the lines of a side, stronger when they are taken.
    function sideTint(side, taken) {
        const bar = sideBar(side)
        return taken ? Qt.tint(sideColor(side), Qt.rgba(bar.r, bar.g, bar.b, 0.3))
                     : sideColor(side)
    }

    function bounds(side) {
        const result = [0]
        let y = 0
        for (const part of root.layout) {
            // Conflicts have a header, and a row when the side has no lines.
            const lines = part.lines[side]
            y += (part.conflict >= 0 ? 1 + Math.max(1, lines) : lines) * root.lineHeight
            result.push(y)
        }
        return result
    }

    // The position in a pane with the parts at 'to' and lines of 'toLine'
    // pixels that shows what 'y' shows in a pane with the parts at 'from' and
    // lines of 'fromLine' pixels. Where a part has fewer lines, it waits at
    // its end while the longer one scrolls on, and where it has more lines it
    // catches up.
    function mapY(from, fromLine, to, toLine, y) {
        if (from.length < 2 || from.length !== to.length || fromLine <= 0 || toLine <= 0)
            return y

        let i = 0
        while (i < from.length - 2 && y >= from[i + 1])
            ++i

        const offset = Math.max(0, y - from[i]) / fromLine
        const fromLines = (from[i + 1] - from[i]) / fromLine
        const toLines = (to[i + 1] - to[i]) / toLine
        const lines = toLines <= fromLines ? Math.min(offset, toLines)
                    : fromLines > 0 ? Math.min(offset * toLines / fromLines, toLines) : 0
        return to[i] + lines * toLine
    }

    // Pane 0 and 1 are the sides, and 2 is the output.
    function sync(pane, y, force) {
        if (root.syncing || root.layout.length === 0 || (!force && pane !== root.driver))
            return

        root.syncing = true
        const from = pane === 0 ? root.sideBounds0
                   : pane === 1 ? root.sideBounds1 : output.bounds
        const line = pane === 2 ? output.textLineHeight : root.lineHeight
        if (pane !== 0)
            oursPane.scrollTo(mapY(from, line, root.sideBounds0, root.lineHeight, y))
        if (pane !== 1)
            theirsPane.scrollTo(mapY(from, line, root.sideBounds1, root.lineHeight, y))
        if (pane !== 2)
            output.scrollTo(mapY(from, line, output.bounds, output.textLineHeight, y))
        root.syncing = false
    }

    // The sides scroll sideways together.
    function syncX(pane, x) {
        if (root.syncing || pane !== root.driver)
            return

        root.syncing = true
        if (pane === 0)
            theirsPane.scrollXTo(x)
        else
            oursPane.scrollXTo(x)
        root.syncing = false
    }

    // Show a conflict with two lines before it in all panes.
    function showConflict(index) {
        if (merge.conflictCount === 0)
            return
        current = Math.max(0, Math.min(merge.conflictCount - 1, index))
        const part = root.layout.findIndex((part) => part.conflict === root.current)
        if (part < 0)
            return
        oursPane.scrollTo(root.sideBounds0[part] - 2 * root.lineHeight)
        root.sync(0, oursPane.contentY, true)
        output.placeCursor(root.merge.regionStart(root.current))
    }

    function position() {
        if (root.positioned || !root.visible || oursPane.height <= 0 || output.height <= 0)
            return
        root.positioned = true
        root.showConflict(root.current)
    }

    color: Theme.base
    focus: visible

    // The file can be loaded before the merge editor is shown.
    Component.onCompleted: Qt.callLater(root.position)
    onVisibleChanged: {
        root.positioned = false
        Qt.callLater(root.position)
    }

    Keys.onEscapePressed: detailView.closeFile()

    FontMetrics {
        id: metrics

        font.family: Theme.codeFont
        font.pointSize: Theme.codeFontSize
    }

    // Taking lines changes the output, which the text cursor scrolls, so
    // align it again with the pane that leads.
    function resync() {
        if (!root.positioned)
            return
        const pane = root.driver >= 0 ? root.driver : 0
        root.sync(pane, pane === 0 ? oursPane.contentY
                      : pane === 1 ? theirsPane.contentY : output.contentY, true)
    }

    Connections {
        target: root.merge

        function onRegionsChanged() {
            Qt.callLater(root.resync)
        }

        function onLoaded() {
            // Start at the first conflict.
            root.current = 0
            root.positioned = false
            Qt.callLater(root.position)
        }
    }

    // A letter that marks a side, like in GitKraken.
    component SideBadge: Rectangle {
        property int side

        implicitWidth: 20
        implicitHeight: 20
        radius: 4
        color: Qt.lighter(root.sideColor(side), Theme.dark ? 2.2 : 0.8)

        Text {
            anchors.centerIn: parent
            text: parent.side === 0 ? "A" : "B"
            color: Theme.dark ? Theme.base : "#FFFFFF"
            font.pixelSize: 11
            font.bold: true
        }
    }

    // A check box of a line or of a whole conflict.
    component Check: Rectangle {
        id: check

        // 0 unchecked, 1 partly and 2 checked.
        property int state: 0

        implicitWidth: 14
        implicitHeight: 14
        radius: 3
        color: state > 0 ? Theme.accent : "transparent"
        border.color: state > 0 ? Theme.accent : Theme.textMuted

        Icon {
            anchors.centerIn: parent
            visible: check.state > 0
            name: check.state === 2 ? "check" : "minus"
            size: 10
            color: Theme.accentText
        }
    }

    // The lines of one side with their conflicts.
    component SidePane: Rectangle {
        id: pane

        property int side
        property var model

        readonly property real contentY: list.contentY

        HoverHandler {
            onHoveredChanged: {
                if (hovered)
                    root.driver = pane.side
            }
        }

        function scrollTo(y) {
            // The height of the content is known after the layout.
            list.forceLayout()
            list.contentY = Math.max(0, Math.min(y, list.contentHeight - list.height))
        }

        function scrollXTo(x) {
            list.contentX = Math.max(0, Math.min(x, list.contentWidth - list.width))
        }

        color: Theme.base

        ColumnLayout {
            anchors.fill: parent
            spacing: 0

            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 32
                color: Theme.panel

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 10
                    anchors.rightMargin: 10
                    spacing: 8

                    SideBadge {
                        side: pane.side
                    }

                    Text {
                        Layout.fillWidth: true
                        text: root.sideLabels[pane.side]
                        elide: Text.ElideRight
                        color: Theme.text
                        font.pixelSize: 12
                        font.bold: true
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
                id: list

                objectName: pane.side === 0 ? "mergeOurs" : "mergeTheirs"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                model: pane.model
                reuseItems: true
                boundsBehavior: Flickable.StopAtBounds
                flickableDirection: Flickable.AutoFlickIfNeeded
                contentWidth: Math.max(width, 30 + root.numberWidth + 2000)

                onContentYChanged: root.sync(pane.side, contentY)
                onContentXChanged: root.syncX(pane.side, contentX)
                onHeightChanged: Qt.callLater(root.position)

                Controls.ScrollBar.vertical: ThinScrollBar {}

                Controls.ScrollBar.horizontal: ThinScrollBar {}

                delegate: Rectangle {
                    id: row

                    required property int kind
                    required property int conflict
                    required property int line
                    required property int number
                    required property string html
                    required property bool checked
                    required property int checkState

                    readonly property bool isConflict: kind === 1
                    readonly property bool isLine: kind === 2
                    readonly property bool isEmpty: kind === 3

                    // All rows have the same height, so that the panes can
                    // scroll together.
                    width: list.contentWidth
                    height: root.lineHeight
                    color: isConflict ? Theme.panel
                           : isLine ? root.sideTint(pane.side, checked)
                           : isEmpty ? root.sideTint(pane.side, false)
                           : "transparent"

                    // The lines of a conflict have the bar of their side.
                    Rectangle {
                        visible: row.isConflict || row.isLine || row.isEmpty
                        width: 3
                        height: parent.height
                        color: root.sideBar(pane.side)
                    }

                    // The header of a conflict takes all its lines.
                    RowLayout {
                        visible: row.isConflict
                        anchors.fill: parent
                        anchors.leftMargin: 8
                        spacing: 8

                        Check {
                            state: row.checkState
                        }

                        Text {
                            text: qsTr("Conflict %1").arg(row.conflict + 1)
                            color: row.conflict === root.current ? Theme.accent : Theme.textMuted
                            font.pixelSize: 11
                            font.bold: true
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Row {
                        visible: !row.isConflict
                        anchors.fill: parent

                        Item {
                            width: 30
                            height: parent.height

                            Check {
                                visible: row.isLine
                                anchors.centerIn: parent
                                state: row.checked ? 2 : 0
                            }
                        }

                        Text {
                            width: root.numberWidth
                            height: parent.height
                            rightPadding: 8
                            horizontalAlignment: Text.AlignRight
                            verticalAlignment: Text.AlignVCenter
                            text: row.isEmpty ? "" : row.number
                            color: Theme.textMuted
                            font.family: Theme.codeFont
                            font.pointSize: Math.max(7, Theme.codeFontSize - 1)
                        }

                        Text {
                            height: parent.height
                            verticalAlignment: Text.AlignVCenter
                            textFormat: row.isEmpty ? Text.PlainText : Text.RichText
                            text: row.isEmpty ? qsTr("No lines on this side") : row.html
                            color: row.isEmpty ? Theme.textMuted : Theme.text
                            font.family: Theme.codeFont
                            font.pointSize: Theme.codeFontSize
                            font.italic: row.isEmpty
                        }
                    }

                    // Clicking a line of a conflict or its header takes it.
                    MouseArea {
                        anchors.fill: parent
                        enabled: row.isConflict || row.isLine || row.isEmpty
                        cursorShape: row.isConflict || row.isLine ? Qt.PointingHandCursor
                                                                  : Qt.ArrowCursor
                        onClicked: {
                            root.current = row.conflict
                            if (row.isEmpty)
                                return
                            if (row.isConflict)
                                root.merge.setConflictChecked(pane.side, row.conflict,
                                                              row.checkState !== 2)
                            else
                                root.merge.setLineChecked(pane.side, row.conflict,
                                                          row.line, !row.checked)
                        }
                    }
                }
            }
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
                    tip: qsTr("Close the merge editor (Esc)")
                    onClicked: detailView.closeFile()
                }

                StatusBadge {
                    status: "!"
                }

                Text {
                    Layout.fillWidth: true
                    textFormat: Text.StyledText
                    text: {
                        const path = root.merge.path
                        const slash = path.lastIndexOf("/")
                        return (slash < 0 ? "" : path.substring(0, slash + 1))
                               + "<b>" + path.substring(slash + 1) + "</b>"
                    }
                    elide: Text.ElideLeft
                    color: Theme.text
                    font.pixelSize: 13
                }

                ActionButton {
                    compact: true
                    implicitWidth: 28
                    implicitHeight: 28
                    visible: root.merge.conflictCount > 0
                    enabled: root.current > 0
                    icon: "chevron-up"
                    tip: qsTr("Previous conflict")
                    onClicked: root.showConflict(root.current - 1)
                }

                Text {
                    visible: root.merge.conflictCount > 0
                    text: qsTr("Conflict %1 of %2").arg(root.current + 1)
                                                    .arg(root.merge.conflictCount)
                    color: Theme.textMuted
                    font.pixelSize: 12
                }

                ActionButton {
                    compact: true
                    implicitWidth: 28
                    implicitHeight: 28
                    visible: root.merge.conflictCount > 0
                    enabled: root.current < root.merge.conflictCount - 1
                    icon: "chevron-down"
                    tip: qsTr("Next conflict")
                    onClicked: root.showConflict(root.current + 1)
                }

                PushButton {
                    implicitHeight: 26
                    visible: root.merge.conflictCount > 0
                    text: qsTr("Take All A")
                    tip: qsTr("Take all lines of %1").arg(root.merge.oursLabel)
                    onClicked: root.merge.takeAll(0)
                }

                PushButton {
                    implicitHeight: 26
                    visible: root.merge.conflictCount > 0
                    text: qsTr("Take All B")
                    tip: qsTr("Take all lines of %1").arg(root.merge.theirsLabel)
                    onClicked: root.merge.takeAll(1)
                }

                MergeModeSwitch {}

                PushButton {
                    implicitHeight: 26
                    primary: true
                    enabled: root.merge.notice === ""
                    text: qsTr("Save")
                    tip: qsTr("Save the output and mark the conflicts as resolved")
                    onClicked: root.merge.save()
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

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.merge.notice !== ""

            Text {
                anchors.centerIn: parent
                text: root.merge.notice
                color: Theme.textMuted
                font.pixelSize: 13
            }
        }

        Controls.SplitView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.merge.notice === ""
            orientation: Qt.Vertical

            handle: Rectangle {
                implicitHeight: 5
                color: Controls.SplitHandle.hovered || Controls.SplitHandle.pressed
                       ? Theme.accent : Theme.border
            }

            Controls.SplitView {
                Controls.SplitView.preferredHeight: parent.height * 0.5
                Controls.SplitView.minimumHeight: 120
                orientation: Qt.Horizontal

                handle: Rectangle {
                    implicitWidth: 5
                    color: Controls.SplitHandle.hovered || Controls.SplitHandle.pressed
                           ? Theme.accent : Theme.border
                }

                SidePane {
                    id: oursPane

                    Controls.SplitView.preferredWidth: parent.width / 2
                    Controls.SplitView.minimumWidth: 160
                    side: 0
                    model: root.merge.ours
                }

                SidePane {
                    id: theirsPane

                    Controls.SplitView.fillWidth: true
                    Controls.SplitView.minimumWidth: 160
                    side: 1
                    model: root.merge.theirs
                }
            }

            MergeOutput {
                id: output

                Controls.SplitView.fillHeight: true
                Controls.SplitView.minimumHeight: 120
                merge: root.merge
                layout: root.layout
                lineHeight: root.lineHeight
                numberWidth: root.numberWidth
                // The lines in the output are taken.
                sideColors: [root.sideTint(0, true), root.sideTint(1, true)]
                sideBars: [root.sideBar0, root.sideBar1]
                onScrolled: (y) => root.sync(2, y)
                onActivated: root.driver = 2
                onResized: root.position()
            }
        }
    }
}
