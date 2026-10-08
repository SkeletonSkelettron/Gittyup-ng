import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// Shown when no repository is open or a new tab is requested. 'welcome' is
// the C++ WelcomePage.
Rectangle {
    id: root

    color: Theme.base
    focus: true

    Keys.onEscapePressed: welcome.close()

    component SectionTitle: Text {
        color: Theme.textMuted
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        font.capitalization: Font.AllUppercase
    }

    component ActionCard: Rectangle {
        id: card

        property string icon
        property string title
        property string description

        signal clicked()

        Layout.fillWidth: true
        Layout.preferredWidth: 1
        implicitHeight: 112
        radius: 10
        color: cardMouse.pressed ? Theme.pressed
                                 : cardMouse.containsMouse ? Theme.hover : Theme.panel
        border.color: cardMouse.containsMouse ? Theme.accent : Theme.border
        Behavior on color { ColorAnimation { duration: 80 } }
        Behavior on border.color { ColorAnimation { duration: 80 } }

        MouseArea {
            id: cardMouse

            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: card.clicked()
        }

        Column {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 16
            spacing: 8

            Rectangle {
                width: 34
                height: 34
                radius: 17
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16)

                Icon {
                    anchors.centerIn: parent
                    name: card.icon
                    size: 18
                    color: Theme.accent
                }
            }

            Text {
                width: parent.width
                text: card.title
                elide: Text.ElideRight
                color: Theme.text
                font.pixelSize: 14
                font.weight: Font.DemiBold
            }

            Text {
                width: parent.width
                text: card.description
                elide: Text.ElideRight
                color: Theme.textMuted
                font.pixelSize: 12
            }
        }
    }

    ActionButton {
        visible: welcome.closable
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.margins: 10
        z: 1
        compact: true
        icon: "close"
        tip: qsTr("Close (Esc)")
        onClicked: welcome.close()
    }

    Flickable {
        id: flickable

        anchors.fill: parent
        contentHeight: Math.max(height, content.implicitHeight + 96)
        boundsBehavior: Flickable.StopAtBounds
        clip: true

        Controls.ScrollBar.vertical: ThinScrollBar {}

        ColumnLayout {
            id: content

            x: (flickable.width - width) / 2
            y: 48
            width: Math.min(820, flickable.width - 64)
            spacing: 0

            Row {
                spacing: 14

                Rectangle {
                    width: 48
                    height: 48
                    radius: 12
                    color: Theme.accent

                    Icon {
                        anchors.centerIn: parent
                        name: "branch"
                        size: 26
                        color: Theme.accentText
                    }
                }

                Column {
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 2

                    Text {
                        text: qsTr("Welcome to %1").arg(Qt.application.name)
                        color: Theme.text
                        font.pixelSize: 24
                        font.weight: Font.Bold
                    }

                    Text {
                        text: qsTr("Open a repository to get started.")
                        color: Theme.textMuted
                        font.pixelSize: 13
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: 32
                spacing: 14

                ActionCard {
                    icon: "folder"
                    title: qsTr("Open a repository")
                    description: qsTr("Browse for a repository on this computer")
                    onClicked: welcome.openRepository()
                }

                ActionCard {
                    icon: "cloud"
                    title: qsTr("Clone a repository")
                    description: qsTr("Copy a remote repository to this computer")
                    onClicked: welcome.cloneRepository()
                }

                ActionCard {
                    icon: "plus"
                    title: qsTr("Start a local repository")
                    description: qsTr("Initialize a new, empty repository")
                    onClicked: welcome.initRepository()
                }
            }

            SectionTitle {
                Layout.topMargin: 32
                Layout.bottomMargin: 8
                text: qsTr("Recent repositories")
            }

            Rectangle {
                Layout.fillWidth: true
                // Up to eight repositories without scrolling.
                Layout.preferredHeight: Math.max(64, Math.min(recentList.contentHeight + 2,
                                                              8 * 44 + 2))
                radius: 10
                color: Theme.panel
                border.color: Theme.border
                clip: true

                Text {
                    visible: recentList.count === 0
                    anchors.centerIn: parent
                    text: qsTr("Repositories you open will show up here.")
                    color: Theme.textMuted
                    font.pixelSize: 12
                }

                ListView {
                    id: recentList

                    anchors.fill: parent
                    anchors.margins: 1
                    clip: true
                    interactive: contentHeight > height
                    boundsBehavior: Flickable.StopAtBounds
                    model: welcome.recent

                    Controls.ScrollBar.vertical: ThinScrollBar { thickness: 6 }

                    delegate: Item {
                        id: recent

                        required property int index
                        required property var modelData

                        width: ListView.view.width
                        height: 44

                        Rectangle {
                            anchors.fill: parent
                            anchors.margins: 4
                            radius: 6
                            color: recentMouse.pressed ? Theme.pressed
                                                       : recentMouse.containsMouse ? Theme.hover
                                                                                   : "transparent"
                        }

                        Rectangle {
                            visible: recent.index > 0
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            height: 1
                            color: Theme.border
                            opacity: 0.6
                        }

                        MouseArea {
                            id: recentMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: welcome.openRecent(recent.modelData.path)
                        }

                        Icon {
                            id: recentIcon

                            anchors.left: parent.left
                            anchors.leftMargin: 16
                            anchors.verticalCenter: parent.verticalCenter
                            name: "repo"
                            size: 16
                            color: Theme.accent
                        }

                        Text {
                            id: recentName

                            anchors.left: recentIcon.right
                            anchors.leftMargin: 12
                            anchors.verticalCenter: parent.verticalCenter
                            width: Math.min(implicitWidth, parent.width * 0.4)
                            text: recent.modelData.name
                            elide: Text.ElideRight
                            color: Theme.text
                            font.pixelSize: 13
                            font.weight: Font.DemiBold
                        }

                        Text {
                            anchors.left: recentName.right
                            anchors.leftMargin: 12
                            anchors.right: removeButton.left
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            text: recent.modelData.display
                            elide: Text.ElideMiddle
                            color: Theme.textMuted
                            font.pixelSize: 12
                        }

                        Rectangle {
                            id: removeButton

                            anchors.right: parent.right
                            anchors.rightMargin: 14
                            anchors.verticalCenter: parent.verticalCenter
                            width: 22
                            height: 22
                            radius: 5
                            opacity: recentMouse.containsMouse || removeMouse.containsMouse ? 1 : 0
                            color: removeMouse.containsMouse ? Theme.pressed : "transparent"

                            Icon {
                                anchors.centerIn: parent
                                name: "close"
                                size: 10
                                color: Theme.textMuted
                            }

                            MouseArea {
                                id: removeMouse

                                anchors.fill: parent
                                hoverEnabled: true
                                onClicked: welcome.removeRecent(recent.modelData.path)
                            }

                            HoverTip {
                                target: removeButton
                                text: qsTr("Remove from the list")
                                hovered: removeMouse.containsMouse
                            }
                        }
                    }
                }
            }

            SectionTitle {
                Layout.topMargin: 28
                Layout.bottomMargin: 10
                text: qsTr("Connect a hosting service")
            }

            Flow {
                Layout.fillWidth: true
                spacing: 8

                Repeater {
                    model: welcome.accounts

                    delegate: Rectangle {
                        id: account

                        required property var modelData

                        width: accountRow.implicitWidth + 24
                        height: 34
                        radius: 17
                        color: accountMouse.pressed ? Theme.pressed
                                                    : accountMouse.containsMouse ? Theme.hover
                                                                                 : Theme.panel
                        border.color: accountMouse.containsMouse ? Theme.accent : Theme.border

                        Row {
                            id: accountRow

                            anchors.centerIn: parent
                            spacing: 8

                            Image {
                                anchors.verticalCenter: parent.verticalCenter
                                width: 16
                                height: 16
                                sourceSize: Qt.size(32, 32)
                                source: "image://icons/account-" + account.modelData.kind
                            }

                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: account.modelData.name
                                color: Theme.text
                                font.pixelSize: 12
                            }
                        }

                        MouseArea {
                            id: accountMouse

                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: welcome.addAccount(account.modelData.kind)
                        }
                    }
                }
            }

            Text {
                Layout.topMargin: 28
                text: qsTr("Need help? <a href=\"support\">Contact us for support</a>")
                textFormat: Text.StyledText
                linkColor: Theme.accent
                color: Theme.textMuted
                font.pixelSize: 12
                onLinkActivated: welcome.openSupport()

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }
        }
    }
}
