import QtQuick
import QtQuick.Controls.Basic as Controls
import Gittyup

// The command palette (Ctrl+P), like GitKraken's fuzzy finder: commands,
// branches, files and repositories. 'commandPalette' is the C++
// CommandPalette.
Item {
    id: root

    // The item that had the focus before the palette opened.
    property Item previousFocus: null

    visible: commandPalette.visible
    z: 50

    onVisibleChanged: {
        if (visible) {
            previousFocus = Window.activeFocusItem
            input.text = commandPalette.query
            input.selectAll()
            input.forceActiveFocus()
            listView.currentIndex = 0
        } else if (previousFocus) {
            previousFocus.forceActiveFocus()
            previousFocus = null
        }
    }

    // Clicking outside closes the palette.
    Rectangle {
        anchors.fill: parent
        color: "#000000"
        opacity: Theme.dark ? 0.35 : 0.15
    }

    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.AllButtons
        onClicked: commandPalette.close()
        onWheel: (wheel) => wheel.accepted = true
    }

    Rectangle {
        id: box

        anchors.horizontalCenter: parent.horizontalCenter
        y: 64
        width: Math.min(640, parent.width - 32)
        height: column.implicitHeight
        radius: 10
        color: Theme.panel
        border.color: Theme.border
        clip: true

        // Keep clicks inside.
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
        }

        Column {
            id: column

            width: parent.width

            Item {
                width: parent.width
                height: 48

                Icon {
                    id: searchIcon

                    anchors.left: parent.left
                    anchors.leftMargin: 16
                    anchors.verticalCenter: parent.verticalCenter
                    name: "search"
                    size: 16
                    color: Theme.textMuted
                }

                Controls.TextField {
                    id: input

                    objectName: "commandPaletteInput"
                    anchors.left: searchIcon.right
                    anchors.right: parent.right
                    anchors.leftMargin: 8
                    anchors.rightMargin: 12
                    anchors.verticalCenter: parent.verticalCenter
                    background: null
                    color: Theme.text
                    selectionColor: Theme.accent
                    selectedTextColor: Theme.accentText
                    placeholderText: qsTr("Search commands, branches, files and repositories")
                    placeholderTextColor: Theme.textMuted
                    font.pixelSize: 15

                    onTextEdited: {
                        commandPalette.query = text
                        listView.currentIndex = 0
                    }

                    Keys.onUpPressed: listView.decrementCurrentIndex()
                    Keys.onDownPressed: listView.incrementCurrentIndex()
                    Keys.onReturnPressed: commandPalette.activate(listView.currentIndex)
                    Keys.onEnterPressed: commandPalette.activate(listView.currentIndex)
                    Keys.onEscapePressed: commandPalette.close()
                    Keys.onPressed: (event) => {
                        const page = Math.max(1, Math.floor(listView.height / 38) - 1)
                        if (event.key === Qt.Key_PageDown) {
                            listView.currentIndex = Math.min(listView.count - 1, listView.currentIndex + page)
                            event.accepted = true
                        } else if (event.key === Qt.Key_PageUp) {
                            listView.currentIndex = Math.max(0, listView.currentIndex - page)
                            event.accepted = true
                        }
                    }
                }
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.border
            }

            ListView {
                id: listView

                width: parent.width
                height: Math.min(count, 10) * 38 + (count > 0 ? 12 : 0)
                topMargin: 6
                bottomMargin: 6
                clip: true
                model: commandPalette
                boundsBehavior: Flickable.StopAtBounds
                highlightMoveDuration: 0
                currentIndex: 0

                Controls.ScrollBar.vertical: ThinScrollBar {}

                delegate: Item {
                    id: entry

                    required property int index
                    required property string html
                    required property string detail
                    required property string shortcut
                    required property string icon

                    readonly property bool current: ListView.isCurrentItem

                    width: ListView.view.width
                    height: 38

                    Rectangle {
                        anchors.fill: parent
                        anchors.leftMargin: 6
                        anchors.rightMargin: 6
                        radius: 6
                        color: entry.current ? Theme.selected
                                             : mouse.containsMouse ? Theme.hover : "transparent"
                    }

                    Icon {
                        id: kindIcon

                        anchors.left: parent.left
                        anchors.leftMargin: 18
                        anchors.verticalCenter: parent.verticalCenter
                        name: entry.icon
                        size: 15
                        color: entry.current ? Theme.selectedText : Theme.textMuted
                    }

                    Text {
                        id: titleText

                        anchors.left: kindIcon.right
                        anchors.leftMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        width: Math.min(implicitWidth, parent.width * 0.55)
                        textFormat: Text.StyledText
                        text: entry.html
                        elide: Text.ElideRight
                        color: entry.current ? Theme.selectedText : Theme.text
                        font.pixelSize: 13
                    }

                    Text {
                        anchors.left: titleText.right
                        anchors.leftMargin: 10
                        anchors.right: shortcutLabel.left
                        anchors.rightMargin: 10
                        anchors.verticalCenter: parent.verticalCenter
                        text: entry.detail
                        elide: Text.ElideMiddle
                        color: entry.current ? Theme.selectedText : Theme.textMuted
                        opacity: entry.current ? 0.8 : 1
                        font.pixelSize: 12
                    }

                    Text {
                        id: shortcutLabel

                        anchors.right: parent.right
                        anchors.rightMargin: 18
                        anchors.verticalCenter: parent.verticalCenter
                        text: entry.shortcut
                        color: entry.current ? Theme.selectedText : Theme.textMuted
                        font.pixelSize: 12
                    }

                    MouseArea {
                        id: mouse

                        anchors.fill: parent
                        hoverEnabled: true
                        onClicked: commandPalette.activate(entry.index)
                    }
                }
            }

            Text {
                visible: listView.count === 0
                width: parent.width
                height: 44
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: qsTr("No matches")
                color: Theme.textMuted
                font.pixelSize: 13
            }

            Rectangle {
                width: parent.width
                height: 1
                color: Theme.border
            }

            Text {
                width: parent.width
                height: 30
                leftPadding: 16
                rightPadding: 16
                verticalAlignment: Text.AlignVCenter
                text: qsTr("> commands  ·  @ branches and tags  ·  ↑↓ select  ·  Enter run  ·  Esc close")
                elide: Text.ElideRight
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }
    }
}
