import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The commits of a branch rebased interactively like in GitKraken: pick,
// reword, squash into the commit below or drop each one, and drag them to
// reorder them. 'interactiveRebase' is the C++ InteractiveRebase, which
// lists the newest commit first.
Rectangle {
    id: root

    readonly property var rebase: interactiveRebase
    // Keep in sync with InteractiveRebase::Action.
    readonly property var actionNames: [qsTr("Pick"), qsTr("Reword"),
                                        qsTr("Squash"), qsTr("Drop")]
    readonly property var actionKeys: ["P", "R", "S", "D"]
    readonly property var actionColors: [Theme.added, Theme.modified,
                                         Theme.accent, Theme.deleted]
    readonly property int authorWidth: width > 760 ? 150 : 0
    readonly property int idWidth: width > 600 ? 72 : 0

    function tint(color, alpha) {
        return Qt.rgba(color.r, color.g, color.b, alpha)
    }

    function setAction(row, action) {
        if (action === 2 && !listView.itemAtIndex(row)?.canSquash)
            return
        rebase.setAction(row, action)
    }

    color: Theme.base
    focus: visible

    Keys.onEscapePressed: rebase.cancel()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // What is rebased onto what, and the buttons.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 56
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 12
                spacing: 10

                Icon {
                    name: "branch"
                    size: 18
                    color: Theme.accent
                }

                Column {
                    Layout.fillWidth: true
                    spacing: 2

                    Text {
                        width: parent.width
                        text: qsTr("Interactive Rebase")
                        elide: Text.ElideRight
                        color: Theme.text
                        font.pixelSize: 14
                        font.bold: true
                    }

                    Text {
                        width: parent.width
                        textFormat: Text.StyledText
                        text: qsTr("%1 onto %2").arg("<b>" + root.rebase.branch + "</b>")
                                                .arg("<b>" + root.rebase.onto + "</b>")
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }
                }

                Text {
                    visible: root.rebase.problem !== ""
                    Layout.maximumWidth: 280
                    text: root.rebase.problem
                    elide: Text.ElideRight
                    color: Theme.deleted
                    font.pixelSize: 12
                }

                PushButton {
                    text: qsTr("Reset")
                    enabled: root.rebase.modified
                    tip: qsTr("Undo the changes to the commits")
                    onClicked: root.rebase.reset()
                }

                PushButton {
                    text: qsTr("Cancel")
                    tip: qsTr("Close without rebasing (Esc)")
                    onClicked: root.rebase.cancel()
                }

                PushButton {
                    primary: true
                    text: qsTr("Start Rebase")
                    enabled: root.rebase.problem === ""
                    tip: qsTr("Rebase %1 with these changes").arg(root.rebase.branch)
                    onClicked: root.rebase.start()
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.border
            }
        }

        // Column titles.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 28
            color: Theme.base

            Row {
                anchors.fill: parent
                anchors.leftMargin: 34
                anchors.rightMargin: 12

                Repeater {
                    model: [
                        {title: qsTr("Action"), width: 248},
                        {title: qsTr("Commit Message"),
                         width: parent.width - 248 - root.authorWidth - root.idWidth},
                        {title: qsTr("Author"), width: root.authorWidth},
                        {title: qsTr("SHA"), width: root.idWidth}
                    ]

                    delegate: Text {
                        required property var modelData

                        visible: modelData.width > 0
                        width: modelData.width
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        text: modelData.title
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.pixelSize: 11
                        font.bold: true
                        font.capitalization: Font.AllUppercase
                        font.letterSpacing: 0.6
                    }
                }
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                height: 1
                color: Theme.border
            }
        }

        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            focus: true
            boundsBehavior: Flickable.StopAtBounds
            model: root.rebase
            currentIndex: 0

            Controls.ScrollBar.vertical: ThinScrollBar {}

            // Keys pick the action of the current commit, and Alt+arrows move it.
            Keys.onPressed: (event) => {
                if (currentIndex < 0)
                    return
                const key = event.text.toUpperCase()
                const action = root.actionKeys.indexOf(key)
                if (action >= 0 && !(event.modifiers & (Qt.ControlModifier | Qt.AltModifier))) {
                    root.setAction(currentIndex, action)
                    event.accepted = true
                } else if (event.modifiers & Qt.AltModifier && event.key === Qt.Key_Up
                           && currentIndex > 0) {
                    root.rebase.move(currentIndex, currentIndex - 1)
                    event.accepted = true
                } else if (event.modifiers & Qt.AltModifier && event.key === Qt.Key_Down
                           && currentIndex < count - 1) {
                    root.rebase.move(currentIndex, currentIndex + 1)
                    event.accepted = true
                }
            }

            moveDisplaced: Transition {
                NumberAnimation { properties: "y"; duration: 120; easing.type: Easing.OutCubic }
            }

            delegate: Rectangle {
                id: row

                required property int index
                required property string shortId
                required property string summary
                required property string message
                required property string author
                required property string initials
                required property int action
                required property bool canSquash

                readonly property bool current: ListView.isCurrentItem
                readonly property bool dropped: action === 3
                readonly property bool squashed: action === 2
                readonly property bool reworded: action === 1
                readonly property color actionColor: root.actionColors[action]

                width: ListView.view.width
                height: reworded ? Math.max(44, editor.implicitHeight + 16) : 44
                color: grip.pressed ? Theme.hover
                       : current && listView.activeFocus ? root.tint(Theme.accent, 0.12)
                       : mouse.containsMouse ? Theme.hover : "transparent"
                z: grip.pressed ? 1 : 0

                // The color of the action along the left edge.
                Rectangle {
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    width: 3
                    color: row.actionColor
                }

                MouseArea {
                    id: mouse

                    anchors.fill: parent
                    hoverEnabled: true
                    onPressed: {
                        listView.currentIndex = row.index
                        listView.forceActiveFocus()
                    }
                    onClicked: root.rebase.select(row.index)
                }

                Row {
                    anchors.fill: parent
                    anchors.leftMargin: 8
                    anchors.rightMargin: 12

                    // Drag to reorder.
                    Item {
                        width: 26
                        height: parent.height

                        Column {
                            anchors.centerIn: parent
                            spacing: 3
                            opacity: grip.containsMouse || grip.pressed ? 1 : 0.5

                            Repeater {
                                model: 3

                                Row {
                                    required property int index

                                    spacing: 3

                                    Repeater {
                                        model: 2

                                        Rectangle {
                                            width: 3
                                            height: 3
                                            radius: 1.5
                                            color: Theme.textMuted
                                        }
                                    }
                                }
                            }
                        }

                        MouseArea {
                            id: grip

                            anchors.fill: parent
                            hoverEnabled: true
                            preventStealing: true
                            cursorShape: pressed ? Qt.ClosedHandCursor : Qt.OpenHandCursor
                            onPressed: {
                                listView.currentIndex = row.index
                                listView.forceActiveFocus()
                            }
                            onPositionChanged: (event) => {
                                if (!pressed)
                                    return
                                const p = mapToItem(listView.contentItem, event.x, event.y)
                                const target = listView.indexAt(10, p.y)
                                if (target >= 0 && target !== row.index) {
                                    root.rebase.move(row.index, target)
                                    listView.currentIndex = target
                                }
                            }
                        }
                    }

                    // The actions, like GitKraken's.
                    Item {
                        width: 248
                        height: parent.height

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 4

                            Repeater {
                                model: 4

                                Rectangle {
                                    id: chip

                                    required property int index

                                    readonly property bool selected: row.action === index
                                    readonly property bool available: index !== 2 || row.canSquash
                                    readonly property color chipColor: root.actionColors[index]

                                    width: 52
                                    height: 24
                                    radius: 5
                                    opacity: available ? 1 : 0.4
                                    color: selected ? chipColor
                                           : chipMouse.containsMouse && available
                                             ? root.tint(chipColor, 0.16) : "transparent"
                                    border.color: selected ? chipColor : root.tint(chipColor, 0.5)

                                    Text {
                                        anchors.centerIn: parent
                                        text: root.actionNames[chip.index]
                                        color: chip.selected ? "#FFFFFF" : Theme.text
                                        font.pixelSize: 11
                                        font.bold: chip.selected
                                    }

                                    MouseArea {
                                        id: chipMouse

                                        anchors.fill: parent
                                        hoverEnabled: true
                                        enabled: chip.available
                                        cursorShape: Qt.PointingHandCursor
                                        onClicked: {
                                            listView.currentIndex = row.index
                                            listView.forceActiveFocus()
                                            root.setAction(row.index, chip.index)
                                        }
                                    }

                                    HoverTip {
                                        target: chip
                                        hovered: chipMouse.containsMouse
                                        text: [qsTr("Use the commit (P)"),
                                               qsTr("Use the commit and edit its message (R)"),
                                               qsTr("Combine with the commit below (S)"),
                                               qsTr("Remove the commit (D)")][chip.index]
                                    }
                                }
                            }
                        }
                    }

                    // The message, which is edited when the commit is reworded.
                    Item {
                        width: parent.width - 26 - 248 - root.authorWidth - root.idWidth
                        height: parent.height

                        Row {
                            visible: !row.reworded
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 6

                            Text {
                                visible: row.squashed
                                anchors.verticalCenter: parent.verticalCenter
                                text: "↓"
                                color: Theme.accent
                                font.pixelSize: 14
                                font.bold: true
                            }

                            Text {
                                width: parent.width - (row.squashed ? 20 : 0)
                                anchors.verticalCenter: parent.verticalCenter
                                text: row.summary
                                elide: Text.ElideRight
                                color: row.dropped ? Theme.textMuted : Theme.text
                                font.pixelSize: 13
                                font.strikeout: row.dropped
                            }
                        }

                        Rectangle {
                            visible: row.reworded
                            anchors.fill: parent
                            anchors.topMargin: 6
                            anchors.bottomMargin: 6
                            anchors.rightMargin: 8
                            radius: 5
                            color: Theme.field
                            border.width: editor.activeFocus ? 2 : 1
                            border.color: editor.activeFocus ? Theme.accent : Theme.border

                            Controls.TextArea {
                                id: editor

                                anchors.fill: parent
                                leftPadding: 8
                                rightPadding: 8
                                topPadding: 5
                                bottomPadding: 5
                                background: null
                                wrapMode: TextEdit.Wrap
                                selectByMouse: true
                                text: row.message
                                color: Theme.text
                                selectionColor: Theme.accent
                                selectedTextColor: Theme.accentText
                                font.pixelSize: 13
                                onTextChanged: {
                                    if (row.reworded && text !== row.message)
                                        root.rebase.setMessage(row.index, text)
                                }
                                Keys.onEscapePressed: listView.forceActiveFocus()
                            }
                        }
                    }

                    // Author.
                    Item {
                        visible: root.authorWidth > 0
                        width: root.authorWidth
                        height: parent.height

                        Row {
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: 8

                            Rectangle {
                                width: 22
                                height: 22
                                radius: 11
                                color: Theme.accent

                                Text {
                                    anchors.centerIn: parent
                                    text: row.initials
                                    color: "#FFFFFF"
                                    font.pixelSize: 9
                                    font.bold: true
                                }
                            }

                            Text {
                                width: root.authorWidth - 38
                                anchors.verticalCenter: parent.verticalCenter
                                text: row.author
                                elide: Text.ElideRight
                                color: Theme.textMuted
                                font.pixelSize: 12
                            }
                        }
                    }

                    Text {
                        visible: root.idWidth > 0
                        width: root.idWidth
                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        text: row.shortId
                        color: Theme.textMuted
                        font.family: Theme.monoFont
                        font.pixelSize: 12
                    }
                }

                Rectangle {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.bottom: parent.bottom
                    height: 1
                    color: Theme.border
                    opacity: 0.5
                }
            }
        }

        // How it works.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 30
            color: Theme.panel

            Text {
                anchors.fill: parent
                anchors.leftMargin: 14
                anchors.rightMargin: 14
                verticalAlignment: Text.AlignVCenter
                text: qsTr("Newest commit on top · Drag or press Alt+↑/↓ to reorder · "
                           + "P, R, S, D pick, reword, squash or drop")
                elide: Text.ElideRight
                color: Theme.textMuted
                font.pixelSize: 11
            }

            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                height: 1
                color: Theme.border
            }
        }
    }
}
