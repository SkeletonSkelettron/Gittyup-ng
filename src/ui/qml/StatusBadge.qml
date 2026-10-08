import QtQuick
import Gittyup

// A one letter file status: A(dded), M(odified), D(eleted), R(enamed),
// ? (untracked) or ! (conflicted).
Rectangle {
    id: root

    property string status

    readonly property color statusColor: {
        switch (status) {
        case "A":
        case "?":
            return Theme.added
        case "D":
        case "!":
            return Theme.deleted
        case "R":
        case "C":
            return Theme.accent
        }
        return Theme.modified
    }

    implicitWidth: 16
    implicitHeight: 16
    radius: 3
    color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.18)
    border.color: Qt.rgba(statusColor.r, statusColor.g, statusColor.b, 0.6)

    Text {
        anchors.centerIn: parent
        text: root.status === "?" ? "U" : root.status
        color: root.statusColor
        font.pixelSize: 10
        font.bold: true
    }
}
