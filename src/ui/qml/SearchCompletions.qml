import QtQuick
import QtQuick.Layouts
import Gittyup

// The completions of the word being typed in the search field. 'search' is
// the C++ SearchField.
Rectangle {
    id: root

    readonly property int rowHeight: 26

    implicitHeight: Math.min(listView.count, 8) * rowHeight + footer.height + 10
    color: Theme.panel
    border.color: Theme.border

    ListView {
        id: listView

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: footer.top
        anchors.margins: 4
        clip: true
        boundsBehavior: Flickable.StopAtBounds
        model: search.completions
        currentIndex: search.completionIndex
        onCurrentIndexChanged: {
            if (currentIndex >= 0)
                positionViewAtIndex(currentIndex, ListView.Contain)
        }

        ThinScrollBar.vertical: ThinScrollBar {}

        delegate: Rectangle {
            id: row

            required property int index
            required property string modelData

            width: ListView.view.width
            height: root.rowHeight
            radius: 4
            color: index === search.completionIndex ? Theme.selected
                                                    : rowMouse.containsMouse ? Theme.hover
                                                                             : "transparent"

            Text {
                anchors.fill: parent
                anchors.leftMargin: 8
                anchors.rightMargin: 8
                verticalAlignment: Text.AlignVCenter
                text: row.modelData
                elide: Text.ElideLeft
                color: row.index === search.completionIndex ? Theme.selectedText : Theme.text
                font.pixelSize: 12
            }

            MouseArea {
                id: rowMouse

                anchors.fill: parent
                hoverEnabled: true
                onClicked: search.applyCompletion(row.index)
            }
        }
    }

    Rectangle {
        id: footer

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
        height: 30
        color: footerMouse.containsMouse ? Theme.hover : "transparent"

        Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            height: 1
            color: Theme.border
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 11
            anchors.rightMargin: 11
            spacing: 6

            Icon {
                name: "sliders"
                size: 12
                color: Theme.textMuted
            }

            Text {
                Layout.fillWidth: true
                text: qsTr("Show Advanced Search")
                elide: Text.ElideRight
                color: Theme.textMuted
                font.pixelSize: 12
            }
        }

        MouseArea {
            id: footerMouse

            anchors.fill: parent
            hoverEnabled: true
            onClicked: search.showAdvanced()
        }
    }
}
