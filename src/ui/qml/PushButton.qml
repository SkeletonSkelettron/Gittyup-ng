import QtQuick
import Gittyup

// A text button. 'primary' buttons are filled with the accent color, or
// red when they are also 'danger' buttons.
Rectangle {
    id: root

    property string text
    property string icon
    property string tip
    property bool primary: false
    property bool danger: false
    property int minimumWidth: 0

    signal clicked()

    readonly property color fill: danger ? Theme.deleted : Theme.accent
    readonly property color foreground: !enabled ? Theme.textDisabled
                                       : primary ? (danger ? "#ffffff" : Theme.accentText)
                                       : danger ? Theme.deleted : Theme.text

    implicitWidth: Math.max(minimumWidth, row.implicitWidth + 20)
    implicitHeight: 28
    radius: 6
    // Disabled buttons look alike, so the label stays readable.
    readonly property bool filled: primary && enabled

    color: filled ? (mouse.pressed ? Qt.darker(fill, 1.15)
                                   : mouse.containsMouse ? Qt.lighter(fill, 1.1)
                                                         : fill)
                  : (!enabled ? Theme.field
                              : mouse.pressed ? Theme.pressed
                                              : mouse.containsMouse ? Theme.hover : Theme.field)
    border.color: filled ? "transparent" : Theme.border

    MouseArea {
        id: mouse

        anchors.fill: parent
        hoverEnabled: true
        onClicked: root.clicked()
    }

    HoverTip {
        target: root
        text: root.tip
        hovered: mouse.containsMouse
    }

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 6

        Icon {
            visible: root.icon !== ""
            anchors.verticalCenter: parent.verticalCenter
            name: root.icon
            size: 14
            color: root.foreground
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: root.text
            color: root.foreground
            font.pixelSize: 12
            font.bold: root.primary
        }
    }
}
