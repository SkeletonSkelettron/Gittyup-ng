import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ ThemeDialog.
DialogPage {
    title: qsTr("Welcome to %1").arg(Qt.application.name)
    subtitle: qsTr("Pick a theme. You can change it later in the settings.")
    contentWidth: 804
    acceptVisible: false
    rejectText: qsTr("Skip")

    GridLayout {
        Layout.fillWidth: true
        columns: 3
        columnSpacing: 14
        rowSpacing: 14

        Repeater {
            model: dialog.themes

            delegate: Rectangle {
                id: card

                required property var modelData

                Layout.fillWidth: true
                implicitHeight: preview.height + 76
                radius: 10
                color: cardMouse.containsMouse ? Theme.hover : Theme.base
                border.width: cardMouse.containsMouse ? 2 : 1
                border.color: cardMouse.containsMouse ? Theme.accent : Theme.border

                Image {
                    id: preview

                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.margins: 10
                    height: width * 0.8
                    source: card.modelData.image
                    fillMode: Image.PreserveAspectCrop
                    smooth: true
                    mipmap: true
                }

                Column {
                    anchors.left: parent.left
                    anchors.right: parent.right
                    anchors.top: preview.bottom
                    anchors.margins: 12
                    anchors.topMargin: 10
                    spacing: 3

                    Text {
                        width: parent.width
                        text: card.modelData.title
                        elide: Text.ElideRight
                        color: Theme.text
                        font.pixelSize: 14
                        font.weight: Font.DemiBold
                    }

                    Text {
                        width: parent.width
                        text: card.modelData.description
                        elide: Text.ElideRight
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }
                }

                MouseArea {
                    id: cardMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: dialog.choose(card.modelData.name)
                }
            }
        }
    }
}
