import QtQuick
import Gittyup

// A single line text field with an icon, a placeholder and a clear button.
Rectangle {
    id: root

    property alias text: input.text
    property string placeholder
    property string icon: "search"
    property bool showIcon: true
    readonly property bool focused: input.activeFocus

    signal edited(string text)
    signal accepted()
    // Shift+Enter.
    signal acceptedBackward()
    // Escape or the clear button.
    signal cleared()

    function clear() {
        input.clear()
        root.edited("")
        root.cleared()
    }

    function focusInput() {
        input.forceActiveFocus()
    }

    // Let the input accept plain Enter itself.
    function handleReturn(event) {
        if (event.modifiers & Qt.ShiftModifier)
            root.acceptedBackward()
        else
            event.accepted = false
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
        visible: root.showIcon
        name: root.icon
        size: 14
        color: Theme.textMuted
    }

    TextInput {
        id: input

        anchors.left: root.showIcon ? leading.right : parent.left
        anchors.leftMargin: root.showIcon ? 6 : 8
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
        Keys.onReturnPressed: (event) => root.handleReturn(event)
        Keys.onEnterPressed: (event) => root.handleReturn(event)

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
