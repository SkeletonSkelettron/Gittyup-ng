import QtQuick
import QtQuick.Layouts
import Gittyup

// The gutter of a line of a file: the commit that last changed it and its
// number. 'blame' is the C++ FileViewModel of the file.
Rectangle {
    id: root

    property var blame
    property int blameWidth
    property int numberWidth
    // The commit of the lines under the mouse, shared by the rows.
    property string hoveredCommit
    // The height of the first line of text, which is less than the height
    // of a wrapped line.
    property real textHeight: height

    property int number
    property string blameId
    property bool blameFirst
    property bool blameLast
    property int blameOffset: -1
    property bool blameCommitted
    property string blameSummary
    property string blameAuthor
    property string blameDate
    property string blameColor
    property string blameTip

    // Hovering over the lines of a commit.
    signal hoverRequested(string id)

    width: blameWidth + numberWidth
    color: Theme.base

    // The commit of the line.
    Rectangle {
        id: cell

        visible: root.blameWidth > 0
        width: root.blameWidth
        height: parent.height
        color: root.blameId !== "" && root.blameId === root.blame.selectedCommit
               ? Theme.selected
               : root.blameOffset >= 0 && root.blameId === root.hoveredCommit
                 ? Theme.hover : Theme.panel

        // The age of the commit, from cold to hot.
        Rectangle {
            visible: root.blameOffset >= 0
            x: 3
            y: root.blameFirst ? 3 : 0
            width: 3
            height: parent.height - (root.blameFirst ? 3 : 0) - (root.blameLast ? 3 : 0)
            radius: 1.5
            color: root.blameColor !== "" ? root.blameColor : Theme.textDisabled
        }

        RowLayout {
            visible: root.blameFirst || (root.blameOffset === 1 && root.blameCommitted)
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: root.textHeight
            anchors.leftMargin: 14
            anchors.rightMargin: 8
            spacing: 8

            Text {
                Layout.fillWidth: true
                text: root.blameFirst ? root.blameSummary : root.blameAuthor
                elide: Text.ElideRight
                color: root.blameFirst && root.blameCommitted ? Theme.text : Theme.textMuted
                font.pixelSize: root.blameFirst ? 12 : 11
            }

            Text {
                visible: root.blameFirst && root.blameDate !== ""
                text: root.blameDate
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }

        Rectangle {
            visible: root.blameLast
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            height: 1
            color: Theme.border
        }

        MouseArea {
            id: mouse

            anchors.fill: parent
            hoverEnabled: true
            onContainsMouseChanged: {
                if (containsMouse)
                    root.hoverRequested(root.blameId)
                else if (root.hoveredCommit === root.blameId)
                    root.hoverRequested("")
            }
            onClicked: {
                if (root.blameCommitted)
                    root.blame.selectedCommit =
                        root.blame.selectedCommit === root.blameId ? "" : root.blameId
            }
            onDoubleClicked: {
                if (root.blameCommitted)
                    root.blame.showCommit(root.blameId)
            }
            onExited: host.hideToolTip()
        }

        HoverTip {
            target: cell
            text: root.blameTip
            hovered: mouse.containsMouse
        }
    }

    Text {
        x: root.blameWidth
        width: root.numberWidth
        height: root.textHeight
        rightPadding: 8
        horizontalAlignment: Text.AlignRight
        verticalAlignment: Text.AlignVCenter
        text: root.number > 0 ? root.number : ""
        color: Theme.textMuted
        font.family: Theme.codeFont
        font.pointSize: Math.max(7, Theme.codeFontSize - 1)
    }

    Rectangle {
        anchors.right: parent.right
        width: 1
        height: parent.height
        color: Theme.border
    }
}
