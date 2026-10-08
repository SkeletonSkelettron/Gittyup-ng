import QtQuick
import Gittyup

// A flat button with a label and a chevron that opens a menu.
Item {
    id: root

    property string text
    property string icon
    property string tip

    // Scene coordinates of the point where the menu should open.
    signal menuRequested(real x, real y)

    implicitWidth: row.implicitWidth + 16
    implicitHeight: 26

    Rectangle {
        anchors.fill: parent
        radius: 5
        color: mouse.pressed ? Theme.pressed : mouse.containsMouse ? Theme.hover : "transparent"
    }

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            const p = root.mapToItem(null, 0, root.height + 2)
            root.menuRequested(p.x, p.y)
        }
    }

    HoverTip {
        target: root
        text: root.tip
        hovered: mouse.containsMouse
    }

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 5

        Icon {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            name: root.icon
            size: 14
            color: Theme.textMuted
        }

        Text {
            visible: root.text !== ""
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: Theme.text
            font.pixelSize: 12
        }

        Icon {
            visible: root.text !== ""
            anchors.verticalCenter: parent.verticalCenter
            name: "chevron-down"
            size: 11
            color: Theme.textMuted
        }
    }
}
