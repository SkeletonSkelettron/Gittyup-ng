import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ UpdateDialog.
DialogPage {
    title: dialog.title
    subtitle: dialog.text
    contentWidth: 560
    acceptText: dialog.installable ? qsTr("Install Update") : qsTr("OK")
    rejectText: qsTr("Remind Me Later")
    rejectVisible: dialog.installable

    extraButtons: [
        PushButton {
            visible: dialog.installable
            implicitHeight: 32
            text: qsTr("Skip This Version")
            onClicked: dialog.skip()
        },
        PushButton {
            implicitHeight: 32
            icon: "star"
            text: qsTr("Donate")
            onClicked: dialog.donate()
        }
    ]

    Text {
        text: qsTr("Release notes")
        color: Theme.textMuted
        font.pixelSize: 11
        font.weight: Font.DemiBold
        font.letterSpacing: 0.8
        font.capitalization: Font.AllUppercase
    }

    Rectangle {
        Layout.fillWidth: true
        implicitHeight: 260
        radius: 8
        color: Theme.base
        border.color: Theme.border
        clip: true

        Flickable {
            id: flickable

            anchors.fill: parent
            anchors.margins: 1
            contentHeight: notes.height + 28
            boundsBehavior: Flickable.StopAtBounds
            clip: true

            Controls.ScrollBar.vertical: ThinScrollBar {}

            Text {
                id: notes

                x: 14
                y: 14
                width: flickable.width - 28
                text: "<style>a { color: " + Theme.accent + "; }</style>" + dialog.changelog
                textFormat: Text.RichText
                wrapMode: Text.Wrap
                color: Theme.text
                font.pixelSize: 13
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }
        }
    }

    CheckBox {
        visible: dialog.installable
        text: qsTr("Download and install updates automatically")
        checked: dialog.installAutomatically
        onToggled: dialog.installAutomatically = checked
    }
}
