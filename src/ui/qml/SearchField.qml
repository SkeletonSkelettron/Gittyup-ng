import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// The search field of the tool bar, with the completions of the word being
// typed and the advanced search below it. 'search' is the C++ SearchField.
Rectangle {
    id: root

    function clear() {
        search.edit("", 0)
    }

    implicitWidth: 220
    implicitHeight: 28
    radius: 6
    enabled: search.enabled
    opacity: enabled ? 1 : 0.5
    color: Theme.field
    border.color: input.activeFocus ? Theme.accent
                                    : hover.hovered ? Theme.textMuted : Theme.border

    HoverHandler {
        id: hover
    }

    Icon {
        id: leading

        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.verticalCenter: parent.verticalCenter
        name: "search"
        size: 14
        color: Theme.textMuted
    }

    TextInput {
        id: input

        objectName: "searchInput"
        anchors.left: leading.right
        anchors.leftMargin: 6
        anchors.right: clearButton.visible ? clearButton.left : advancedButton.left
        anchors.rightMargin: 4
        anchors.verticalCenter: parent.verticalCenter
        clip: true
        text: search.text
        color: Theme.text
        selectionColor: Theme.selected
        selectedTextColor: Theme.selectedText
        selectByMouse: true
        font.pixelSize: 12

        onTextEdited: search.edit(text, cursorPosition)
        onActiveFocusChanged: {
            if (!activeFocus)
                search.hideCompletions()
        }

        Keys.onUpPressed: (event) => event.accepted = search.moveCompletion(-1)
        Keys.onDownPressed: (event) => event.accepted = search.moveCompletion(1)
        Keys.onReturnPressed: (event) => event.accepted = search.acceptCompletion()
        Keys.onEnterPressed: (event) => event.accepted = search.acceptCompletion()
        Keys.onEscapePressed: {
            if (!search.hideCompletions())
                root.clear()
        }

        Connections {
            target: search

            function onCursorRequested(position) {
                input.cursorPosition = position
            }
        }

        Text {
            anchors.fill: parent
            verticalAlignment: Text.AlignVCenter
            visible: !input.text && !input.preeditText
            text: search.placeholderText
            color: Theme.textMuted
            font: input.font
            elide: Text.ElideRight
        }
    }

    component FieldButton: Rectangle {
        id: button

        property string icon
        property string tip

        signal clicked()

        anchors.verticalCenter: parent.verticalCenter
        width: 20
        height: 20
        radius: 4
        color: buttonMouse.pressed ? Theme.pressed
                                   : buttonMouse.containsMouse ? Theme.hover : "transparent"

        Icon {
            anchors.centerIn: parent
            name: button.icon
            size: 12
            color: buttonMouse.containsMouse ? Theme.text : Theme.textMuted
        }

        MouseArea {
            id: buttonMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.ArrowCursor
            onClicked: button.clicked()
            onExited: host.hideToolTip()
        }

        HoverTip {
            target: button
            text: button.tip
            hovered: buttonMouse.containsMouse
        }
    }

    FieldButton {
        id: clearButton

        anchors.right: advancedButton.left
        visible: input.text !== ""
        icon: "close"
        tip: qsTr("Clear")
        onClicked: root.clear()
    }

    FieldButton {
        id: advancedButton

        anchors.right: parent.right
        anchors.rightMargin: 4
        icon: "chevron-down"
        tip: qsTr("Advanced Search")
        onClicked: {
            if (search.advancedVisible)
                search.hideAdvanced()
            else
                search.showAdvanced()
        }
    }

    // The completions keep the focus in the field.
    Controls.Popup {
        x: root.width - width
        y: root.height + 5
        width: root.width
        padding: 0
        focus: false
        closePolicy: Controls.Popup.NoAutoClose
        visible: search.completionsVisible
        background: null

        contentItem: SearchCompletions {}
    }

    Controls.Popup {
        id: advanced

        x: root.width - width
        y: root.height + 5
        width: 400
        // Keep it in the window, and scroll the fields when it's too short.
        height: Math.min(implicitHeight, root.Window.height - 16
                         - root.mapToItem(null, 0, root.height + 5).y)
        padding: 0
        focus: true
        closePolicy: Controls.Popup.CloseOnEscape | Controls.Popup.CloseOnPressOutsideParent
        visible: search.advancedVisible
        background: null
        onClosed: search.hideAdvanced()

        contentItem: Flickable {
            implicitHeight: panel.implicitHeight
            contentHeight: panel.implicitHeight
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            Controls.ScrollBar.vertical: ThinScrollBar {}

            AdvancedSearch {
                id: panel

                width: parent.width
                height: implicitHeight
            }
        }
    }
}
