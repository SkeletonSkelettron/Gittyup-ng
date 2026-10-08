import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The output of the merge editor: the common lines and the lines taken from
// each side, colored like the side they come from. It can be edited.
// 'merge' is the C++ MergeModel.
Rectangle {
    id: root

    objectName: "mergeOutputPane"

    property var merge
    // The parts of the file, like MergeModel.layout.
    property var layout: []
    property int lineHeight
    property int numberWidth
    property var sideColors: []
    property var sideBars: []

    // The tops and ends of the parts of the file in the output: the lines
    // between the conflicts and the lines taken into each conflict.
    readonly property var bounds: {
        area.contentHeight
        area.width
        const regions = root.merge.regions
        const result = [0]
        for (let i = 0; i < root.layout.length; ++i) {
            const part = root.layout[i]
            let end = result[i]
            if (part.conflict >= 0) {
                if (part.conflict < regions.length)
                    end = positionY(regions[part.conflict].end)
            } else {
                // Common lines end where the next conflict starts.
                const next = root.layout[i + 1]
                end = next && next.conflict < regions.length
                      ? positionY(regions[next.conflict].start) : area.contentHeight
            }
            result.push(Math.max(result[i], end))
        }
        return result
    }

    // The height of a line of the text.
    readonly property real textLineHeight: {
        area.contentHeight
        return area.positionToRectangle(0).height
    }

    // Scrolling the output, and using it with the mouse or the keyboard.
    signal scrolled(real y)
    signal activated()
    signal resized()

    readonly property real contentY: flick.contentY

    function scrollTo(y) {
        flick.contentY = Math.max(0, Math.min(y, flick.contentHeight - flick.height))
    }

    // The top of a position of the text.
    function positionY(position) {
        // Follow the layout of the text.
        area.contentHeight
        area.width
        return area.positionToRectangle(position).y
    }

    function rowY(row) {
        area.contentHeight
        area.width
        if (row > root.merge.outputRow(area.length))
            return area.contentHeight
        return area.positionToRectangle(root.merge.outputPosition(row)).y
    }

    // Move the text cursor without selecting.
    function placeCursor(position) {
        area.deselect()
        area.cursorPosition = Math.max(0, Math.min(position, area.length))
    }

    color: Theme.base

    HoverHandler {
        onHoveredChanged: {
            if (hovered)
                root.activated()
        }
    }

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

                Text {
                    text: qsTr("Output")
                    color: Theme.text
                    font.pixelSize: 12
                    font.bold: true
                }

                Item { Layout.fillWidth: true }

                Text {
                    text: root.merge.unresolvedCount === 0
                          ? qsTr("All conflicts have a resolution")
                          : root.merge.unresolvedCount === 1
                            ? qsTr("1 conflict to resolve")
                            : qsTr("%1 conflicts to resolve").arg(root.merge.unresolvedCount)
                    color: root.merge.unresolvedCount === 0 ? Theme.added : Theme.modified
                    font.pixelSize: 12
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

            // The numbers of the lines on the screen.
            Item {
                id: gutter

                readonly property int firstRow: {
                    area.contentHeight
                    return root.merge.outputRow(area.positionAt(0, flick.contentY))
                }
                readonly property int lastRow: {
                    area.contentHeight
                    return root.merge.outputRow(area.positionAt(0, flick.contentY + flick.height))
                }

                width: root.numberWidth + 30
                height: parent.height
                clip: true

                Repeater {
                    model: Math.max(0, gutter.lastRow - gutter.firstRow + 1)

                    delegate: Text {
                        required property int index
                        readonly property int row: gutter.firstRow + index

                        y: root.rowY(row) - flick.contentY
                        width: gutter.width - 1
                        height: area.cursorRectangle.height
                        rightPadding: 8
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter
                        text: row + 1
                        color: Theme.textMuted
                        font.family: Theme.codeFont
                        font.pointSize: Math.max(7, Theme.codeFontSize - 1)
                    }
                }

                Rectangle {
                    anchors.right: parent.right
                    width: 1
                    height: parent.height
                    color: Theme.border
                }

                // The bars of the sides that the lines of the conflicts come
                // from.
                Repeater {
                    model: root.merge.regions

                    delegate: Item {
                        id: bar

                        required property var modelData

                        readonly property real startY: root.positionY(modelData.start) - flick.contentY
                        readonly property real middleY: root.positionY(modelData.middle) - flick.contentY
                        readonly property real endY: root.positionY(modelData.end) - flick.contentY

                        Rectangle {
                            x: gutter.width - 4
                            y: bar.startY
                            width: 3
                            height: Math.max(0, bar.middleY - bar.startY)
                            color: root.sideBars[bar.modelData.first]
                        }

                        Rectangle {
                            x: gutter.width - 4
                            y: bar.middleY
                            width: 3
                            height: Math.max(0, bar.endY - bar.middleY)
                            color: root.sideBars[1 - bar.modelData.first]
                        }
                    }
                }
            }

            Flickable {
                id: flick

                objectName: "mergeOutput"
                anchors.left: gutter.right
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                clip: true
                boundsBehavior: Flickable.StopAtBounds

                onContentYChanged: root.scrolled(contentY)
                onHeightChanged: Qt.callLater(root.resized)

                Controls.ScrollBar.vertical: ThinScrollBar {}

                Controls.ScrollBar.horizontal: ThinScrollBar {}

                Controls.TextArea.flickable: Controls.TextArea {
                    id: area

                    textFormat: TextEdit.PlainText
                    wrapMode: TextEdit.NoWrap
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
                    tabStopDistance: 4 * metrics.advanceWidth("0")
                    background: null
                    Component.onCompleted: root.merge.setDocument(textDocument)
                    // Typing drives the other panes, and the keys still
                    // reach the text.
                    Keys.onPressed: (event) => {
                        root.activated()
                        event.accepted = false
                    }

                    FontMetrics {
                        id: metrics

                        font: area.font
                    }

                    // The lines of the conflicts, behind the text.
                    Repeater {
                        model: root.merge.regions

                        delegate: Item {
                            id: region

                            required property int index
                            required property var modelData

                            readonly property real startY: root.positionY(modelData.start)
                            readonly property real middleY: root.positionY(modelData.middle)
                            readonly property real endY: root.positionY(modelData.end)
                            readonly property bool empty: modelData.end === modelData.start

                            z: -1
                            width: area.width
                            height: area.height

                            Rectangle {
                                visible: region.middleY > region.startY
                                y: region.startY
                                width: parent.width
                                height: region.middleY - region.startY
                                color: root.sideColors[region.modelData.first]
                            }

                            Rectangle {
                                visible: region.endY > region.middleY
                                y: region.middleY
                                width: parent.width
                                height: region.endY - region.middleY
                                color: root.sideColors[1 - region.modelData.first]
                            }

                            // A conflict without lines is a line between the others.
                            Rectangle {
                                visible: region.empty
                                y: region.startY - 1
                                width: parent.width
                                height: 2
                                color: region.modelData.touched ? Theme.textMuted : Theme.modified
                            }

                            Rectangle {
                                visible: region.empty
                                x: parent.width - width - 12
                                y: region.startY - height / 2
                                implicitWidth: markerText.implicitWidth + 12
                                implicitHeight: 16
                                radius: 8
                                color: region.modelData.touched ? Theme.textMuted : Theme.modified

                                Text {
                                    id: markerText

                                    anchors.centerIn: parent
                                    text: region.modelData.touched
                                          ? qsTr("Conflict %1: no lines").arg(region.index + 1)
                                          : qsTr("Conflict %1").arg(region.index + 1)
                                    color: Theme.base
                                    font.pixelSize: 10
                                    font.bold: true
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}
