import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// A single line text field.
Controls.TextField {
    id: control

    // Mark the text as invalid.
    property bool error: false

    implicitHeight: 32
    leftPadding: 10
    rightPadding: 10
    color: Theme.text
    placeholderTextColor: Theme.textMuted
    selectionColor: Theme.accent
    selectedTextColor: Theme.accentText
    selectByMouse: true
    font.pixelSize: 13

    background: Rectangle {
        radius: 6
        color: Theme.field
        border.width: control.activeFocus ? 2 : 1
        border.color: control.error ? Theme.deleted
                                    : control.activeFocus ? Theme.accent
                                                          : control.hovered ? Theme.textMuted
                                                                            : Theme.border
    }
}
