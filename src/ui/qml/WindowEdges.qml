import QtQuick
import Gittyup

// The edges of a window whose view draws its frame: they resize the window
// through the window system, and a line in the color of the borders shows
// where the window ends. 'frame' is the WindowFrame of the window. It goes
// last in the view, over everything else.
Item {
    id: root

    property var frame
    readonly property int grip: 5

    anchors.fill: parent
    z: 1000
    visible: !!frame && frame.custom && !frame.maximized

    Rectangle {
        anchors.fill: parent
        color: "transparent"
        border.width: 1
        border.color: Theme.border
    }

    Grip {
        edges: Qt.LeftEdge
        y: root.grip
        width: root.grip
        height: root.height - 2 * root.grip
        cursorShape: Qt.SizeHorCursor
    }

    Grip {
        edges: Qt.RightEdge
        x: root.width - root.grip
        y: root.grip
        width: root.grip
        height: root.height - 2 * root.grip
        cursorShape: Qt.SizeHorCursor
    }

    Grip {
        edges: Qt.TopEdge
        x: root.grip
        width: root.width - 2 * root.grip
        height: root.grip
        cursorShape: Qt.SizeVerCursor
    }

    Grip {
        edges: Qt.BottomEdge
        x: root.grip
        y: root.height - root.grip
        width: root.width - 2 * root.grip
        height: root.grip
        cursorShape: Qt.SizeVerCursor
    }

    Grip {
        edges: Qt.TopEdge | Qt.LeftEdge
        width: root.grip
        height: root.grip
        cursorShape: Qt.SizeFDiagCursor
    }

    Grip {
        edges: Qt.TopEdge | Qt.RightEdge
        x: root.width - root.grip
        width: root.grip
        height: root.grip
        cursorShape: Qt.SizeBDiagCursor
    }

    Grip {
        edges: Qt.BottomEdge | Qt.LeftEdge
        y: root.height - root.grip
        width: root.grip
        height: root.grip
        cursorShape: Qt.SizeBDiagCursor
    }

    Grip {
        edges: Qt.BottomEdge | Qt.RightEdge
        x: root.width - root.grip
        y: root.height - root.grip
        width: root.grip
        height: root.grip
        cursorShape: Qt.SizeFDiagCursor
    }

    component Grip: MouseArea {
        property int edges

        onPressed: root.frame.startResize(edges)
    }
}
