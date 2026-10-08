import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// A check box with a label.
Controls.CheckBox {
    id: control

    spacing: 8
    padding: 0
    font.pixelSize: 13

    indicator: Rectangle {
        x: control.leftPadding
        y: (control.height - height) / 2
        width: 16
        height: 16
        radius: 4
        color: control.checked ? Theme.accent : Theme.field
        border.color: control.checked ? Theme.accent
                                      : control.hovered ? Theme.text : Theme.textMuted
        opacity: control.enabled ? 1 : 0.5

        Icon {
            anchors.centerIn: parent
            visible: control.checked
            name: "check"
            size: 12
            color: Theme.accentText
        }
    }

    contentItem: Text {
        leftPadding: control.indicator.width + control.spacing
        verticalAlignment: Text.AlignVCenter
        text: control.text
        wrapMode: Text.Wrap
        color: control.enabled ? Theme.text : Theme.textDisabled
        font: control.font
    }
}
