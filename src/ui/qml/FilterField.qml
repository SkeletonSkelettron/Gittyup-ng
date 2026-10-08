import QtQuick
import Gittyup

// A single line text field with an icon, a placeholder and a clear button.
Rectangle {
    id: root

    property alias text: input.text
    property string placeholder
    property string icon: "search"

    signal edited(string text)
    signal accepted()

    function clear() {
        input.clear()
        root.edited("")
    }

    implicitWidth: 160
    implicitHeight: 28
    radius: 6
    color: Theme.field
    border.color: input.activeFocus ? Theme.accent : Theme.border

    Icon {
        id: leading

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        name: root.icon
        size: 14
        color: Theme.textMuted
    }

    TextInput {
        id: input

        anchors.left: leading.right
        anchors.leftMargin: 6
        anchors.right: clearButton.visible ? clearButton.left : parent.right
        anchors.rightMargin: 6
        anchors.verticalCenter: parent.verticalCenter
        clip: true
        color: Theme.text
        selectionColor: Theme.selected
        selectedTextColor: Theme.selectedText
        selectByMouse: true
        font.pixelSize: 12
        onTextEdited: root.edited(text)
        onAccepted: root.accepted()
        Keys.onEscapePressed: root.clear()

        Text {
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            visible: !input.text && !input.preeditText
            text: root.placeholder
            color: Theme.textMuted
            font: input.font
            elide: Text.ElideRight
        }
    }

    Icon {
        id: clearButton

        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        visible: input.text !== ""
        name: "close"
        size: 12
        color: clearMouse.containsMouse ? Theme.text : Theme.textMuted

        MouseArea {
            id: clearMouse

            anchors.fill: parent
            anchors.margins: -4
            hoverEnabled: true
            cursorShape: Qt.ArrowCursor
            onClicked: root.clear()
        }
    }
}
