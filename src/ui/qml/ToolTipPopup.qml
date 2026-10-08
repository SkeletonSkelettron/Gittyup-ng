import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// The tool tip that 'host' shows in a view that draws popups, below the
// item under the mouse, or above it when there's no room below. Put it in
// the root item of the view.
Controls.Popup {
    id: root

    readonly property rect target: host.toolTipRect
    readonly property real sceneWidth: parent ? parent.width : 0
    readonly property real sceneHeight: parent ? parent.height : 0

    x: Math.max(8, Math.min(target.x, sceneWidth - width - 8))
    y: target.y + target.height + 4 + height <= sceneHeight - 8
       ? target.y + target.height + 4 : target.y - height - 4
    width: Math.min(implicitWidth, 440)
    padding: 6
    leftPadding: 9
    rightPadding: 9
    margins: 8
    focus: false
    closePolicy: Controls.Popup.NoAutoClose
    visible: host.toolTipVisible

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0; to: 1; duration: 90 }
    }

    contentItem: Text {
        text: host.toolTipText
        textFormat: Text.AutoText
        wrapMode: Text.Wrap
        color: Theme.tooltipText
        font.pixelSize: 12
    }

    background: Rectangle {
        radius: 6
        color: Theme.tooltip
        border.color: Theme.border
    }
}
