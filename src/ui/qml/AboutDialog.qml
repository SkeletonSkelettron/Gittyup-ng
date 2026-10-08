import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// 'dialog' is the C++ AboutDialog.
Rectangle {
    id: root

    implicitWidth: 860
    implicitHeight: 600
    color: Theme.panel
    focus: true

    Keys.onEscapePressed: dialog.close()

    function focusInitialItem() {
        root.forceActiveFocus()
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0

        // The application.
        Rectangle {
            Layout.preferredWidth: 250
            Layout.fillHeight: true
            color: Theme.sidebar

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 24
                spacing: 8

                Image {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 12
                    width: 112
                    height: 112
                    sourceSize: Qt.size(224, 224)
                    source: "qrc:/Gittyup.iconset/icon_128x128@2x.png"
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 8
                    text: dialog.name
                    color: Theme.text
                    font.pixelSize: 24
                    font.weight: Font.Bold
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: qsTr("Understand your history!")
                    color: Theme.textMuted
                    font.pixelSize: 13
                }

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    Layout.topMargin: 10
                    implicitWidth: versionText.implicitWidth + 20
                    implicitHeight: 24
                    radius: 12
                    color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.16)

                    Text {
                        id: versionText

                        anchors.centerIn: parent
                        text: "v" + dialog.version
                        color: Theme.accent
                        font.pixelSize: 12
                        font.weight: Font.DemiBold
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: dialog.build
                    color: Theme.textMuted
                    font.pixelSize: 11
                }

                Item { Layout.fillHeight: true }

                Text {
                    Layout.fillWidth: true
                    text: dialog.copyright
                    wrapMode: Text.Wrap
                    horizontalAlignment: Text.AlignHCenter
                    color: Theme.textMuted
                    font.pixelSize: 11
                }
            }
        }

        Rectangle {
            Layout.fillHeight: true
            width: 1
            color: Theme.border
        }

        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            Layout.margins: 24
            spacing: 16

            Text {
                Layout.fillWidth: true
                text: dialog.support
                textFormat: Text.StyledText
                wrapMode: Text.Wrap
                linkColor: Theme.accent
                color: Theme.text
                font.pixelSize: 13
                onLinkActivated: (link) => Qt.openUrlExternally(link)

                HoverHandler {
                    cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                }
            }

            SegmentedControl {
                model: [qsTr("Changelog"), qsTr("Acknowledgments"), qsTr("Privacy")]
                currentIndex: dialog.index
                onActivated: (index) => dialog.index = index
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                radius: 8
                color: Theme.base
                border.color: Theme.border
                clip: true

                Flickable {
                    id: flickable

                    anchors.fill: parent
                    anchors.margins: 1
                    contentHeight: documentText.height + 32
                    boundsBehavior: Flickable.StopAtBounds
                    clip: true

                    Controls.ScrollBar.vertical: ThinScrollBar {}

                    Text {
                        id: documentText

                        x: 16
                        y: 16
                        width: flickable.width - 32
                        text: "<style>a { color: " + Theme.accent + "; } h3 { margin-top: 18px; }</style>"
                              + dialog.document
                        textFormat: Text.RichText
                        wrapMode: Text.Wrap
                        color: Theme.text
                        font.pixelSize: 13
                        onLinkActivated: (link) => Qt.openUrlExternally(link)
                        onTextChanged: flickable.contentY = 0

                        HoverHandler {
                            cursorShape: parent.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
                        }
                    }
                }
            }

            PushButton {
                Layout.alignment: Qt.AlignRight
                implicitHeight: 32
                minimumWidth: 88
                primary: true
                text: qsTr("Close")
                onClicked: dialog.close()
            }
        }
    }
}
