import QtQuick
import Gittyup

// A branch or tag label in the commit graph, tinted with the lane color.
Rectangle {
    id: root

    // {name, head, tag, local, remote}
    property var ref: ({})
    property color laneColor: Theme.accent
    property real maxWidth: 160

    readonly property string leadingIcon: ref.tag ? "tag"
                                          : ref.head ? "check"
                                          : ref.local ? "laptop" : "cloud"

    implicitWidth: content.implicitWidth + 12
    implicitHeight: 20
    radius: 4
    color: Qt.rgba(laneColor.r, laneColor.g, laneColor.b, Theme.dark ? 0.3 : 0.18)
    border.color: Qt.rgba(laneColor.r, laneColor.g, laneColor.b, 0.85)

    Row {
        id: content

        anchors.left: parent.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        spacing: 4

        Icon {
            anchors.verticalCenter: parent.verticalCenter
            name: root.leadingIcon
            size: 12
            color: Theme.text
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            // Icons and spacing take 16 px each.
            width: Math.max(0, Math.min(implicitWidth,
                                        root.maxWidth - 12 - 16 - (cloud.visible ? 16 : 0)))
            text: root.ref.name || ""
            elide: Text.ElideMiddle
            color: Theme.text
            font.pixelSize: 11
            font.bold: root.ref.head || false
        }

        // A local branch that is also on a remote.
        Icon {
            id: cloud

            visible: (root.ref.local && root.ref.remote) || false
            anchors.verticalCenter: parent.verticalCenter
            name: "cloud"
            size: 12
            color: Theme.textMuted
        }
    }
}
