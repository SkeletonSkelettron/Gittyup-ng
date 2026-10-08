import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// All files of the selected commit. Double click opens a file.
TreeView {
    id: root

    clip: true
    model: detailView.tree
    boundsBehavior: Flickable.StopAtBounds
    columnWidthProvider: function () { return root.width }
    onWidthChanged: forceLayout()

    Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

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
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            radius: 5
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
                    detailView.showTreeMenu(node.edit, p.x, p.y)
                } else if (node.hasChildren) {
                    node.treeView.toggleExpanded(node.row)
                } else {
                    detailView.selectPath(node.edit)
                }
            }
            onDoubleClicked: {
                if (!node.hasChildren)
                    detailView.openTreeFile(node.edit)
            }
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 10 + node.depth * 14
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                opacity: node.hasChildren ? 1 : 0
                name: "chevron-right"
                size: 11
                color: Theme.textMuted
                rotation: node.expanded ? 90 : 0
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
                color: node.edit === detailView.selectedFile ? Theme.accent : Theme.text
                font.pixelSize: 12
            }
        }
    }
}
