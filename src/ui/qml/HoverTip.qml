import QtQuick

// Shows the tool tip of 'target' after hovering over it for a while. The
// host draws it in the view, or shows a native one. Assign 'hovered' from
// the target's MouseArea.
Timer {
    id: root

    property Item target
    property string text
    property bool hovered: false

    interval: 650
    running: hovered && text !== ""
    onTriggered: {
        const p = target.mapToItem(null, 0, 0)
        host.showToolTip(text, p.x, p.y, target.width, target.height)
    }
    onHoveredChanged: {
        if (!hovered)
            host.hideToolTip()
    }
    // Items like the rows of lists go away while the mouse is over them.
    Component.onDestruction: {
        if (hovered)
            host.hideToolTip()
    }
}
