import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The repository page: references on the left, the commit graph or the diff
// of the selected file in the middle and the details on the right. The
// activity log slides in at the bottom. The page keeps the focus of its
// items while another page of the window is shown.
FocusScope {
    id: root

    // Height of the log when it's shown. The user can drag its top edge.
    property real logHeight: 180

    Rectangle {
        anchors.fill: parent
        color: Theme.base
    }

    Controls.SplitView {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: logArea.top
        orientation: Qt.Horizontal

        handle: Item {
            implicitWidth: 5

            Rectangle {
                anchors.horizontalCenter: parent.horizontalCenter
                width: 1
                height: parent.height
                color: Controls.SplitHandle.hovered || Controls.SplitHandle.pressed
                       ? Theme.accent : "transparent"
            }
        }

        RefsPanel {
            visible: !repoView.maximized
            Controls.SplitView.preferredWidth: 240
            Controls.SplitView.minimumWidth: 160
            Controls.SplitView.maximumWidth: 480
        }

        Item {
            Controls.SplitView.fillWidth: true
            Controls.SplitView.minimumWidth: 300

            GraphView {
                anchors.fill: parent
                visible: detailView.selectedFile === "" && !interactiveRebase.active
                         && !pullRequests.active
            }

            DiffPanel {
                anchors.fill: parent
                visible: detailView.selectedFile !== "" && detailView.viewMode !== 1
                         && !(detailView.diff.conflicted && detailView.mergeEditor)
                         && !interactiveRebase.active
            }

            // Conflicts in the merge editor, when it's switched on.
            MergePanel {
                anchors.fill: parent
                visible: detailView.selectedFile !== "" && detailView.viewMode !== 1
                         && detailView.diff.conflicted && detailView.mergeEditor
                         && !interactiveRebase.active
            }

            FileView {
                anchors.fill: parent
                visible: detailView.selectedFile !== "" && detailView.viewMode === 1
                         && !interactiveRebase.active
            }

            // The commits of an interactive rebase, like GitKraken shows them.
            RebasePanel {
                anchors.fill: parent
                visible: interactiveRebase.active
            }

            // A pull request of the references panel.
            PullRequestPanel {
                anchors.fill: parent
                visible: pullRequests.active && !interactiveRebase.active
                         && detailView.selectedFile === ""
            }
        }

        DetailsPanel {
            visible: !repoView.maximized
            Controls.SplitView.preferredWidth: 360
            Controls.SplitView.minimumWidth: 280
            Controls.SplitView.maximumWidth: 640
        }
    }

    Item {
        id: logArea

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: logPanel.visible ? Math.min(root.logHeight, root.height - 160) : 0
        clip: true

        Behavior on height {
            enabled: !resizeHandle.pressed
            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        LogPanel {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: Math.min(root.logHeight, root.height - 160)
        }

        // Drag the top edge to resize the log.
        MouseArea {
            id: resizeHandle

            property real startY
            property real startHeight

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 5
            cursorShape: Qt.SizeVerCursor
            onPressed: (event) => {
                startY = mapToItem(root, event.x, event.y).y
                startHeight = root.logHeight
            }
            onPositionChanged: (event) => {
                const y = mapToItem(root, event.x, event.y).y
                root.logHeight = Math.max(80, Math.min(root.height - 160,
                                                       startHeight + startY - y))
            }
        }
    }

    // The last failure, until the user shows or dismisses it.
    Rectangle {
        id: alertBanner

        readonly property bool shown: logPanel.alert !== ""
        // Keep the text while the banner slides out.
        property string message

        anchors.horizontalCenter: parent.horizontalCenter
        y: shown ? 12 : -height - 12
        z: 50
        visible: y > -height - 12
        width: Math.min(parent.width - 32, alertRow.implicitWidth + 20)
        height: 40
        radius: 8
        color: Theme.alert
        border.color: Theme.deleted

        Behavior on y {
            NumberAnimation { duration: 180; easing.type: Easing.OutCubic }
        }

        Connections {
            target: logPanel

            function onAlertChanged() {
                if (logPanel.alert !== "")
                    alertBanner.message = logPanel.alert
            }
        }

        RowLayout {
            id: alertRow

            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 8
            spacing: 8

            Icon {
                name: "alert"
                size: 16
                color: Theme.alertText
            }

            Text {
                Layout.fillWidth: true
                Layout.maximumWidth: 640
                text: alertBanner.message
                elide: Text.ElideRight
                color: Theme.alertText
                font.pixelSize: 12
                font.weight: Font.Medium
            }

            PushButton {
                implicitHeight: 26
                text: qsTr("Show Details")
                onClicked: logPanel.showAlert()
            }

            ActionButton {
                compact: true
                implicitWidth: 24
                implicitHeight: 24
                icon: "close"
                foregroundOverride: Theme.alertText
                tip: qsTr("Dismiss")
                onClicked: logPanel.dismissAlert()
            }
        }
    }

    // The branch that is dragged onto another one, like in GitKraken.
    Rectangle {
        id: dragChip

        readonly property point pointer: root.mapFromItem(null, refDrop.x, refDrop.y)

        visible: refDrop.active
        x: pointer.x + 14
        y: pointer.y + 10
        z: 100
        implicitWidth: chipRow.implicitWidth + 16
        implicitHeight: 24
        radius: 6
        color: Theme.accent

        Drag.active: refDrop.active
        Drag.keys: ["gittyup/ref"]
        Drag.hotSpot.x: -14
        Drag.hotSpot.y: -10

        Connections {
            target: refDrop

            function onReleased() {
                dragChip.Drag.drop()
            }
        }

        Row {
            id: chipRow

            anchors.centerIn: parent
            spacing: 6

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "branch"
                size: 13
                color: Theme.accentText
            }

            Text {
                anchors.verticalCenter: parent.verticalCenter
                text: refDrop.label
                color: Theme.accentText
                font.pixelSize: 12
                font.bold: true
            }
        }
    }
}
