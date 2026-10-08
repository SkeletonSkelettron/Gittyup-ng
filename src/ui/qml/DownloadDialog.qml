import QtQuick
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ DownloadDialog.
DialogPage {
    title: qsTr("Updating %1").arg(Qt.application.name)
    acceptText: qsTr("Install and Restart")
    acceptVisible: dialog.complete

    RowLayout {
        Layout.fillWidth: true
        spacing: 16

        Image {
            Layout.alignment: Qt.AlignTop
            width: 56
            height: 56
            sourceSize: Qt.size(112, 112)
            source: "qrc:/Gittyup.iconset/icon_128x128@2x.png"
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 10

            Text {
                Layout.fillWidth: true
                text: dialog.text
                wrapMode: Text.Wrap
                color: Theme.text
                font.pixelSize: 13
            }

            // Progress.
            Rectangle {
                Layout.fillWidth: true
                implicitHeight: 6
                radius: 3
                color: Theme.field
                border.color: Theme.border

                Rectangle {
                    width: parent.width * dialog.progress
                    height: parent.height
                    radius: 3
                    color: Theme.accent

                    Behavior on width { NumberAnimation { duration: 120 } }
                }
            }

            Text {
                text: Math.round(dialog.progress * 100) + "%"
                color: Theme.textMuted
                font.pixelSize: 11
            }
        }
    }
}
