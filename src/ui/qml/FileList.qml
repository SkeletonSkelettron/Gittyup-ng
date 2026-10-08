import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// Changed files of a diff. 'list' identifies the model in DetailView.
ListView {
    id: root

    // DetailView::List
    property int list: 0
    // Show buttons to stage (1) or unstage (-1) files, or none (0).
    property int stageAction: 0
    property bool discardable: false

    clip: true
    boundsBehavior: Flickable.StopAtBounds
    reuseItems: true

    Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

    delegate: Item {
        id: row

        required property int index
        required property string name
        required property string dir
        required property string path
        required property string status
        required property int depth
        required property bool isDir
        required property bool expanded

        readonly property bool selected: !isDir && path === detailView.selectedFile
        readonly property bool hovered: mouse.containsMouse || stageMouse.containsMouse
                                        || discardMouse.containsMouse

        width: ListView.view.width
        height: 26

        Rectangle {
            anchors.fill: parent
            anchors.leftMargin: 4
            anchors.rightMargin: 4
            radius: 5
            color: row.selected ? Theme.selected : row.hovered ? Theme.hover : "transparent"
        }

        MouseArea {
            id: mouse

            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onClicked: (event) => {
                if (event.button === Qt.RightButton) {
                    const p = mapToItem(null, event.x, event.y)
                    detailView.showFileMenu(root.list, row.index, p.x, p.y)
                } else {
                    detailView.selectFile(root.list, row.index)
                }
            }
        }

        HoverTip {
            target: row
            text: row.isDir ? "" : row.path
            hovered: mouse.containsMouse
        }

        Row {
            anchors.left: parent.left
            anchors.leftMargin: 10 + row.depth * 14
            anchors.right: actions.left
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6

            Icon {
                visible: row.isDir
                anchors.verticalCenter: parent.verticalCenter
                name: "chevron-right"
                size: 11
                color: Theme.textMuted
                rotation: row.expanded ? 90 : 0
            }

            Icon {
                visible: row.isDir
                anchors.verticalCenter: parent.verticalCenter
                name: "folder"
                size: 14
                color: Theme.textMuted
            }

            StatusBadge {
                visible: !row.isDir
                anchors.verticalCenter: parent.verticalCenter
                status: row.status
            }

            Text {
                id: nameLabel

                anchors.verticalCenter: parent.verticalCenter
                width: Math.min(implicitWidth, parent.width - 30)
                text: row.name
                elide: Text.ElideMiddle
                color: row.selected ? Theme.selectedText : Theme.text
                font.pixelSize: 12
            }

            Text {
                visible: !row.isDir && row.dir !== "" && row.depth === 0
                anchors.verticalCenter: parent.verticalCenter
                width: Math.max(0, parent.width - nameLabel.width - 30)
                text: row.dir
                elide: Text.ElideLeft
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }

        // Stage, unstage and discard buttons shown on hover.
        Row {
            id: actions

            anchors.right: parent.right
            anchors.rightMargin: 10
            anchors.verticalCenter: parent.verticalCenter
            spacing: 6
            opacity: row.hovered ? 1 : 0

            Icon {
                visible: root.discardable
                name: "close"
                size: 13
                color: discardMouse.containsMouse ? Theme.deleted : Theme.textMuted

                MouseArea {
                    id: discardMouse

                    anchors.fill: parent
                    anchors.margins: -4
                    hoverEnabled: true
                    onClicked: detailView.discardFiles(root.list, row.index)
                }

                HoverTip {
                    target: parent
                    text: qsTr("Discard changes")
                    hovered: discardMouse.containsMouse
                }
            }

            Rectangle {
                visible: root.stageAction !== 0
                width: stageLabel.implicitWidth + 12
                height: 18
                radius: 4
                color: stageMouse.containsMouse ? Theme.accent : Theme.hover

                Text {
                    id: stageLabel

                    anchors.centerIn: parent
                    text: root.stageAction > 0 ? qsTr("Stage") : qsTr("Unstage")
                    color: stageMouse.containsMouse ? Theme.accentText : Theme.text
                    font.pixelSize: 11
                }

                MouseArea {
                    id: stageMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: detailView.stageFiles(root.list, row.index, root.stageAction > 0)
                }
            }
        }
    }
}
