import QtQuick
import Gittyup

// Indeterminate progress indicator.
Icon {
    id: root

    property bool running: true

    name: "spinner"
    color: Theme.textMuted
    visible: running

    RotationAnimator on rotation {
        running: root.running && root.visible
        loops: Animation.Infinite
        from: 0
        to: 360
        duration: 900
    }
}
