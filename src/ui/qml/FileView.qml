import QtQuick
import QtQuick.Controls.Basic as Controls
import QtQuick.Layouts
import Gittyup

// The content of the file selected in tree mode, with the commit that last
// changed each line on the left. 'detailView.content' is the C++
// FileViewModel.
Rectangle {
    id: root

    readonly property var content: detailView.content

    // Fit the code font of the editor settings.
    readonly property int lineHeight: Math.max(20, Math.ceil(metrics.height) + 4)
    readonly property real charWidth: metrics.advanceWidth("0")
    readonly property int numberWidth: charWidth * content.lineNumberWidth + 16
    readonly property bool showBlame: content.hasBlame || content.blameLoading
    readonly property int blameWidth: showBlame ? 300 : 0
    readonly property int gutterWidth: blameWidth + numberWidth
    readonly property int codeX: gutterWidth + 12

    // The commit of the lines under the mouse.
    property string hoveredCommit

    color: Theme.base
    focus: visible

    Keys.onEscapePressed: detailView.closeFile()

    FontMetrics {
        id: metrics

        font.family: Theme.codeFont
        font.pointSize: Theme.codeFontSize
    }

    // Scroll to the current match of the find bar. The gutter covers the
    // left of the view.
    Connections {
        target: detailView.finder
        enabled: root.visible

        function onCurrentChanged(row, start, length) {
            listView.positionViewAtIndex(row, ListView.Contain)
            const x = root.codeX + start * root.charWidth
            const right = x + length * root.charWidth
            if (right > listView.contentX + listView.width)
                listView.contentX = Math.min(right - listView.width + 40, listView.contentWidth - listView.width)
            else if (x < listView.contentX + root.codeX)
                listView.contentX = Math.max(0, x - root.codeX - 40)
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        // File header.
        Rectangle {
            Layout.fillWidth: true
            implicitHeight: 44
            color: Theme.panel

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 10
                spacing: 8

                ActionButton {
                    compact: true
                    icon: "close"
                    tip: qsTr("Close the file (Esc)")
                    onClicked: detailView.closeFile()
                }

                Icon {
                    name: "file"
                    size: 14
                    color: Theme.textMuted
                }

                Text {
                    Layout.fillWidth: true
                    textFormat: Text.StyledText
                    text: {
                        const path = root.content.path
                        const slash = path.lastIndexOf("/")
                        const name = "<b>" + path.substring(slash + 1) + "</b>"
                        return (slash < 0 ? "" : path.substring(0, slash + 1)) + name
                    }
                    elide: Text.ElideLeft
                    color: Theme.text
                    font.pixelSize: 13
                }

                Spinner {
                    running: root.content.blameLoading
                    size: 14
                }

                Text {
                    visible: root.content.blameLoading
                    text: qsTr("Finding the commits of the lines...")
                    color: Theme.textMuted
                    font.pixelSize: 12
                }

                // The version of the file.
                Rectangle {
                    visible: root.content.revision !== ""
                    implicitWidth: revisionText.implicitWidth + 16
                    implicitHeight: 22
                    radius: 11
                    color: "transparent"
                    border.color: Theme.border

                    Text {
                        id: revisionText

                        anchors.centerIn: parent
                        text: root.content.revision
                        color: Theme.textMuted
                        font.family: Theme.monoFont
                        font.pixelSize: 11
                    }
                }

                ActionButton {
                    compact: true
                    icon: "pencil"
                    tip: qsTr("Open in an editor window")
                    onClicked: detailView.openTreeFile(root.content.path)
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

        FindBar {
            Layout.fillWidth: true
            visible: finder.visible
            finder: detailView.finder
        }

        // Binary, large and empty files.
        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.content.notice !== ""

            Text {
                anchors.centerIn: parent
                text: root.content.notice
                color: Theme.textMuted
                font.pixelSize: 13
            }
        }

        ListView {
            id: listView

            Layout.fillWidth: true
            Layout.fillHeight: true
            visible: root.content.notice === ""
            clip: true
            model: root.content
            reuseItems: true
            boundsBehavior: Flickable.StopAtBounds
            flickableDirection: Flickable.AutoFlickIfNeeded
            contentWidth: Math.max(width, root.codeX
                                          + root.content.maxLineLength * root.charWidth + 40)

            Controls.ScrollBar.vertical: ThinScrollBar {}

            Controls.ScrollBar.horizontal: ThinScrollBar {}

            delegate: Item {
                id: row

                required property int number
                required property string html
                required property string blameId
                required property bool blameFirst
                required property bool blameLast
                required property int blameOffset
                required property bool blameCommitted
                required property string blameSummary
                required property string blameAuthor
                required property string blameDate
                required property string blameColor
                required property string blameTip
                required property var matches

                width: listView.contentWidth
                height: root.lineHeight

                Item {
                    x: root.codeX
                    width: codeText.implicitWidth
                    height: parent.height

                    FindMatches {
                        matches: row.matches
                        charWidth: root.charWidth
                    }

                    Text {
                        id: codeText

                        height: parent.height
                        verticalAlignment: Text.AlignVCenter
                        textFormat: Text.RichText
                        text: row.html
                        color: Theme.text
                        font.family: Theme.codeFont
                        font.pointSize: Theme.codeFontSize
                    }
                }

                // The gutter stays in place when the lines scroll sideways.
                BlameGutter {
                    x: listView.contentX
                    height: parent.height
                    blame: root.content
                    blameWidth: root.blameWidth
                    numberWidth: root.numberWidth
                    hoveredCommit: root.hoveredCommit
                    number: row.number
                    blameId: row.blameId
                    blameFirst: row.blameFirst
                    blameLast: row.blameLast
                    blameOffset: row.blameOffset
                    blameCommitted: row.blameCommitted
                    blameSummary: row.blameSummary
                    blameAuthor: row.blameAuthor
                    blameDate: row.blameDate
                    blameColor: row.blameColor
                    blameTip: row.blameTip
                    onHoverRequested: (id) => root.hoveredCommit = id
                }
            }
        }
    }
}
