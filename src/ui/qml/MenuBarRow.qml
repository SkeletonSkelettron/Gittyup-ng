import QtQuick
import QtQuick.Layouts
import Gittyup

// The menu bar of a window, unless the platform shows the menus. 'target'
// is the C++ window, which has 'menuBarVisible', 'menuTitles' and
// showMenu().
Rectangle {
    id: root

    property var target

    Layout.preferredHeight: 28
    implicitHeight: 28
    visible: target.menuBarVisible
    color: Theme.base

    Row {
        anchors.left: parent.left
        anchors.leftMargin: 6
        anchors.verticalCenter: parent.verticalCenter

        Repeater {
            model: root.target.menuTitles

            delegate: Rectangle {
                id: title

                required property int index
                required property string modelData

                width: label.implicitWidth + 16
                height: 24
                radius: 5
                color: titleMouse.pressed || titleMouse.containsMouse ? Theme.hover
                                                                      : "transparent"

                Text {
                    id: label

                    anchors.centerIn: parent
                    text: title.modelData
                    color: Theme.text
                    font.pixelSize: 13
                }

                MouseArea {
                    id: titleMouse

                    anchors.fill: parent
                    hoverEnabled: true
                    onPressed: {
                        const p = title.mapToItem(null, 0, title.height + 2)
                        root.target.showMenu(title.index, p.x, p.y)
                    }
                }
            }
        }
    }
}
