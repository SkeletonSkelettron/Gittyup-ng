import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// A thin scroll bar that only shows when the content doesn't fit.
Controls.ScrollBar {
    id: bar

    property int thickness: 7

    policy: Controls.ScrollBar.AsNeeded
    visible: size < 1.0

    contentItem: Rectangle {
        implicitWidth: bar.thickness
        implicitHeight: bar.thickness
        radius: bar.thickness / 2
        color: Theme.textMuted
        opacity: bar.pressed ? 0.7 : bar.hovered ? 0.5 : 0.3
    }
}
