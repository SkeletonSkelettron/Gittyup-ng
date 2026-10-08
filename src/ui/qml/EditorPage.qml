import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// A text editor with the commit that last changed each line on the left.
// 'editor' is the C++ FileEditor and 'editorWindow' its window.
Rectangle {
    id: root

    readonly property var blame: editor.blame
    readonly property real charWidth: metrics.advanceWidth("0")
    readonly property int numberWidth: charWidth * Math.max(2, String(area.lineCount).length) + 16
    readonly property bool showBlame: blame.hasBlame || blame.blameLoading
    readonly property int blameWidth: showBlame ? 300 : 0

    // The commit of the lines under the mouse.
    property string hoveredCommit
    // Changes when the lines of the blame change.
    property int blameRevision: 0

    // The top of 'row' in the text area.
    function rowY(row) {
        // Follow the layout of the text.
        area.contentHeight
        area.width
        if (row > editor.row(area.length))
            return area.contentHeight
        return area.positionToRectangle(editor.position(row, 0)).y
    }

    Connections {
        target: root.blame

        function onModelReset() { root.blameRevision++ }
        function onDataChanged() { root.blameRevision++ }
        function onBlameChanged() { root.blameRevision++ }
    }

    // Scroll the cursor into view, near the top when jumping to a line.
    function showCursor(top) {
        const rect = area.cursorRectangle
        const maxY = Math.max(0, flick.contentHeight - flick.height)
        if (top)
            flick.contentY = Math.max(0, Math.min(rect.y - flick.height / 3, maxY))
        else if (rect.y < flick.contentY)
            flick.contentY = rect.y
        else if (rect.y + rect.height > flick.contentY + flick.height)
            flick.contentY = Math.min(rect.y + rect.height - flick.height, maxY)

        if (rect.x < flick.contentX + 20)
            flick.contentX = Math.max(0, rect.x - 40)
        else if (rect.x > flick.contentX + flick.width - 20)
            flick.contentX = rect.x - flick.width + 80
    }

    color: Theme.base

    FontMetrics {
        id: metrics

        font: area.font
    }

    Connections {
        target: editor

        function onCursorRequested(position) {
            area.cursorPosition = position
            area.forceActiveFocus()
            root.showCursor(true)
        }
    }

    // Select the current match of the find bar.
    Connections {
        target: editor.finder

        function onCurrentChanged(row, start, length) {
            const position = editor.position(row, start)
            area.select(position, position + length)
            root.showCursor(false)
        }
    }

    ToolTipPopup {}

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        MenuBarRow {
            Layout.fillWidth: true
            target: editorWindow
        }

        FindBar {
            Layout.fillWidth: true
            visible: finder.visible
            finder: editor.finder
        }

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            // The gutter of the lines on the screen, placed like the lines of
            // the text area, which may wrap.
            Item {
                id: gutter

                readonly property int firstRow: {
                    area.contentHeight
                    return editor.row(area.positionAt(0, flick.contentY))
                }
                readonly property int lastRow: {
                    area.contentHeight
                    return editor.row(area.positionAt(0, flick.contentY + flick.height))
                }

                width: root.blameWidth + root.numberWidth
                height: parent.height
                clip: true

                Rectangle {
                    anchors.fill: parent
                    color: Theme.base
                }

                Repeater {
                    model: Math.max(0, gutter.lastRow - gutter.firstRow + 1)

                    delegate: BlameGutter {
                        id: cell

                        required property int index
                        readonly property int row: gutter.firstRow + index
                        readonly property var line: {
                            root.blameRevision
                            return root.blame.line(row)
                        }
                        readonly property real rowTop: root.rowY(row)

                        y: rowTop - flick.contentY
                        height: Math.max(1, root.rowY(row + 1) - rowTop)
                        textHeight: area.cursorRectangle.height
                        blame: root.blame
                        blameWidth: root.blameWidth
                        numberWidth: root.numberWidth
                        hoveredCommit: root.hoveredCommit
                        number: line.number
                        blameId: line.blameId
                        blameFirst: line.blameFirst
                        blameLast: line.blameLast
                        blameOffset: line.blameOffset
                        blameCommitted: line.blameCommitted
                        blameSummary: line.blameSummary
                        blameAuthor: line.blameAuthor
                        blameDate: line.blameDate
                        blameColor: line.blameColor
                        blameTip: line.blameTip
                        onHoverRequested: (id) => root.hoveredCommit = id
                    }
                }

                // Scroll wheel over the gutter scrolls the text.
                WheelHandler {
                    onWheel: (event) => {
                        const maxY = Math.max(0, flick.contentHeight - flick.height)
                        flick.contentY = Math.max(0, Math.min(flick.contentY - event.angleDelta.y,
                                                              maxY))
                    }
                }
            }

            Flickable {
                id: flick

                anchors.left: gutter.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                Controls.ScrollBar.vertical: ThinScrollBar {}

                Controls.ScrollBar.horizontal: ThinScrollBar {}

                Controls.TextArea.flickable: Controls.TextArea {
                    id: area

                    textFormat: TextEdit.PlainText
                    wrapMode: editor.wrapLines ? TextEdit.Wrap : TextEdit.NoWrap
                    readOnly: editor.readOnly
                    selectByMouse: true
                    persistentSelection: true
                    leftPadding: 8
                    rightPadding: 8
                    topPadding: 0
                    bottomPadding: 0
                    color: Theme.text
                    selectionColor: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.35)
                    selectedTextColor: Theme.text
                    font.family: Theme.codeFont
                    font.pointSize: Theme.codeFontSize
                    tabStopDistance: editor.tabWidth * root.charWidth
                    background: null
                    focus: true
                    Component.onCompleted: editor.setDocument(textDocument)

                    // Indent with spaces up to the next indentation stop.
                    Keys.onTabPressed: (event) => {
                        if (editor.useTabs || readOnly) {
                            event.accepted = false
                            return
                        }

                        remove(selectionStart, selectionEnd)
                        const row = editor.row(cursorPosition)
                        const column = cursorPosition - editor.position(row, 0)
                        const width = editor.indentWidth
                        insert(cursorPosition, " ".repeat(width - column % width))
                    }
                }
            }
        }
    }
}
