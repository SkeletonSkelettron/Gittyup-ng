import QtQuick
import Gittyup

// Mutually exclusive options side by side.
Rectangle {
    id: root

    property var model: []
    property int currentIndex: 0

    signal activated(int index)

    implicitWidth: row.implicitWidth + 4
    implicitHeight: 30
    radius: 7
    color: Theme.base
    border.color: Theme.border

    Row {
        id: row

        anchors.centerIn: parent
        spacing: 2

        Repeater {
            model: root.model

            delegate: Rectangle {
                id: segment

                required property int index
                required property string modelData

                readonly property bool current: index === root.currentIndex

                width: Math.max(64, label.implicitWidth + 20)
                height: root.height - 4
                radius: 5
                color: current ? Theme.accent : mouse.containsMouse ? Theme.hover : "transparent"

                Text {
                    id: label

                    anchors.centerIn: parent
                    text: segment.modelData
                    color: segment.current ? Theme.accentText : Theme.text
                    font.pixelSize: 12
                    font.weight: segment.current ? Font.DemiBold : Font.Normal
                }

                MouseArea {
                    id: mouse

                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        root.currentIndex = segment.index
                        root.activated(segment.index)
                    }
                }
            }
        }
    }
}
