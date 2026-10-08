import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// A pull request in place of the graph, like GitKraken shows it: its title,
// branches, labels and description, with buttons to check out its branch
// and to open it in the browser. 'pullRequests' is the C++ PullRequestList.
Rectangle {
    id: root

    readonly property var pr: pullRequests.current

    // A branch name in a chip.
    component BranchChip: Rectangle {
        property alias text: label.text

        implicitWidth: row.implicitWidth + 12
        implicitHeight: 22
        radius: 4
        color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.14)
        border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.6)

        Row {
            id: row

            anchors.centerIn: parent
            spacing: 4

            Icon {
                anchors.verticalCenter: parent.verticalCenter
                name: "branch"
                size: 12
                color: Theme.accent
            }

            Text {
                id: label

                anchors.verticalCenter: parent.verticalCenter
                color: Theme.text
                font.family: Theme.monoFont
                font.pixelSize: 12
            }
        }
    }

    color: Theme.base
    focus: visible

    Keys.onEscapePressed: pullRequests.close()

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true
            implicitHeight: header.implicitHeight + 28
            color: Theme.panel

            ColumnLayout {
                id: header

                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 14
                spacing: 10

                RowLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Icon {
                        name: "pull-request"
                        size: 20
                        color: root.pr.draft ? Theme.textMuted : Theme.added
                    }

                    Text {
                        Layout.fillWidth: true
                        textFormat: Text.StyledText
                        text: "<font color='" + Theme.textMuted + "'>#" + (root.pr.number || "")
                              + "</font> " + String(root.pr.title || "").replace(/&/g, "&amp;")
                                                                        .replace(/</g, "&lt;")
                        elide: Text.ElideRight
                        color: Theme.text
                        font.pixelSize: 16
                        font.bold: true
                    }

                    Rectangle {
                        implicitWidth: stateLabel.implicitWidth + 16
                        implicitHeight: 22
                        radius: 11
                        color: root.pr.draft ? Theme.textMuted : Theme.added

                        Text {
                            id: stateLabel

                            anchors.centerIn: parent
                            text: root.pr.draft ? qsTr("Draft") : qsTr("Open")
                            color: "#FFFFFF"
                            font.pixelSize: 11
                            font.bold: true
                        }
                    }

                    PushButton {
                        text: qsTr("Checkout")
                        icon: "branch"
                        enabled: pullRequests.canCheckout
                        tip: pullRequests.canCheckout
                             ? qsTr("Check out a local branch of %1").arg(root.pr.head)
                             : root.pr.fork ? qsTr("The branch is in a fork")
                                            : qsTr("Fetch to get the branch of the pull request")
                        onClicked: pullRequests.checkout()
                    }

                    PushButton {
                        primary: true
                        text: qsTr("Open in %1").arg(pullRequests.service)
                        tip: root.pr.url || ""
                        onClicked: pullRequests.openInBrowser()
                    }

                    ActionButton {
                        compact: true
                        icon: "close"
                        tip: qsTr("Close (Esc)")
                        onClicked: pullRequests.close()
                    }
                }

                Flow {
                    Layout.fillWidth: true
                    spacing: 6

                    Text {
                        height: 22
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("%1 wants to merge").arg(root.pr.author || "")
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }

                    BranchChip {
                        text: root.pr.head || ""
                    }

                    Text {
                        height: 22
                        verticalAlignment: Text.AlignVCenter
                        text: qsTr("into")
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }

                    BranchChip {
                        text: root.pr.base || ""
                    }

                    Text {
                        height: 22
                        verticalAlignment: Text.AlignVCenter
                        leftPadding: 8
                        text: qsTr("Opened %1 · Updated %2").arg(root.pr.created || "")
                                                             .arg(root.pr.updated || "")
                        color: Theme.textMuted
                        font.pixelSize: 12
                    }

                    Repeater {
                        model: root.pr.labels || []

                        Rectangle {
                            required property string modelData

                            implicitWidth: labelText.implicitWidth + 14
                            implicitHeight: 22
                            radius: 11
                            color: Theme.hover
                            border.color: Theme.border

                            Text {
                                id: labelText

                                anchors.centerIn: parent
                                text: parent.modelData
                                color: Theme.text
                                font.pixelSize: 11
                            }
                        }
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

        // The description.
        Controls.ScrollView {
            id: scroll

            Layout.fillWidth: true
            Layout.fillHeight: true
            contentWidth: availableWidth

            Controls.ScrollBar.vertical: ThinScrollBar {
                parent: scroll
                x: scroll.width - width - 2
                y: 2
                height: scroll.height - 4
            }

            Controls.TextArea {
                width: scroll.availableWidth
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.Wrap
                textFormat: root.pr.body ? TextEdit.MarkdownText : TextEdit.PlainText
                text: root.pr.body || qsTr("No description provided.")
                color: root.pr.body ? Theme.text : Theme.textMuted
                selectionColor: Theme.accent
                selectedTextColor: Theme.accentText
                font.pixelSize: 13
                leftPadding: 18
                rightPadding: 18
                topPadding: 14
                bottomPadding: 14
                background: null
                onLinkActivated: (link) => Qt.openUrlExternally(link)
            }
        }
    }
}
