import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// A number field with buttons to step the value.
Controls.SpinBox {
    id: control

    implicitWidth: 110
    implicitHeight: 32
    editable: true
    font.pixelSize: 13

    contentItem: TextInput {
        text: control.displayText
        font: control.font
        color: Theme.text
        selectionColor: Theme.accent
        selectedTextColor: Theme.accentText
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
    }

    component StepButton: Rectangle {
        property bool pressed: false
        property bool hovered: false
        property string icon

        implicitWidth: 26
        implicitHeight: control.height - 2
        radius: 5
        color: pressed ? Theme.pressed : hovered ? Theme.hover : "transparent"

        Icon {
            anchors.centerIn: parent
            name: parent.icon
            size: 12
            color: Theme.textMuted
        }
    }

    up.indicator: StepButton {
        x: control.width - width - 1
        y: 1
        icon: "plus"
        pressed: control.up.pressed
        hovered: control.up.hovered
    }

    down.indicator: StepButton {
        x: 1
        y: 1
        icon: "minus"
        pressed: control.down.pressed
        hovered: control.down.hovered
    }

    background: Rectangle {
        radius: 6
        color: Theme.field
        border.width: control.activeFocus ? 2 : 1
        border.color: control.activeFocus ? Theme.accent : Theme.border
    }
}
